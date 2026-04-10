#include "grid.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

const int NUM_THREADS = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 4;

#if defined(__AVX512F__)
    const int GOL_CHUNK_SIZE = 512;
#elif defined(__AVX2__)
    const int GOL_CHUNK_SIZE = 256;
#elif defined(__ARM_NEON) || defined(__SSE2__)
    const int GOL_CHUNK_SIZE = 128;
#else
    const int GOL_CHUNK_SIZE = 256;
#endif
const int GOL_WORDS_PER_ROW = GOL_CHUNK_SIZE / 64;

struct GOLChunk {
    uint64_t cells[GOL_CHUNK_SIZE * GOL_WORDS_PER_ROW] = {}; // 8KB bitfield
    int alive_count = 0;
};

static inline bool chunk_get_bit(const GOLChunk &c, int lx, int ly) {
    int word = ly * GOL_WORDS_PER_ROW + (lx >> 6);
    return (c.cells[word] >> (lx & 63)) & 1;
}

static inline void chunk_set_bit(GOLChunk &c, int lx, int ly) {
    int word = ly * GOL_WORDS_PER_ROW + (lx >> 6);
    c.cells[word] |= (uint64_t)1 << (lx & 63);
}

static inline void chunk_clear_bit(GOLChunk &c, int lx, int ly) {
    int word = ly * GOL_WORDS_PER_ROW + (lx >> 6);
    c.cells[word] &= ~((uint64_t)1 << (lx & 63));
}

static inline int chunk_popcount(const GOLChunk &c) {
    int count = 0;
    for (int i = 0; i < GOL_CHUNK_SIZE * GOL_WORDS_PER_ROW; i++)
        count += __builtin_popcountll(c.cells[i]);
    return count;
}

// two maps: read from one, write to the other, swap each step
std::unordered_map<uint64_t, GOLChunk> gol_maps[2];
int gol_read = 0;

static inline uint64_t gol_chunk_key(int32_t cx, int32_t cy) {
    return (uint64_t)(uint32_t)cx << 32 | (uint32_t)cy;
}

static inline int32_t floor_div(int x, int d) {
    return x >= 0 ? x / d : (x - d + 1) / d;
}

static inline uint8_t get_cell(std::unordered_map<uint64_t, GOLChunk> &m, int x, int y) {
    int32_t cx = floor_div(x, GOL_CHUNK_SIZE);
    int32_t cy = floor_div(y, GOL_CHUNK_SIZE);
    uint64_t key = gol_chunk_key(cx, cy);
    auto it = m.find(key);
    if (it == m.end()) return 0;
    int lx = x - cx * GOL_CHUNK_SIZE;
    int ly = y - cy * GOL_CHUNK_SIZE;
    return chunk_get_bit(it->second, lx, ly) ? 1 : 0;
}


static inline bool is_alive(std::unordered_map<uint64_t, GOLChunk> &m, int x, int y) {
    return get_cell(m, x, y) != 0;
}

bool paused = true;
bool left_mouse_pressed = false;
bool right_mouse_pressed = false;

float speeds[10] = {0, 1, 0.25, 0.05, 0.01, 0.005, 0.001, 0.0005, 0.0001, 0.00005};

void Grid::onKeyPressEvent(int key_code) {
    switch (key_code) {
        case sf::Keyboard::Space:
            if (paused) {
                paused = false;
                startTimer();
            }
            else {
                paused = true;
                stopTimer();
            }
            break;

        case sf::Keyboard::Num1:
        case sf::Keyboard::Num2:
        case sf::Keyboard::Num3:
        case sf::Keyboard::Num4:
        case sf::Keyboard::Num5:
        case sf::Keyboard::Num6:
        case sf::Keyboard::Num7:
        case sf::Keyboard::Num8:
        case sf::Keyboard::Num9:
        case sf::Keyboard::Num0:
            int speed_idx = key_code - sf::Keyboard::Num0;
            setTimer(speeds[speed_idx]);
            break;
    }
}

void Grid::onKeyReleaseEvent(int key_code) {
}

