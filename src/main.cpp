#include "grid.h"
#include <unordered_set>
#include <iostream>
#include <unordered_map>
#include <vector>


// yes i know this needs to be cleaned up


std::vector<std::pair<int, int>> change_list;
std::unordered_set<uint64_t> alive_cells;
std::unordered_set<uint64_t> cells_to_add;
std::unordered_set<uint64_t> cells_to_delete;
std::unordered_map<uint64_t, int> neighbours;

bool paused = true;
bool left_mouse_pressed = false;
bool right_mouse_pressed = false;

int iterations_per_tick = 1;

float speeds[9]     = {1, 0.5, 0.25, 0.1, 0.05, 0.01, 0.005, 0.002, 0.0};
float iterations[9] = {1,   1,    1,   1,    1,    1,     1,     1,   1};


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

        case sf::Keyboard::Num0:
            iterations_per_tick = 9;
            setTimer(0);
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
            int speed_idx = key_code - sf::Keyboard::Num1;
            iterations_per_tick = iterations[speed_idx];
            setTimer(speeds[speed_idx]);
            break;
    }
}

void Grid::onKeyReleaseEvent(int key_code) {
}

void addCell(int cell_x, int cell_y) {
    change_list.push_back({cell_x, cell_y});

    // when a new cell is introduced,
    // all of the surrounding cells gain a neighbour
    neighbours[((uint64_t)(cell_x - 1) << 32 | (uint32_t)(cell_y - 1))]++;
    neighbours[((uint64_t)(cell_x    ) << 32 | (uint32_t)(cell_y - 1))]++;
    neighbours[((uint64_t)(cell_x + 1) << 32 | (uint32_t)(cell_y - 1))]++;

    neighbours[((uint64_t)(cell_x - 1) << 32 | (uint32_t)(cell_y    ))]++;
    neighbours[((uint64_t)(cell_x + 1) << 32 | (uint32_t)(cell_y    ))]++;

    neighbours[((uint64_t)(cell_x - 1) << 32 | (uint32_t)(cell_y + 1))]++;
    neighbours[((uint64_t)(cell_x    ) << 32 | (uint32_t)(cell_y + 1))]++;
    neighbours[((uint64_t)(cell_x + 1) << 32 | (uint32_t)(cell_y + 1))]++;
}

void deleteCell(int cell_x, int cell_y) {
    change_list.push_back({cell_x, cell_y});

    // when a cell is removed,
    // all of the surrounding cells lose a neighbour
    neighbours[((uint64_t)(cell_x - 1) << 32 | (uint32_t)(cell_y - 1))]--;
    neighbours[((uint64_t)(cell_x    ) << 32 | (uint32_t)(cell_y - 1))]--;
    neighbours[((uint64_t)(cell_x + 1) << 32 | (uint32_t)(cell_y - 1))]--;

    neighbours[((uint64_t)(cell_x - 1) << 32 | (uint32_t)(cell_y    ))]--;
    neighbours[((uint64_t)(cell_x + 1) << 32 | (uint32_t)(cell_y    ))]--;

    neighbours[((uint64_t)(cell_x - 1) << 32 | (uint32_t)(cell_y + 1))]--;
    neighbours[((uint64_t)(cell_x    ) << 32 | (uint32_t)(cell_y + 1))]--;
    neighbours[((uint64_t)(cell_x + 1) << 32 | (uint32_t)(cell_y + 1))]--;
}

void doIteration() {
    cells_to_delete.clear();
    cells_to_add.clear();

    for (auto it: change_list) {
        int cell_x = it.first;
        int cell_y = it.second;
        for (int x = cell_x - 1; x < cell_x + 2; x++) {
            for (int y = cell_y - 1; y < cell_y + 2; y++) {
                uint64_t idx = (uint64_t)x << 32 | (uint32_t)y;

                // alive cell without 2 or 3 neighbours = die
                if (alive_cells.find(idx) != alive_cells.end()) {
                    int n_neighbours = neighbours[idx];
                    if (n_neighbours < 2 || n_neighbours > 3)
                        cells_to_delete.insert(idx);
                }

                // dead cell with 3 neighbours = born
                else if (neighbours[idx] == 3)
                    cells_to_add.insert(idx);
            }
        }
    }
}

void Grid::onTimerEvent() {
    if (paused) return;

    for (int i = 0; i < iterations_per_tick; i++) {
        cells_to_add.clear();
        cells_to_delete.clear();
        doIteration();
        change_list.clear();

        auto add_it = cells_to_add.begin();
        auto add_end = cells_to_add.end();

        auto del_it = cells_to_delete.begin();
        auto del_end = cells_to_delete.end();

        int x, y;

        while (add_it != add_end && del_it != del_end) {
            x = (*add_it) >> 32;
            y = *add_it;
            alive_cells.insert(*add_it);
            addCell(x, y);
            threadDrawCell(x, y, _foreground_color);

            x = (*del_it) >> 32;
            y = *del_it;
            alive_cells.erase(*del_it);
            deleteCell(x, y);
            threadDrawCell(x, y, _background_color);

            add_it++;
            del_it++;
        }

        while (add_it != add_end) {
            x = (*add_it) >> 32;
            y = *add_it;
            alive_cells.insert(*add_it);
            addCell(x, y);
            threadDrawCell(x, y, _foreground_color);

            add_it++;
        }

        while (del_it != del_end) {
            x = (*del_it) >> 32;
            y = *del_it;
            alive_cells.erase(*del_it);
            deleteCell(x, y);
            threadDrawCell(x, y, _background_color);

            del_it++;
        }
    }
}

void Grid::onMouseDragEvent(int x, int y) {
    if (!paused) return;

    uint64_t idx = (uint64_t)x << 32 | (uint32_t)y;
    bool alive = alive_cells.find(idx) != alive_cells.end();

    if (left_mouse_pressed) {
        if (!alive) {
            addCell(x, y);
            alive_cells.insert(idx);
            drawCell(x, y, _foreground_color, 0.1);
        }
    }
    else if (right_mouse_pressed) {
        if (alive) {
            deleteCell(x, y);
            alive_cells.erase(idx);
            drawCell(x, y, _background_color, 0.1);
        }
    }
}

void Grid::onMousePressEvent(int x, int y, int button) {
    uint64_t idx = (uint64_t)x << 32 | (uint32_t)y;
    bool alive = alive_cells.find(idx) != alive_cells.end();

    if (button == sf::Mouse::Left) {
        left_mouse_pressed = true;
        if (paused && !alive) {
            addCell(x, y);
            alive_cells.insert(idx);
            drawCell(x, y, _foreground_color, 0.1);
        }
    }

    else if (button == sf::Mouse::Right) {
        right_mouse_pressed = true;
        if (paused && alive) {
            deleteCell(x, y);
            alive_cells.erase(idx);
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
    int speed_idx = 3;
    iterations_per_tick = iterations[speed_idx];
    setTimer(speeds[speed_idx]);
}

int main() {
    Grid grid("Grid", 20, 20, 40);
    // grid.setGridThickness(6);
    // grid.useAntialiasing(false);
    // grid.setGridlinesFade(0, 0);
    // grid.setGridlineAlpha(50);
    return grid.start();
}
