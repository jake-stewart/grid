#include "grid.h"
#include <iostream>
#include "robin_hood.h"
#include <vector>


// yes i know this needs to be cleaned up


robin_hood::unordered_set<uint64_t> alive_cells;
std::vector<uint64_t> cells_to_add;
std::vector<uint64_t> cells_to_delete;
robin_hood::unordered_map<uint64_t, int> neighbours;

bool paused = true;
bool left_mouse_pressed = false;
bool right_mouse_pressed = false;

float speeds[5] = {1, 0.25, 0.05, 0.01, 0.001};

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
            int speed_idx = key_code - sf::Keyboard::Num1;
            setTimer(speeds[speed_idx]);
            break;
    }
}

void Grid::onKeyReleaseEvent(int key_code) {
}

const uint64_t DY = 0x100000000;
const uint64_t DX = 0x1;

inline void addCell(uint64_t idx) {
    // when a new cell is introduced,
    // all of the surrounding cells gain a neighbour
    neighbours[idx - DY - DX]++;
    neighbours[idx - DY     ]++;
    neighbours[idx - DY + DX]++;
    neighbours[idx      - DX]++;
    neighbours[idx      + DX]++;
    neighbours[idx + DY - DX]++;
    neighbours[idx + DY     ]++;
    neighbours[idx + DY + DX]++;

    alive_cells.insert(idx);
}

inline void deleteCell(uint64_t idx) {
    // when a cell is removed,
    // all of the surrounding cells lose a neighbour
    if (!neighbours[idx - DY - DX]--) neighbours.erase(idx - DY - DX);
    if (!neighbours[idx - DY     ]--) neighbours.erase(idx - DY     );
    if (!neighbours[idx - DY + DX]--) neighbours.erase(idx - DY + DX);
    if (!neighbours[idx      - DX]--) neighbours.erase(idx      - DX);
    if (!neighbours[idx      + DX]--) neighbours.erase(idx      + DX);
    if (!neighbours[idx + DY - DX]--) neighbours.erase(idx + DY - DX);
    if (!neighbours[idx + DY     ]--) neighbours.erase(idx + DY     );
    if (!neighbours[idx + DY + DX]--) neighbours.erase(idx + DY + DX);
    alive_cells.erase(idx);
}

void Grid::onTimerEvent(int n_iterations) {
    if (paused) return;

    for (int i = 0; i < n_iterations; i++) {
        cells_to_add.clear();
        cells_to_delete.clear();

        for (auto it: alive_cells) {
            auto n = neighbours.find(it);
            if (n == neighbours.end() || n->second < 2 || n->second > 3)
                cells_to_delete.push_back(it);
        }

        for (auto it: neighbours) {
            if (it.second == 3 && alive_cells.find(it.first) == alive_cells.end())
                cells_to_add.push_back(it.first);
        }

        for (auto it: cells_to_add) {
            addCell(it);
            threadDrawCell(it >> 32, it, _foreground_color);
        }

        for (auto it: cells_to_delete) {
            deleteCell(it);
            threadDrawCell(it >> 32, it, _background_color);
        }

        if (_timer.getElapsedTime().asSeconds() > _frame_duration * 3)
            break;
    }
}

void Grid::onMouseDragEvent(int x, int y) {
    if (!paused) return;

    uint64_t idx = (uint64_t)x << 32 | (uint32_t)y;
    bool alive = alive_cells.find(idx) != alive_cells.end();

    if (left_mouse_pressed) {
        if (!alive) {
            addCell(idx);
            alive_cells.insert(idx);
            drawCell(x, y, _foreground_color, 0.1);
        }
    }
    else if (right_mouse_pressed) {
        if (alive) {
            deleteCell(idx);
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
            addCell(idx);
            alive_cells.insert(idx);
            drawCell(x, y, _foreground_color, 0.1);
        }
    }

    else if (button == sf::Mouse::Right) {
        right_mouse_pressed = true;
        if (paused && alive) {
            deleteCell(idx);
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
    addText(8,  9, "GRID", _foreground_color, 0);

    // for (int i = 0; i < 1000; i++) {
    //     uint64_t idx = (uint64_t)i << 32;
    //     drawCell(i, 0, _foreground_color);
    //     addCell(idx);
    // }

    int speed_idx = 2;
    setTimer(speeds[speed_idx]);
}

int main() {
    Grid grid("Grid", 20, 19, 40);
    // grid.setGridThickness(6);
    // grid.useAntialiasing(false);
    // grid.setGridlinesFade(0, 0);
    // grid.setGridlineAlpha(50);
    return grid.start();
}