static void gol_set_alive(int x, int y) {
    for (int i = 0; i < 2; i++) {
        int32_t cx = floor_div(x, GOL_CHUNK_SIZE);
        int32_t cy = floor_div(y, GOL_CHUNK_SIZE);
        uint64_t key = gol_chunk_key(cx, cy);
        auto &chunk = gol_maps[i][key];
        int lx = x - cx * GOL_CHUNK_SIZE;
        int ly = y - cy * GOL_CHUNK_SIZE;
        if (!chunk_get_bit(chunk, lx, ly)) {
            chunk_set_bit(chunk, lx, ly);
            chunk.alive_count++;
        }
    }
}

static void gol_set_dead(int x, int y) {
    for (int i = 0; i < 2; i++) {
        int32_t cx = floor_div(x, GOL_CHUNK_SIZE);
        int32_t cy = floor_div(y, GOL_CHUNK_SIZE);
        uint64_t key = gol_chunk_key(cx, cy);
        auto it = gol_maps[i].find(key);
        if (it == gol_maps[i].end()) continue;
        int lx = x - cx * GOL_CHUNK_SIZE;
        int ly = y - cy * GOL_CHUNK_SIZE;
        if (chunk_get_bit(it->second, lx, ly)) {
            chunk_clear_bit(it->second, lx, ly);
            it->second.alive_count--;
        }
    }
}

struct WorkItem {
    uint64_t key;
    GOLChunk* read_chunk;
    GOLChunk* write_chunk;
    GOLChunk* nb[3][3];
};

static std::vector<WorkItem> work_items;
static std::atomic<int> work_idx(0);
static int pool_n_items = 0;

// get a row of 4 words from a chunk (row ly)
static inline void get_row(const GOLChunk &c, int ly, uint64_t out[GOL_WORDS_PER_ROW]) {
    const uint64_t *base = c.cells + ly * GOL_WORDS_PER_ROW;
    for (int i = 0; i < GOL_WORDS_PER_ROW; i++) out[i] = base[i];
}

// shift a row right by 1 bit (towards higher x), inserting 'carry_in' at bit 0 of word 0
static inline void row_shift_right(const uint64_t in[GOL_WORDS_PER_ROW], uint64_t out[GOL_WORDS_PER_ROW], uint64_t carry_in) {
    for (int i = 0; i < GOL_WORDS_PER_ROW; i++) {
        out[i] = (in[i] << 1) | carry_in;
        carry_in = in[i] >> 63;
    }
}

// shift a row left by 1 bit (towards lower x), inserting 'carry_in' at top bit of last word
static inline void row_shift_left(const uint64_t in[GOL_WORDS_PER_ROW], uint64_t out[GOL_WORDS_PER_ROW], uint64_t carry_in) {
    for (int i = GOL_WORDS_PER_ROW - 1; i >= 0; i--) {
        out[i] = (in[i] >> 1) | (carry_in << 63);
        carry_in = in[i] & 1;
    }
}

// full-adder: a + b + c = (sum, carry)
static inline void full_add(uint64_t a, uint64_t b, uint64_t c, uint64_t &sum, uint64_t &carry) {
    uint64_t ab = a ^ b;
    sum = ab ^ c;
    carry = (a & b) | (ab & c);
}

