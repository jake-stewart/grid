#include <SFML/Graphics.hpp>
#include <iostream>
#include <math.h>
#include "grid.h"

#define ADD_VERTEX(x, y, color) _cell_vertexes.append(sf::Vertex({x, y}, color))

void Grid::setScale(float scale) {
	if (scale > _max_scale)
		_scale = _max_scale;
	else if (scale < _min_scale)
		_scale = _min_scale;
    else
        _scale = scale;
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

    int n_chunks_width = ceil(_screen_width / (CHUNK_SIZE * _scale));
    int spare_chunks_x = _render_distance - n_chunks_width;
    if (spare_chunks_x < 0) spare_chunks_x = 0;
    _chunk_render_left = _chunk_x - spare_chunks_x / 2;
    _chunk_render_right = _chunk_render_left + _render_distance;

    int n_chunks_height = ceil(_screen_height / (CHUNK_SIZE * _scale));
    int spare_chunks_y = _render_distance - n_chunks_height;
    if (spare_chunks_y < 0) spare_chunks_y = 0;
    _chunk_render_top = _chunk_y - spare_chunks_y / 2;
    _chunk_render_bottom = _chunk_render_top + _render_distance;

    onStartEvent();
    mainloop();
    return 0;
}

void Grid::setFPS(int fps) {
	if (fps) _frame_duration = 1.0 / fps;
  else _frame_duration = 0;
	_window.setFramerateLimit(fps);
}

sf::Color Grid::getCell(int x, int y) {
    int chunk_idx_x = floor(x / (float)CHUNK_SIZE);
    int chunk_idx_y = floor(y / (float)CHUNK_SIZE);

    uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;
    auto chunk = _chunks[0].find(chunk_idx);

    if (chunk == _chunks[0].end())
        return _background_color;

    int pixel_y = y % CHUNK_SIZE;
    if (pixel_y < 0) pixel_y += CHUNK_SIZE;

    int pixel_x = x % CHUNK_SIZE;
    if (pixel_x < 0) pixel_x += CHUNK_SIZE;

    int pixel_idx = (pixel_y * CHUNK_SIZE + pixel_x) * 4;

    return sf::Color{
        chunk->second.pixels[pixel_idx],
        chunk->second.pixels[pixel_idx + 1],
        chunk->second.pixels[pixel_idx + 2]
    };
}


void Grid::threadDrawCell(int x, int y, sf::Color color) {
    _cell_draw_queue.push_back({x, y, color});

    int chunk_idx_x = floor(x / (float)CHUNK_SIZE);
    int chunk_idx_y = floor(y / (float)CHUNK_SIZE);

    uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;

    auto chunk = _chunks[!_buffer_idx].find(chunk_idx);

    if (chunk == _chunks[!_buffer_idx].end()) {
        auto pixels = _chunks[!_buffer_idx][chunk_idx].pixels;

        for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * 4; i += 4) {
            pixels[i] = _background_color.r;
            pixels[i + 1] = _background_color.g;
            pixels[i + 2] = _background_color.b;
            pixels[i + 3] = 255;
        }
    }

    int pixel_y = y % CHUNK_SIZE;
    if (pixel_y < 0) pixel_y += CHUNK_SIZE;

    int pixel_x = x % CHUNK_SIZE;
    if (pixel_x < 0) pixel_x += CHUNK_SIZE;

    int pixel_idx = (pixel_y * CHUNK_SIZE + pixel_x) * 4;

    auto pixels = _chunks[!_buffer_idx][chunk_idx].pixels;
    pixels[pixel_idx] = color.r;
    pixels[pixel_idx + 1] = color.g;
    pixels[pixel_idx + 2] = color.b;
    pixels[pixel_idx + 3] = 255;

    if (chunk_idx_x < _old_chunk_render_left
            || chunk_idx_x >= _old_chunk_render_right
            || chunk_idx_y < _old_chunk_render_top
            || chunk_idx_y >= _old_chunk_render_bottom)
        return;


    float blit_x = (x % _grid_texture_size) + 0.5;
    if (blit_x < 0) blit_x += _grid_texture_size;

    float blit_y = (y % _grid_texture_size) + 0.5;
    if (blit_y < 0) blit_y += _grid_texture_size;

    ADD_VERTEX(blit_x, blit_y, color);
}

