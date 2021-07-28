#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <math.h>
#include "grid.h"

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
    _grid_sprite.setScale(_scale, _scale);

    _grid_sprite.setTextureRect({
        _cam_x % _grid_texture_width,
        _cam_y % _grid_texture_height,
        _n_visible_cols,
        _n_visible_rows
    });

    _blit_x_offset = (-1 + (1 - _cam_x_decimal)) * _scale;
    _blit_y_offset = (-1 + (1 - _cam_y_decimal)) * _scale;
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
    // calculate where on the grid texture the columns will be drawn
    int blit_y = y % _grid_texture_height;
    if (blit_y < 0) blit_y += _grid_texture_height;

    int blit_x = x % _grid_texture_width;
    if (blit_x < 0) blit_x += _grid_texture_width;

    // if the columns extend beyond the grid texture, then the
    // extra columns will be drawn at the start of the texture
    if (blit_x + n_cols > _grid_texture_width) {
        int excess_cols = (blit_x + n_cols) - _grid_texture_width;
        n_cols -= excess_cols;
        drawRows(y, n_rows, x + n_cols, excess_cols);
    }

    // the pixels, which will be used to draw cells before being sent
    // to the graphics card must be cleared to be the background color
    clearPixels(n_rows, n_cols);

    unordered_map<int, unordered_map<int, unordered_map
        <int, sf::Uint8[4]>>>::const_iterator row_iter;

    unordered_map<int, unordered_map<int, sf::Uint8[4]>>
        ::const_iterator chunk_iter;

    int chunk_start_idx = x / _chunk_size;
    int chunk_end_idx = chunk_start_idx + ceil(n_cols / (float)_chunk_size) + 1;
    int chunk_idx;

    // variables used to calculate where in the pixel array a cell belongs
    int cell_idx;
    int y_offset = 0;
    int row_size = n_cols * 4;

    // for every row, iterate over every visible chunk within that row
    // for every chunk, iterate over every cell contained within that chunk
    // for every cell, add it to the pixel array
    for (int y_idx = y; y_idx < y + n_rows; y_idx++) {
        row_iter = _rows.find(y_idx);

        if (row_iter != _rows.end()) {
            for (chunk_idx = chunk_start_idx; chunk_idx < chunk_end_idx; chunk_idx++) {
                chunk_iter = row_iter->second.find(chunk_idx);
                if (chunk_iter == row_iter->second.end())
                    continue;

                for (auto& cell: chunk_iter->second) {
                    cell_idx = cell.first - x;
                    if (cell_idx < 0 || cell_idx >= n_cols)
                        continue;

                    cell_idx = cell_idx * 4 + y_offset;
                    _pixels[cell_idx] = cell.second[0];
                    _pixels[cell_idx + 1] = cell.second[1];
                    _pixels[cell_idx + 2] = cell.second[2];
                }
            }
        }
        y_offset += row_size;
    }

    // use generated pixels to update the grid texture
    updateTexture(blit_x, blit_y, n_rows, n_cols);
}

void Grid::drawColumns(int x, int n_cols) {
    // calculate where on the grid texture the columns will be drawn
    int blit_x = x % _grid_texture_width;
    if (blit_x < 0) blit_x += _grid_texture_width;

    int blit_y = _cam_y % _grid_texture_height;
    if (blit_y < 0) blit_y += _grid_texture_height;

    // if the columns extend beyond the grid texture, then the
    // extra columns will be drawn at the start of the texture
    if (blit_x + n_cols > _grid_texture_width) {
        int excess_cols = (blit_x + n_cols) - _grid_texture_width;
        n_cols -= excess_cols;
        drawColumns(x + n_cols, excess_cols);
    }

    // the pixels, which will be used to draw cells before being sent
    // to the graphics card must be cleared to be the background color
    clearPixels(n_cols, _n_visible_rows);

    unordered_map<int, unordered_map<int, unordered_map
        <int, sf::Uint8[4]>>>::const_iterator col_iter;

    unordered_map<int, unordered_map<int, sf::Uint8[4]>>
        ::const_iterator chunk_iter;

    int chunk_start_idx = _cam_y / _chunk_size;
    int chunk_end_idx = chunk_start_idx + ceil(_n_visible_rows / (float)_chunk_size) + 1;
    int chunk_idx;

    // variables used to calculate where in the pixel array a cell belongs
    int cell_idx;
    int x_offset = 0;
    int row_size = n_cols * 4;

    // for every column, iterate over every visible chunk within that column
    // for every chunk, iterate over every cell contained within that chunk
    // for every cell, add it to the pixel array
    for (int x_idx = x; x_idx < x + n_cols; x_idx++) {
        col_iter = _columns.find(x_idx);

        if (col_iter != _columns.end()) {
            for (chunk_idx = chunk_start_idx; chunk_idx < chunk_end_idx; chunk_idx++) {
                chunk_iter = col_iter->second.find(chunk_idx);
                if (chunk_iter == col_iter->second.end())
                    continue;

                for (auto& cell: chunk_iter->second) {
                    cell_idx = (cell.first - _cam_y);
                    if (cell_idx < 0 || cell_idx >= _n_visible_rows) continue;
                    cell_idx = cell_idx * row_size + x_offset;
                    _pixels[cell_idx] = cell.second[0];
                    _pixels[cell_idx + 1] = cell.second[1];
                    _pixels[cell_idx + 2] = cell.second[2];
                }
            }
        }
        x_offset += 4;
    }

    // use generated pixels to update the grid texture
    updateTexture(blit_x, blit_y, _n_visible_rows, n_cols);
}


void Grid::clearPixels(int n_rows, int n_cols) {
    int pixels_size = n_rows * n_cols * 4;

    for (int i = 0; i < pixels_size; i += 4) {
        _pixels[i] = _background_color.r;
        _pixels[i + 1] = _background_color.g;
        _pixels[i + 2] = _background_color.b;
    }
}

void Grid::updateTexture(int blit_x, int blit_y, int n_rows, int n_cols) {
    // if the pixels extend beyond the grid texture's height,
    // then the excess pixels will need to be wrapped to the start
    // of the texture
    if (blit_y + n_rows > _grid_texture_height) {
        int n = n_rows - ((blit_y + n_rows) - _grid_texture_height);
        _grid_texture.update(_pixels, n_cols, n, blit_x, blit_y);
        _grid_texture.update(
            &_pixels[n * n_cols * 4],
            n_cols, n_rows - n, blit_x, 0
        );
    }
    else {
        // otherwise, the pixels can be updated all at once
        _grid_texture.update(_pixels, n_cols, n_rows, blit_x, blit_y);
    }
}
