#include <SFML/Graphics.hpp>
#include <iostream>
#include <unordered_map>
#include <math.h>
#include "grid.h"

#define ADD_VERTEX(x, y, color) _vertex_array.append(sf::Vertex({x, y}, color))

using std::unordered_map;


void Grid::calcGridSize() {
    _col_start = _cam_x;
    _row_start = _cam_y;

    // 1 is added, since half of two cells could be visible
    _n_visible_cols = ceil(_screen_width / _scale) + 1;
    _n_visible_rows = ceil(_screen_height / _scale) + 1;

    _col_end = _col_start + _n_visible_cols;
    _row_end = _row_start + _n_visible_rows;
}

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
    // by comparing the old visible grid position and size to the new size,
    // we are able to calculate how many new rows/columns are visible
    // this way, instead of drawing every cell, only the introduced cells
    // have to be drawn

    // rows and columns intersect in the corners
    // to avoid drawing cells twice, we keep track of how many columns
    // are drawn, and skip them when drawing rows
    int n_cols_drawn_left = 0;
    int n_cols_drawn_right = 0;

    // store old visible grid
    int old_col_start = _col_start;
    int old_col_end = _col_end;
    int old_row_start = _row_start;
    int old_row_end = _row_end;

    // calculate new visible grid
    calcGridSize();

    // draw new columns introduced on the left
    if (_col_start < old_col_start) {
        n_cols_drawn_left = old_col_start - _col_start;
        drawColumns(
            _col_start,
            n_cols_drawn_left
        );
    }

    // draw new columns introduced on the right
    if (old_col_end < _col_end) {
        n_cols_drawn_right = _col_end - old_col_end;
        drawColumns(
            old_col_end,
            n_cols_drawn_right
        );
    }

    // draw new rows introduced on the top
    if (_row_start < old_row_start)
        drawRows(
            _row_start,
            old_row_start - _row_start,
            _col_start + n_cols_drawn_left,
            _n_visible_cols - n_cols_drawn_left - n_cols_drawn_right
        );

    // draw new rows introduced on the bottom
    if (old_row_end < _row_end)
        drawRows(
            old_row_end,
            _row_end - old_row_end,
            _col_start + n_cols_drawn_left,
            _n_visible_cols - n_cols_drawn_left - n_cols_drawn_right
        );
}

void Grid::drawRows(int y, int n_rows, int x, int n_cols) {
    clearArea(x, y, n_cols, n_rows);

    int chunk_start_idx = x / _chunk_size;
    int chunk_end_idx = chunk_start_idx + ceil(n_cols / (float)_chunk_size) + 1;

    for (int y_idx = y; y_idx < y + n_rows; y_idx++) {
        auto row_iter = _rows.find(y_idx);

        float cell_y = y_idx % _grid_texture_height + 0.5;
        if (cell_y < 0) cell_y += _grid_texture_height;

        if (row_iter != _rows.end()) {
            for (int chunk_idx = chunk_start_idx; chunk_idx < chunk_end_idx; chunk_idx++) {
                auto chunk_iter = row_iter->second.find(chunk_idx);
                if (chunk_iter == row_iter->second.end())
                    continue;

                for (auto& cell: chunk_iter->second) {
                    if (cell.first < x || cell.first >= x + n_cols)
                        continue;

                    float cell_x = cell.first % _grid_texture_width + 0.5;
                    if (cell_x < 0) cell_x += _grid_texture_width;

                    ADD_VERTEX(cell_x, cell_y, cell.second);
                }
            }
        }
    }
}

void Grid::drawColumns(int x, int n_cols) {
    clearArea(x, _cam_y, n_cols, _n_visible_rows);

    int chunk_start_idx = _cam_y / _chunk_size;
    int chunk_end_idx = chunk_start_idx + ceil(_n_visible_rows / (float)_chunk_size) + 1;

    for (int x_idx = x; x_idx < x + n_cols; x_idx++) {
        auto col_iter = _columns.find(x_idx);

        float cell_x = x_idx % _grid_texture_width + 0.5;
        if (cell_x < 0) cell_x += _grid_texture_width;

        if (col_iter != _columns.end()) {
            for (int chunk_idx = chunk_start_idx; chunk_idx < chunk_end_idx; chunk_idx++) {
                auto chunk_iter = col_iter->second.find(chunk_idx);
                if (chunk_iter == col_iter->second.end())
                    continue;

                for (auto& cell: chunk_iter->second) {
                    if (cell.first < _cam_y || cell.first >= _cam_y + _n_visible_rows)
                        continue;

                    float cell_y = cell.first % _grid_texture_height + 0.5;
                    if (cell_y < 0) cell_y += _grid_texture_height;

                    ADD_VERTEX(cell_x, cell_y, cell.second);
                }
            }
        }
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
