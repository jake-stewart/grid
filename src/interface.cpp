#include <SFML/Graphics.hpp>
#include <iostream>
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
    _grid_render_texture.setSmooth(_antialias_enabled);
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

    int n_chunks_width = ceil(_screen_width / (_chunk_size * _scale));
    int spare_chunks_x = _render_distance - n_chunks_width;
    if (spare_chunks_x < 0) spare_chunks_x = 0;
    _chunk_render_left = _chunk_x - spare_chunks_x / 2;
    _chunk_render_right = _chunk_render_left + _render_distance;

    int n_chunks_height = ceil(_screen_height / (_chunk_size * _scale));
    int spare_chunks_y = _render_distance - n_chunks_height;
    if (spare_chunks_y < 0) spare_chunks_y = 0;
    _chunk_render_top = _chunk_y - spare_chunks_y / 2;
    _chunk_render_bottom = _chunk_render_top + _render_distance;

    onStartEvent();
    startThread();
    mainloop();
    return 0;
}

sf::Color Grid::getCell(int x, int y) {
    int chunk_idx_x = floor(x / (float)_chunk_size);
    int chunk_idx_y = floor(y / (float)_chunk_size);

    uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;
    auto chunk = _chunks_pointer->find(chunk_idx);

    if (chunk == _chunks_pointer->end())
        return _background_color;

    int pixel_y = y % _chunk_size;
    if (pixel_y < 0) pixel_y += _chunk_size;

    int pixel_x = x % _chunk_size;
    if (pixel_x < 0) pixel_x += _chunk_size;

    int pixel_idx = (pixel_y * _chunk_size + pixel_x) * 4;

    return sf::Color{chunk->second[pixel_idx], chunk->second[pixel_idx + 1], chunk->second[pixel_idx + 2]};
}

void Grid::drawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g) {
    drawCell(x, y, sf::Color{r, g, b});
}

void Grid::drawCell(int x, int y, sf::Color color) {

    int chunk_idx_x = floor(x / (float)_chunk_size);
    int chunk_idx_y = floor(y / (float)_chunk_size);

    uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;

    auto chunk = _chunks_pointer->find(chunk_idx);

    if (chunk == _chunks_pointer->end()) {
        auto pixels = (*_chunks_pointer)[chunk_idx];
        auto pixels_buffer = (*_thread_chunks_pointer)[chunk_idx];
        for (int i = 0; i < _chunk_size * _chunk_size * 4; i += 4) {
            pixels[i] = _background_color.r;
            pixels[i + 1] = _background_color.g;
            pixels[i + 2] = _background_color.b;
            pixels[i + 3] = 255;

            pixels_buffer[i] = _background_color.r;
            pixels_buffer[i + 1] = _background_color.g;
            pixels_buffer[i + 2] = _background_color.b;
            pixels_buffer[i + 3] = 255;
        }
    }

    int pixel_y = y % _chunk_size;
    if (pixel_y < 0) pixel_y += _chunk_size;

    int pixel_x = x % _chunk_size;
    if (pixel_x < 0) pixel_x += _chunk_size;

    int pixel_idx = (pixel_y * _chunk_size + pixel_x) * 4;

    auto pixels = (*_chunks_pointer)[chunk_idx];
    auto pixels_buffer = (*_thread_chunks_pointer)[chunk_idx];

    pixels[pixel_idx] = color.r;
    pixels[pixel_idx + 1] = color.g;
    pixels[pixel_idx + 2] = color.b;
    pixels[pixel_idx + 3] = 255;
    pixels_buffer[pixel_idx] = color.r;
    pixels_buffer[pixel_idx + 1] = color.g;
    pixels_buffer[pixel_idx + 2] = color.b;
    pixels_buffer[pixel_idx + 3] = 255;

    if (chunk_idx_x < _chunk_render_left
            || chunk_idx_x >= _chunk_render_right
            || chunk_idx_y < _chunk_render_top
            || chunk_idx_y >= _chunk_render_bottom)
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
    _cell_draw_queue.push_back({x, y, color});

    int chunk_idx_x = floor(x / (float)_chunk_size);
    int chunk_idx_y = floor(y / (float)_chunk_size);

    uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;
    auto chunk = _thread_chunks_pointer->find(chunk_idx);

    if (chunk == _thread_chunks_pointer->end()) {
        auto pixels_buffer = (*_thread_chunks_pointer)[chunk_idx];
        for (int i = 0; i < _chunk_size * _chunk_size * 4; i += 4) {
            pixels_buffer[i] = _background_color.r;
            pixels_buffer[i + 1] = _background_color.g;
            pixels_buffer[i + 2] = _background_color.b;
            pixels_buffer[i + 3] = 255;
        }
    }

    int pixel_y = y % _chunk_size;
    if (pixel_y < 0) pixel_y += _chunk_size;

    int pixel_x = x % _chunk_size;
    if (pixel_x < 0) pixel_x += _chunk_size;

    int pixel_idx = (pixel_y * _chunk_size + pixel_x) * 4;

    auto pixels_buffer = (*_thread_chunks_pointer)[chunk_idx];
    pixels_buffer[pixel_idx] = color.r;
    pixels_buffer[pixel_idx + 1] = color.g;
    pixels_buffer[pixel_idx + 2] = color.b;
    pixels_buffer[pixel_idx + 3] = 255;
}

void Grid::copyCellDrawQueue() {
    for (auto it: _cell_draw_queue) {
        int chunk_idx_x = floor(it.x / (float)_chunk_size);
        int chunk_idx_y = floor(it.y / (float)_chunk_size);

        uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;
        auto chunk = _thread_chunks_pointer->find(chunk_idx);

        if (chunk == _thread_chunks_pointer->end()) {
            auto pixels_buffer = (*_thread_chunks_pointer)[chunk_idx];
            for (int i = 0; i < _chunk_size * _chunk_size * 4; i += 4) {
                pixels_buffer[i] = _background_color.r;
                pixels_buffer[i + 1] = _background_color.g;
                pixels_buffer[i + 2] = _background_color.b;
                pixels_buffer[i + 3] = 255;
            }
        }

        int pixel_y = it.y % _chunk_size;
        if (pixel_y < 0) pixel_y += _chunk_size;

        int pixel_x = it.x % _chunk_size;
        if (pixel_x < 0) pixel_x += _chunk_size;

        int pixel_idx = (pixel_y * _chunk_size + pixel_x) * 4;

        auto pixels_buffer = (*_thread_chunks_pointer)[chunk_idx];
        pixels_buffer[pixel_idx] = it.color.r;
        pixels_buffer[pixel_idx + 1] = it.color.g;
        pixels_buffer[pixel_idx + 2] = it.color.b;
        pixels_buffer[pixel_idx + 3] = 255;
    }
    _cell_draw_queue.clear();
}

void Grid::drawCellQueue() {
    for (auto it: _cell_draw_queue) {

        int chunk_idx_x = floor(it.x / (float)_chunk_size);
        int chunk_idx_y = floor(it.y / (float)_chunk_size);

        if (chunk_idx_x < _chunk_render_left
                || chunk_idx_x >= _chunk_render_right
                || chunk_idx_y < _chunk_render_top
                || chunk_idx_y >= _chunk_render_bottom)
            return;

        float blit_x = (it.x % _grid_texture_width) + 0.5;
        if (blit_x < 0) blit_x += _grid_texture_width;

        float blit_y = (it.y % _grid_texture_height) + 0.5;
        if (blit_y < 0) blit_y += _grid_texture_height;

        ADD_VERTEX(blit_x, blit_y, it.color);
    }
}