static void process_work_item(WorkItem &item) {
    const GOLChunk &read_chunk = *item.read_chunk;
    GOLChunk &write_chunk = *item.write_chunk;
    GOLChunk* (&nb)[3][3] = item.nb;

    // process all 256 rows using bitwise parallel neighbor counting
    // for each row, we need 3 rows of input (above, current, below)
    // and for each row we shift left/right to get horizontal neighbors
    // sum all 8 neighbors using full-adder trees into 4-bit count per cell
    // GOL rule: next = (count==3) | (alive & count==2)

    for (int ly = 0; ly < GOL_CHUNK_SIZE; ly++) {

        // get the 3 rows we need (above, current, below)
        uint64_t row_above[GOL_WORDS_PER_ROW], row_cur[GOL_WORDS_PER_ROW], row_below[GOL_WORDS_PER_ROW];

        if (ly == 0)
            get_row(*nb[0][1], GOL_CHUNK_SIZE - 1, row_above);
        else
            get_row(read_chunk, ly - 1, row_above);

        get_row(read_chunk, ly, row_cur);

        if (ly == GOL_CHUNK_SIZE - 1)
            get_row(*nb[2][1], 0, row_below);
        else
            get_row(read_chunk, ly + 1, row_below);

        int above_ly = ly - 1;
        int below_ly = ly + 1;

        uint64_t left_carry_above = (ly == 0) ?
            (nb[0][0]->cells[(GOL_CHUNK_SIZE - 1) * GOL_WORDS_PER_ROW + (GOL_WORDS_PER_ROW - 1)] >> 63) & 1 :
            (nb[1][0]->cells[above_ly * GOL_WORDS_PER_ROW + (GOL_WORDS_PER_ROW - 1)] >> 63) & 1;
        uint64_t right_carry_above = (ly == 0) ?
            nb[0][2]->cells[(GOL_CHUNK_SIZE - 1) * GOL_WORDS_PER_ROW] & 1 :
            nb[1][2]->cells[above_ly * GOL_WORDS_PER_ROW] & 1;

        uint64_t left_carry_cur = (nb[1][0]->cells[ly * GOL_WORDS_PER_ROW + (GOL_WORDS_PER_ROW - 1)] >> 63) & 1;
        uint64_t right_carry_cur = nb[1][2]->cells[ly * GOL_WORDS_PER_ROW] & 1;

        uint64_t left_carry_below = (ly == GOL_CHUNK_SIZE - 1) ?
            (nb[2][0]->cells[0 * GOL_WORDS_PER_ROW + (GOL_WORDS_PER_ROW - 1)] >> 63) & 1 :
            (nb[1][0]->cells[below_ly * GOL_WORDS_PER_ROW + (GOL_WORDS_PER_ROW - 1)] >> 63) & 1;
        uint64_t right_carry_below = (ly == GOL_CHUNK_SIZE - 1) ?
            nb[2][2]->cells[0 * GOL_WORDS_PER_ROW] & 1 :
            nb[1][2]->cells[below_ly * GOL_WORDS_PER_ROW] & 1;

        // skip row if all 3 rows and all 6 carry bits are zero
        uint64_t any = left_carry_above | right_carry_above | left_carry_cur | right_carry_cur | left_carry_below | right_carry_below;
        if (!any) {
            for (int w = 0; w < GOL_WORDS_PER_ROW; w++)
                any |= row_above[w] | row_cur[w] | row_below[w];
            if (!any) {
                int row_base = ly * GOL_WORDS_PER_ROW;
                for (int w = 0; w < GOL_WORDS_PER_ROW; w++)
                    write_chunk.cells[row_base + w] = 0;
                continue;
            }
        }

        uint64_t above_l[GOL_WORDS_PER_ROW], above_r[GOL_WORDS_PER_ROW];
        uint64_t cur_l[GOL_WORDS_PER_ROW], cur_r[GOL_WORDS_PER_ROW];
        uint64_t below_l[GOL_WORDS_PER_ROW], below_r[GOL_WORDS_PER_ROW];

        row_shift_right(row_above, above_l, left_carry_above);
        row_shift_left(row_above, above_r, right_carry_above);
        row_shift_right(row_cur, cur_l, left_carry_cur);
        row_shift_left(row_cur, cur_r, right_carry_cur);
        row_shift_right(row_below, below_l, left_carry_below);
        row_shift_left(row_below, below_r, right_carry_below);

        int row_base = ly * GOL_WORDS_PER_ROW;
        for (int w = 0; w < GOL_WORDS_PER_ROW; w++) {
            uint64_t s1, c1, s2, c2, s3, c3;
            full_add(above_l[w], row_above[w], above_r[w], s1, c1);
            full_add(cur_l[w], cur_r[w], below_l[w], s2, c2);
            s3 = row_below[w] ^ below_r[w];
            c3 = row_below[w] & below_r[w];

            uint64_t bit0, carry_s;
            full_add(s1, s2, s3, bit0, carry_s);

            uint64_t mid, carry_c;
            full_add(c1, c2, c3, mid, carry_c);

            uint64_t bit1 = carry_s ^ mid;
            uint64_t carry_h = carry_s & mid;

            uint64_t bit2 = carry_c ^ carry_h;
            uint64_t not_bit3 = ~(carry_c & carry_h);

            uint64_t alive = row_cur[w];
            uint64_t next = not_bit3 & ~bit2 & bit1 & (bit0 | alive);

            write_chunk.cells[row_base + w] = next;
        }
    }

    write_chunk.alive_count = chunk_popcount(write_chunk);
}