void Grid::drawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g) {
    drawCell(x, y, sf::Color{r, g, b});
}

void Grid::drawCell(int x, int y, sf::Color color) {
    int chunk_idx_x = floor(x / (float)CHUNK_SIZE);
    int chunk_idx_y = floor(y / (float)CHUNK_SIZE);

    uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;

    auto chunk = _chunks[0].find(chunk_idx);

    if (chunk == _chunks[0].end()) {
        auto pixels = _chunks[0][chunk_idx].pixels;
        auto buffer_pixels = _chunks[1][chunk_idx].pixels;

        for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * 4; i += 4) {
            pixels[i] = _background_color.r;
            pixels[i + 1] = _background_color.g;
            pixels[i + 2] = _background_color.b;
            pixels[i + 3] = 255;

            buffer_pixels[i] = _background_color.r;
            buffer_pixels[i + 1] = _background_color.g;
            buffer_pixels[i + 2] = _background_color.b;
            buffer_pixels[i + 3] = 255;
        }
    }

    int pixel_y = y % CHUNK_SIZE;
    if (pixel_y < 0) pixel_y += CHUNK_SIZE;

    int pixel_x = x % CHUNK_SIZE;
    if (pixel_x < 0) pixel_x += CHUNK_SIZE;

    int pixel_idx = (pixel_y * CHUNK_SIZE + pixel_x) * 4;

    auto pixels = _chunks[0][chunk_idx].pixels;
    pixels[pixel_idx] = color.r;
    pixels[pixel_idx + 1] = color.g;
    pixels[pixel_idx + 2] = color.b;
    pixels[pixel_idx + 3] = 255;

    auto buffer_pixels = _chunks[1][chunk_idx].pixels;
    buffer_pixels[pixel_idx] = color.r;
    buffer_pixels[pixel_idx + 1] = color.g;
    buffer_pixels[pixel_idx + 2] = color.b;
    buffer_pixels[pixel_idx + 3] = 255;

    if (chunk_idx_x < _chunk_render_left
            || chunk_idx_x >= _chunk_render_right
            || chunk_idx_y < _chunk_render_top
            || chunk_idx_y >= _chunk_render_bottom)
        return;


    float blit_x = (x % _grid_texture_size) + 0.5;
    if (blit_x < 0) blit_x += _grid_texture_size;

    float blit_y = (y % _grid_texture_size) + 0.5;
    if (blit_y < 0) blit_y += _grid_texture_size;

    ADD_VERTEX(blit_x, blit_y, color);
}


bool nonPrintableAscii(char c) {  
    return c < 32 || c > 126;   
} 
void stripUnicode(std::string & text) { 
    text.erase(remove_if(text.begin(), text.end(), nonPrintableAscii), text.end());  
}

void Grid::addText(int x, int y, std::string text, sf::Color color, int style) {
    int chunk_idx_y = floor(y / (float)CHUNK_SIZE);

    stripUnicode(text);
    auto letters = text.c_str();
    for (int i = 0; i < text.length(); i++) {
        if (letters[i] == ' ') continue;

        int chunk_idx_x = floor((x + i) / (float)CHUNK_SIZE);

        uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;

        auto chunk = _chunks[0].find(chunk_idx);

        if (chunk == _chunks[0].end()) {
            auto pixels = _chunks[0][chunk_idx].pixels;
            auto buffer_pixels = _chunks[1][chunk_idx].pixels;

            for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * 4; i += 4) {
                pixels[i] = _background_color.r;
                pixels[i + 1] = _background_color.g;
                pixels[i + 2] = _background_color.b;
                pixels[i + 3] = 255;

                buffer_pixels[i] = _background_color.r;
                buffer_pixels[i + 1] = _background_color.g;
                buffer_pixels[i + 2] = _background_color.b;
                buffer_pixels[i + 3] = 255;
            }
        }

        uint64_t idx = (uint64_t)(x + i) << 32 | (uint32_t)y;
        _chunks[0][chunk_idx].letters[idx] = {letters[i], color, style};
        _chunks[1][chunk_idx].letters[idx] = {letters[i], color, style};
    }
}
