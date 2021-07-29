#include <SFML/Graphics.hpp>
#include "grid.h"

void Grid::setScale(float scale) {
    // calculate how many cells would be visible at the new scale
    float n_cells_x = _screen_width / scale;
    float n_cells_y = _screen_height / scale;

    // if grid texture is not large enough to support this many cells at this scale
    if (n_cells_x > _max_cells_x || n_cells_y > _max_cells_y) {
        float min_scale_x = (float)_screen_width / _max_cells_x;
        float min_scale_y = (float)_screen_height / _max_cells_y;

        // then set the scale to be as small as possible
        // and stop zooming, since there is no point continuing to zoom out
        _scale = (min_scale_x > min_scale_y) ? min_scale_x : min_scale_y;
        _zoom_vel = 0;
    }
    else {
        // otherwise, set the scale to the desired value
        _scale = scale;
    }

    applyGridEffects();
}

void Grid::setGridlinesFade(float start_scale, float end_scale) {
    // sets at what cell scale gridlines should begin and end fading at

    // fade_start must be less than fade_end
    // if it's not, switch them around
    if (start_scale > end_scale) {
        _fade_start_scale = end_scale;
        _fade_end_scale = start_scale;
    }
    else {
        _fade_start_scale = start_scale;
        _fade_end_scale = end_scale;
    }

    _screen_changed = true;
}

void Grid::setGridlineAlpha(int alpha) {
    _grid_default_max_alpha = alpha;
    _screen_changed = true;
}

void Grid::toggleGridlines() {
    if (_grid_fading == 1)
        _grid_fading = -1;
    else if (_grid_fading == -1)
        _grid_fading = 1;
    else if (_display_grid)
        _grid_fading = -1;
    else
        _grid_fading = 1;
}

void Grid::useAntialiasing(bool value) {
    _antialias_enabled = value;
    _grid_texture.setSmooth(_antialias_enabled);
    _screen_changed = true;
}

void Grid::setGridThickness(float value) {
    _screen_changed = true;

    // value <= 0 will disable gridlines entirely
    if (value <= 0) {
        _grid_thickness = 0;
        _cell_gridline_ratio = 0;
    }

    else {
       // 0 < value < 1 will set gridline thickness to be a ratio of a cell
       // for example 0.1 will mean gridlines account for 10% of a cell
        if (value >= 1) {
            _cell_gridline_ratio = value;
            _gridline_shrink = false;
        }

        // value >= 1 will set gridline thickness to that value
        else {
            _cell_gridline_ratio = value;
            _gridline_shrink = true;
        }

        applyGridEffects();
    }
}

int Grid::start() {
    int err_code = initialize();
    if (err_code != 0)
        return err_code;

    onStart();
    mainloop();
    return 0;
}

sf::Color Grid::getCell(int x, int y) {
    std::unordered_map<int, std::unordered_map<int, std::unordered_map
        <int, sf::Uint8[4]>>>::const_iterator
    col_iter = _columns.find(x);

    if (col_iter != _columns.end()) {
        std::unordered_map<int, std::unordered_map
            <int, sf::Uint8[4]>>::const_iterator
        chunk_iter = col_iter->second.find(y / _chunk_size);

        if (chunk_iter != col_iter->second.end()) {
            std::unordered_map<int, sf::Uint8[4]>::const_iterator
            cell_iter = chunk_iter->second.find(y);

            if (cell_iter != chunk_iter->second.end()) {
                return sf::Color{
                    cell_iter->second[0],
                    cell_iter->second[1],
                    cell_iter->second[2]
                };
            }
        }
    }

    return _background_color;
}

void Grid::drawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g) {
    int chunk_x = x / _chunk_size;
    int chunk_y = y / _chunk_size;

    sf::Uint8 * cell;
    cell = _columns[x][chunk_y][y];
    cell[0] = r;
    cell[1] = g;
    cell[2] = b;
    cell[3] = 255;

    cell = _rows[y][chunk_x][x];
    cell[0] = r;
    cell[1] = g;
    cell[2] = b;
    cell[3] = 255;

    if (_col_start <= x && x <= _col_end && _row_start <= y && y <= _row_end) {
        int blit_x = x % _grid_texture_width;
        if (blit_x < 0) blit_x += _grid_texture_width;

        int blit_y = y % _grid_texture_height;
        if (blit_y < 0) blit_y += _grid_texture_height;

        _grid_texture.update(&cell[0], 1, 1, blit_x, blit_y);
        _screen_changed = true;
    }
}

void Grid::drawCell(int x, int y, sf::Color color) {
    int chunk_x = x / _chunk_size;
    int chunk_y = y / _chunk_size;

    sf::Uint8 * cell;
    cell = _columns[x][chunk_y][y];
    cell[0] = color.r;
    cell[1] = color.g;
    cell[2] = color.b;
    cell[3] = 255;

    cell = _rows[y][chunk_x][x];
    cell[0] = color.r;
    cell[1] = color.g;
    cell[2] = color.b;
    cell[3] = 255;

    if (_col_start <= x && x <= _col_end && _row_start <= y && y <= _row_end) {
        int blit_x = x % _grid_texture_width;
        if (blit_x < 0) blit_x += _grid_texture_width;

        int blit_y = y % _grid_texture_height;
        if (blit_y < 0) blit_y += _grid_texture_height;

        _grid_texture.update(&cell[0], 1, 1, blit_x, blit_y);
        _screen_changed = true;
    }
}