// persistent thread pool — condvar based
static std::atomic<int> pool_generation{0};
static std::atomic<int> pool_done_count{0};
static std::atomic<bool> pool_shutdown{false};
static std::mutex pool_mtx;
static std::condition_variable pool_cv;

static void pool_worker_func() {
    int my_gen = 0;
    while (true) {
        {
            std::unique_lock<std::mutex> lk(pool_mtx);
            pool_cv.wait(lk, [&] {
                return pool_generation.load(std::memory_order_acquire) != my_gen
                    || pool_shutdown.load(std::memory_order_relaxed);
            });
        }
        if (pool_shutdown.load(std::memory_order_relaxed)) return;
        my_gen = pool_generation.load(std::memory_order_relaxed);

        while (true) {
            int i = work_idx.fetch_add(1, std::memory_order_relaxed);
            if (i >= pool_n_items) break;
            process_work_item(work_items[i]);
        }
        pool_done_count.fetch_add(1, std::memory_order_release);
    }
}

struct PoolGuard {
    std::vector<std::thread> threads;
    int n_workers = 0;
    bool started = false;
    void ensure_started() {
        if (started) return;
        started = true;
        n_workers = NUM_THREADS - 1;
        for (int t = 0; t < n_workers; t++)
            threads.emplace_back(pool_worker_func);
    }
    ~PoolGuard() {
        if (!started) return;
        {
            std::lock_guard<std::mutex> lk(pool_mtx);
            pool_shutdown.store(true, std::memory_order_relaxed);
        }
        pool_cv.notify_all();
        for (auto &t : threads) t.join();
    }
};
static PoolGuard pool;

