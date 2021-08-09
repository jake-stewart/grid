#include <SFML/Graphics.hpp>
#include <iostream>
#include <unordered_map>
#include <math.h>
#include "grid.h"

#define ADD_VERTEX(x, y, color) _vertex_array.append(sf::Vertex({x, y}, color))

using std::unordered_map;



void Grid::render() {
    _grid_texture.display();
    _grid_sprite.setScale(_scale, _scale);

    _grid_sprite.setTextureRect({
        _cam_x % _grid_texture_width,
        _cam_y % _grid_texture_height,
        _n_visible_cols,
        _n_visible_rows 
    });

    _blit_x_offset = (-1 + (1 - _cam_x_decimal)) * _scale;
    _blit_y_offset = (-1 + (1 - _cam_y_decimal)) * _scale;

    sf::Rect<int> rect = _grid_sprite.getTextureRect();

    _grid_sprite.setPosition(_blit_x_offset, _blit_y_offset);

    if (_antialias_enabled) {
        _shader.setUniform("scale", _scale);
        _window.draw(_grid_sprite, &_shader);
    }

    else _window.draw(_grid_sprite);
}

void Grid::drawIntroducedCells() {
    int old_col_start = _col_start;
    int old_col_end = _col_end;
    int old_row_start = _row_start;
    int old_row_end = _row_end;

    // 1 is added, since half of two cells could be visible
    _n_visible_cols = ceil(_screen_width / _scale) + 1;
    _n_visible_rows = ceil(_screen_height / _scale) + 1;

    _col_start = _cam_x;
    _row_start = _cam_y;
    _col_end = _col_start + _n_visible_cols;
    _row_end = _row_start + _n_visible_rows;

    int n_cols_drawn_right = _col_end - old_col_end;
    int n_rows_drawn_bottom = _row_end - old_row_end;
    int n_cols_drawn_left = old_col_start - _col_start;
    int n_rows_drawn_top = old_row_start - _row_start;

    bool draw_entire_screen = (
        abs(n_cols_drawn_left) > _n_visible_cols ||
        abs(n_rows_drawn_top) > _n_visible_rows
    );


    if (draw_entire_screen) {
        clearArea(
            _cam_x - 1, _cam_y - 1,
            _n_visible_cols + 2, _n_visible_rows + 2
        );
        drawColumns(_cam_x, _n_visible_cols);
        return;
    }

    if (n_cols_drawn_left > 0) {
        clearArea(
            _col_start - 1, _cam_y - 1,
            n_cols_drawn_left + 1, _n_visible_rows + 2
        );
        drawColumns(_col_start, n_cols_drawn_left);
    }
    else
        n_cols_drawn_left = 0;

    if (n_cols_drawn_right > 0) {
        clearArea(
            old_col_end, _cam_y - 1,
            n_cols_drawn_right + 1, _n_visible_rows + 2
        );
        drawColumns(old_col_end, n_cols_drawn_right);
    }
    else
        n_cols_drawn_right = 0;

    int x = _col_start + n_cols_drawn_left;
    int n_cols = _n_visible_cols - n_cols_drawn_left - n_cols_drawn_right;

    if (n_rows_drawn_top > 0) {
        clearArea(
            x - 1, _row_start -1,
            n_cols + 2, n_rows_drawn_top + 1
        );
        drawRows(
            _row_start, n_rows_drawn_top,
            x, n_cols
        );
    }

    if (n_rows_drawn_bottom > 0) {
        clearArea(
            x - 1, old_row_end,
            n_cols + 2, n_rows_drawn_bottom + 1
        );
        drawRows(
            old_row_end, n_rows_drawn_bottom,
            x, n_cols
        );
    }
}

void Grid::drawRows(int y, int n_rows, int x, int n_cols) {
    int chunk_start_idx = x / _chunk_size;
    int chunk_end_idx = (x + n_cols) / _chunk_size + 1;

    for (int i = 0; i < n_rows; i++) {
        auto row_iter = _rows.find(y);

        float cell_y = y % _grid_texture_height + 0.5;
        if (cell_y < 0) cell_y += _grid_texture_height;

        if (row_iter != _rows.end()) {
            for (int chunk_idx = chunk_start_idx; chunk_idx != chunk_end_idx; chunk_idx++) {
                if (chunk_idx == _max_chunk) {
                    chunk_idx = -_max_chunk;
                }

                auto chunk_iter = row_iter->second.find(chunk_idx);
                if (chunk_iter == row_iter->second.end())
                    continue;

                for (auto& cell: chunk_iter->second) {
                    if (_col_start > _col_end) {
                        int a = cell.first + _n_visible_cols * 2;
                        int b = x + _n_visible_cols * 2;
                        int c = b + n_cols;
                        if (a < b || a >= c)
                            continue;
                    }
                    else if (cell.first < x || cell.first >= x + n_cols)
                        continue;

                    float cell_x = cell.first % _grid_texture_width + 0.5;
                    if (cell_x < 0) cell_x += _grid_texture_width;
                    ADD_VERTEX(cell_x, cell_y, cell.second);
                }
            }
        }
        y++;
    }
}

void Grid::drawColumns(int x, int n_cols) {
    int chunk_start_idx = _cam_y / _chunk_size;
    int chunk_end_idx = (_cam_y + _n_visible_rows) / _chunk_size + 1;

    for (int i = 0; i < n_cols; i++) {
        auto col_iter = _columns.find(x);

        float cell_x = x % _grid_texture_width + 0.5;
        if (cell_x < 0) cell_x += _grid_texture_width;

        if (col_iter != _columns.end()) {
            for (int chunk_idx = chunk_start_idx; chunk_idx != chunk_end_idx; chunk_idx++) {
                if (chunk_idx == _max_chunk) chunk_idx = -_max_chunk;

                auto chunk_iter = col_iter->second.find(chunk_idx);
                if (chunk_iter == col_iter->second.end())
                    continue;

                for (auto& cell: chunk_iter->second) {
                    if (_row_start > _row_end) {
                        int a = cell.first + _n_visible_rows * 2;
                        int b = _cam_y + _n_visible_rows * 2;
                        int c = b + _n_visible_rows;
                        if (a < b || a >= c) {
                            continue;
                        }
                    }
                    else {
                        if (cell.first < _cam_y || cell.first >= _cam_y + _n_visible_rows) {
                            continue;
                        }
                    }
                    float cell_y = cell.first % _grid_texture_height + 0.5;
                    if (cell_y < 0) cell_y += _grid_texture_height;
                    ADD_VERTEX(cell_x, cell_y, cell.second);
                }
            }
        }
        x++;
    }
}


void Grid::clearArea(int x, int y, int n_cols, int n_rows) {
    x %= _grid_texture_width;
    if (x < 0) x += _grid_texture_width;

    y %= _grid_texture_height;
    if (y < 0) y += _grid_texture_height;

    sf::RectangleShape rect;
    rect.setFillColor(_background_color);

    int start_y = y;
    int start_n_rows = n_rows;

    while (n_cols) {
        float cols_fit;
        if (x + n_cols > _grid_texture_width)
            cols_fit = _grid_texture_width - x;
        else
            cols_fit = n_cols;

        y = start_y;
        n_rows = start_n_rows;
        while (n_rows) {
            float rows_fit;
            if (y + n_rows > _grid_texture_height)
                rows_fit = _grid_texture_height - y;
            else
                rows_fit = n_rows;

            rect.setPosition(x, y);
            rect.setSize({cols_fit, rows_fit});
            _grid_texture.draw(rect);

            n_rows -= rows_fit;
            y = 0;
        }

        n_cols -= cols_fit;
        x = 0;
    }
}
