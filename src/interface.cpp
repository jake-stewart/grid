#include <SFML/Graphics.hpp>
#include "grid.h"

#define ADD_VERTEX(x, y, color) _vertex_array.append(sf::Vertex({x, y}, color))

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
}

void Grid::setGridlineAlpha(int alpha) {
    _grid_default_max_alpha = alpha;
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
}

void Grid::setGridThickness(float value) {
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

    onStartEvent();
    startThread();
    mainloop();
    return 0;
}

sf::Color Grid::getCell(int x, int y) {
    auto col_iter = _columns.find(x);

    if (col_iter != _columns.end()) {
        auto chunk_iter = col_iter->second.find(y / _chunk_size);

        if (chunk_iter != col_iter->second.end()) {
            auto cell_iter = chunk_iter->second.find(y);

            if (cell_iter != chunk_iter->second.end())
                return cell_iter->second;
        }
    }

    return _background_color;
}

void Grid::drawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g) {
    drawCell(x, y, sf::Color{r, g, b});
}

void Grid::drawCell(int x, int y, sf::Color color) {
    _columns[x][y / _chunk_size][y] = color;
    _rows[y][x / _chunk_size][x] = color;

    if (_col_start > _col_end) {
        int a = x + _n_visible_cols * 2;
        int b = _cam_x + _n_visible_cols * 2;
        int c = b + _n_visible_cols;
        if (a < b || a >= c)
            return;
    }
    else if (x < _cam_x || x >= _cam_x + _n_visible_cols)
        return;

    if (_row_start > _row_end) {
        int a = y + _n_visible_rows * 2;
        int b = _cam_y + _n_visible_rows * 2;
        int c = b + _n_visible_rows;
        if (a < b || a >= c) {
            return;
        }
    }
    else if (y < _cam_y || y >= _cam_y + _n_visible_rows)
        return;

    float blit_x = (x % _grid_texture_width) + 0.5;
    if (blit_x < 0) blit_x += _grid_texture_width;

    float blit_y = (y % _grid_texture_height) + 0.5;
    if (blit_y < 0) blit_y += _grid_texture_height;

    ADD_VERTEX(blit_x, blit_y, color);
}


void Grid::threadDrawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g) {
    drawCell(x, y, sf::Color{r, g, b});
}

void Grid::threadDrawCell(int x, int y, sf::Color color) {
    _columns[x][y / _chunk_size][y] = color;
    _rows[y][x / _chunk_size][x] = color;

    if (_col_start > _col_end) {
        int a = x + _n_visible_cols * 2;
        int b = _cam_x + _n_visible_cols * 2;
        int c = b + _n_visible_cols;
        if (a < b || a >= c)
            return;
    }
    else if (x < _cam_x || x >= _cam_x + _n_visible_cols)
        return;

    if (_row_start > _row_end) {
        int a = y + _n_visible_rows * 2;
        int b = _cam_y + _n_visible_rows * 2;
        int c = b + _n_visible_rows;
        if (a < b || a >= c) {
            return;
        }
    }
    else if (y < _cam_y || y >= _cam_y + _n_visible_rows)
        return;

    _cell_draw_queue.push_back({x, y, color});
}

void Grid::drawCellQueue() {
    int size = _cell_draw_queue.size();
    for (; _current_queue_idx < size; _current_queue_idx++) {
        if (_col_start > _col_end) {
            int a = _cell_draw_queue[_current_queue_idx].x + _n_visible_cols * 2;
            int b = _cam_x + _n_visible_cols * 2;
            int c = b + _n_visible_cols;
            if (a < b || a >= c)
                continue;
        }
        else if (_cell_draw_queue[_current_queue_idx].x < _cam_x || _cell_draw_queue[_current_queue_idx].x >= _cam_x + _n_visible_cols)
            continue;

        if (_row_start > _row_end) {
            int a = _cell_draw_queue[_current_queue_idx].y + _n_visible_rows * 2;
            int b = _cam_y + _n_visible_rows * 2;
            int c = b + _n_visible_rows;
            if (a < b || a >= c) {
                continue;
            }
        }
        else if (_cell_draw_queue[_current_queue_idx].y < _cam_y || _cell_draw_queue[_current_queue_idx].y >= _cam_y + _n_visible_rows)
            continue;

        float blit_x = (_cell_draw_queue[_current_queue_idx].x % _grid_texture_width) + 0.5;
        if (blit_x < 0) blit_x += _grid_texture_width;

        float blit_y = (_cell_draw_queue[_current_queue_idx].y % _grid_texture_height) + 0.5;
        if (blit_y < 0) blit_y += _grid_texture_height;

        ADD_VERTEX(blit_x, blit_y, _cell_draw_queue[_current_queue_idx].color);
    }
}