void Grid::onTimerEvent(int n_iterations) {
    if (paused) return;

    using clock = std::chrono::high_resolution_clock;
    static int perf_steps = 0;
    static double perf_sim_ms = 0;
    static double perf_draw_ms = 0;
    static long long perf_draw_count = 0;
    static clock::time_point perf_last = clock::now();

    // snapshot initial state for diffing after all iterations
    const int CELLS_SIZE = GOL_CHUNK_SIZE * GOL_WORDS_PER_ROW;
    auto &initial_map = gol_maps[gol_read];
    static std::unordered_map<uint64_t, std::vector<uint64_t>> initial_snapshot;
    initial_snapshot.clear();
    for (auto &pair : initial_map) {
        if (pair.second.alive_count > 0) {
            auto &snap = initial_snapshot[pair.first];
            snap.resize(CELLS_SIZE);
            memcpy(snap.data(), pair.second.cells, CELLS_SIZE * sizeof(uint64_t));
        }
    }

    for (int iter = 0; iter < n_iterations; iter++) {
        auto sim_start = clock::now();

        auto &read_map = gol_maps[gol_read];
        auto &write_map = gol_maps[!gol_read];

        // (write chunks are zeroed per-row inside process_work_item)

        // build work list: every chunk that needs processing
        work_items.clear();

        // static empty chunk for neighbor lookups into non-existent chunks
        static GOLChunk empty_chunk = {};

        // collect keys of alive chunks, then ensure their neighbors exist in write_map only
        // (we no longer insert into read_map — missing neighbors use empty_chunk)
        {
            std::vector<uint64_t> alive_keys;
            for (auto &pair : read_map)
                if (pair.second.alive_count > 0)
                    alive_keys.push_back(pair.first);
            for (uint64_t key : alive_keys) {
                int32_t cx = (int32_t)(key >> 32);
                int32_t cy = (int32_t)(key & 0xFFFFFFFF);
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        uint64_t nk = gol_chunk_key(cx + dx, cy + dy);
                        write_map[nk]; // ensure write target exists
                    }
            }
        }

        // build work items for every chunk in write_map (all chunks that could gain life)
        for (auto &pair : write_map) {
            int32_t cx = (int32_t)(pair.first >> 32);
            int32_t cy = (int32_t)(pair.first & 0xFFFFFFFF);

            WorkItem item;
            item.key = pair.first;

            auto read_it = read_map.find(pair.first);
            item.read_chunk = (read_it != read_map.end()) ? &read_it->second : &empty_chunk;
            item.write_chunk = &pair.second;

            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) {
                        item.nb[dy + 1][dx + 1] = item.read_chunk;
                    } else {
                        uint64_t nk = gol_chunk_key(cx + dx, cy + dy);
                        auto it = read_map.find(nk);
                        item.nb[dy + 1][dx + 1] = (it != read_map.end()) ? &it->second : &empty_chunk;
                    }
                }

            work_items.push_back(item);
        }

        // dispatch to persistent thread pool
        pool.ensure_started();
        work_idx.store(0, std::memory_order_relaxed);
        pool_n_items = work_items.size();
        pool_done_count.store(0, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(pool_mtx);
            pool_generation.fetch_add(1, std::memory_order_release);
        }
        pool_cv.notify_all();

        // main thread also processes work items
        while (true) {
            int i = work_idx.fetch_add(1, std::memory_order_relaxed);
            if (i >= pool_n_items) break;
            process_work_item(work_items[i]);
        }

        // spin-wait for all pool workers to finish
        while (pool_done_count.load(std::memory_order_acquire) < pool.n_workers) {
        }

        // remove empty chunks that have no alive neighbors
        for (auto it = write_map.begin(); it != write_map.end(); ) {
            if (it->second.alive_count == 0) {
                int32_t cx = (int32_t)(it->first >> 32);
                int32_t cy = (int32_t)(it->first & 0xFFFFFFFF);
                bool keep = false;
                for (int dy = -1; dy <= 1 && !keep; dy++)
                    for (int dx = -1; dx <= 1 && !keep; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        auto nb = write_map.find(gol_chunk_key(cx + dx, cy + dy));
                        if (nb != write_map.end() && nb->second.alive_count > 0)
                            keep = true;
                    }
                if (!keep)
                    it = write_map.erase(it);
                else
                    ++it;
            }
            else
                ++it;
        }

        auto sim_end = clock::now();
        perf_sim_ms += std::chrono::duration<double, std::milli>(sim_end - sim_start).count();
        perf_steps++;

        gol_read = !gol_read;

        if (_timer.getElapsedTime().asSeconds() > _frame_duration * 3)
            break;
    }

    // diff final state vs initial snapshot, draw only changes
    auto draw_start = clock::now();
    auto &final_map = gol_maps[gol_read];

    auto draw_bits = [&](uint64_t bits, int base_x, int base_y, int row, int word_idx, sf::Color color) {
        int bx = base_x + word_idx * 64;
        int by = base_y + row;
        while (bits) {
            int bit = __builtin_ctzll(bits);
            threadDrawCell(bx + bit, by, color);
            perf_draw_count++;
            bits &= bits - 1;
        }
    };

    // cells that were alive initially but are now dead or gone
    for (auto &[key, snap] : initial_snapshot) {
        int32_t cx = (int32_t)(key >> 32);
        int32_t cy = (int32_t)(key & 0xFFFFFFFF);
        int base_x = cx * GOL_CHUNK_SIZE;
        int base_y = cy * GOL_CHUNK_SIZE;

        auto it = final_map.find(key);
        if (it == final_map.end()) {
            // entire chunk died — draw all previously alive cells as background
            for (int row = 0; row < GOL_CHUNK_SIZE; row++)
                for (int w = 0; w < GOL_WORDS_PER_ROW; w++)
                    draw_bits(snap[row * GOL_WORDS_PER_ROW + w], base_x, base_y, row, w, _background_color);
        } else {
            const uint64_t *final_cells = it->second.cells;
            for (int row = 0; row < GOL_CHUNK_SIZE; row++) {
                for (int w = 0; w < GOL_WORDS_PER_ROW; w++) {
                    int idx = row * GOL_WORDS_PER_ROW + w;
                    uint64_t diff = snap[idx] ^ final_cells[idx];
                    if (!diff) continue;
                    // newly alive cells
                    draw_bits(diff & final_cells[idx], base_x, base_y, row, w, _foreground_color);
                    // newly dead cells
                    draw_bits(diff & snap[idx], base_x, base_y, row, w, _background_color);
                }
            }
        }
    }

    // cells that are alive now but weren't in the snapshot (new chunks)
    for (auto &pair : final_map) {
        if (pair.second.alive_count == 0) continue;
        if (initial_snapshot.count(pair.first)) continue; // already diffed

        int32_t cx = (int32_t)(pair.first >> 32);
        int32_t cy = (int32_t)(pair.first & 0xFFFFFFFF);
        int base_x = cx * GOL_CHUNK_SIZE;
        int base_y = cy * GOL_CHUNK_SIZE;

        for (int row = 0; row < GOL_CHUNK_SIZE; row++)
            for (int w = 0; w < GOL_WORDS_PER_ROW; w++)
                draw_bits(pair.second.cells[row * GOL_WORDS_PER_ROW + w], base_x, base_y, row, w, _foreground_color);
    }

    auto draw_end = clock::now();
    perf_draw_ms += std::chrono::duration<double, std::milli>(draw_end - draw_start).count();

    auto now = clock::now();
    if (std::chrono::duration<double>(now - perf_last).count() >= 1.0) {
        if (perf_steps > 0) {
            printf("sim: %.2fms  draw: %.2fms  draws/step: %lld  steps/s: %d  chunks: %zu\n",
                perf_sim_ms / perf_steps,
                perf_draw_ms / perf_steps,
                perf_draw_count / perf_steps,
                perf_steps,
                gol_maps[gol_read].size());
        }
        perf_steps = 0;
        perf_sim_ms = 0;
        perf_draw_ms = 0;
        perf_draw_count = 0;
        perf_last = now;
    }
}

void Grid::onMouseDragEvent(int x, int y) {
    if (!paused) return;

    bool alive = is_alive(gol_maps[gol_read], x, y);

    if (left_mouse_pressed) {
        if (!alive) {
            gol_set_alive(x, y);
            drawCell(x, y, _foreground_color, 0.1);
        }
    }
    else if (right_mouse_pressed) {
        if (alive) {
            gol_set_dead(x, y);
            drawCell(x, y, _background_color, 0.1);
        }
    }
}

void Grid::onMousePressEvent(int x, int y, int button) {
    bool alive = is_alive(gol_maps[gol_read], x, y);

    if (button == sf::Mouse::Left) {
        left_mouse_pressed = true;
        if (paused && !alive) {
            gol_set_alive(x, y);
            drawCell(x, y, _foreground_color, 0.1);
        }
    }

    else if (button == sf::Mouse::Right) {
        right_mouse_pressed = true;
        if (paused && alive) {
            gol_set_dead(x, y);
            drawCell(x, y, _background_color, 0.1);
        }
    }
}

void Grid::onMouseReleaseEvent(int x, int y, int button) {
    if (button == sf::Mouse::Left)
        left_mouse_pressed = false;

    else if (button == sf::Mouse::Right)
        right_mouse_pressed = false;
}

void Grid::onStartEvent() {
    addText(4,  5, "GRID", _foreground_color, 0);

    int speed_idx = 2;
    setTimer(speeds[speed_idx]);
}

int main() {
    Grid grid("Grid", 12, 11, 69);
    return grid.start();
}
