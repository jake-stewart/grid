#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <math.h>
#include <iostream>
#include "grid.h"

#define ADD_VERTEX(x, y, color) _cell_vertexes.append(sf::Vertex({x, y}, color))

using std::unordered_map;


void Grid::render() {
    _grid_render_texture.display();
    _grid_sprite.setScale(_scale, _scale);

    _grid_sprite.setTextureRect({
        (_chunk_x * CHUNK_SIZE) % _grid_texture_size,
        (_chunk_y * CHUNK_SIZE) % _grid_texture_size,
        _render_distance * CHUNK_SIZE,
        _render_distance * CHUNK_SIZE 
    });

    _blit_x_offset = (-CHUNK_SIZE + (CHUNK_SIZE - _cam_x_decimal)) * _scale;
    _blit_y_offset = (-CHUNK_SIZE + (CHUNK_SIZE - _cam_y_decimal)) * _scale;

    _grid_sprite.setPosition(_blit_x_offset, _blit_y_offset);

    if (_antialias_enabled && _scale > 1) {
        _shader.setUniform("scale", _scale);
        _window.draw(_grid_sprite, &_shader);
    }

    else _window.draw(_grid_sprite);
}

void Grid::renderText() {
    int character_size;

    if (_scale < 5) return;
    else if (_scale < 7) character_size = 7;
    else if (_scale < 13) character_size = 13;
    else if (_scale < 22) character_size = 22;
    else if (_scale < 40) character_size = 40;
    else if (_scale < 69) character_size = 69;
    else if (_scale < 160) character_size = 160;
    else character_size = 300;

    int alpha = (_scale < 11)
        ? alpha = ((_scale - 5) / (11 - 5)) * 255
        : 255;

    float increased_scale = _scale / character_size;

    sf::Text text;
    text.setFont(_font);
    text.scale(increased_scale, increased_scale);
    text.setCharacterSize(character_size);


    for (int chunk_x = _chunk_x; chunk_x <= _chunk_x + _n_chunks_width; chunk_x++) {
        for (int chunk_y = _chunk_y; chunk_y <= _chunk_y + _n_chunks_height; chunk_y++) {

            uint64_t idx = (uint64_t)chunk_x << 32 | (uint32_t)chunk_y;
            auto chunk = _chunks[_buffer_idx].find(idx);
            if (chunk == _chunks[_buffer_idx].end())
                continue;

            for (auto it: chunk->second.letters) {
                int x = it.first >> 32;
                int y = (int)it.first;

                float blit_x = (x - (_chunk_x * CHUNK_SIZE + _cam_x_decimal) + 0.2) * _scale;
                float blit_y = (y - (_chunk_y * CHUNK_SIZE + _cam_y_decimal) - 0.15) * _scale;

                text.setString(it.second.letter);
                it.second.color.a = alpha;
                text.setFillColor(it.second.color);
                text.setStyle(it.second.style);
                text.setPosition(blit_x, blit_y);
                //text.setOrigin(text.getLocalBounds().left/2.0f,text.getLocalBounds().top);
                _window.draw(text);
            }
        }
    }
}

void Grid::updateChunkQueue() {
    _n_chunks_width = ceil(_screen_width / (CHUNK_SIZE * _scale));
    int spare_chunks_x = _render_distance - _n_chunks_width;
    if (spare_chunks_x < 0) spare_chunks_x = 0;
    int chunk_render_left = _chunk_x - spare_chunks_x / 2;
    int chunk_render_right = chunk_render_left + _render_distance;

    _n_chunks_height = ceil(_screen_height / (CHUNK_SIZE * _scale));
    int spare_chunks_y = _render_distance - _n_chunks_height;
    if (spare_chunks_y < 0) spare_chunks_y = 0;
    int chunk_render_top = _chunk_y - spare_chunks_y / 2;
    int chunk_render_bottom = chunk_render_top + _render_distance;

    if (chunk_render_left == _chunk_render_left
            && chunk_render_right == _chunk_render_right
            && chunk_render_top == _chunk_render_top
            && chunk_render_bottom == _chunk_render_bottom)
        return;

    auto it = _chunk_queue.end();
    while (it > _chunk_queue.begin()) {
        it--;
        if (it->first < chunk_render_left
                || it->first >= chunk_render_right
                || it->second < chunk_render_top
                || it->second >= chunk_render_bottom)
            it = _chunk_queue.erase(it);
    }

    int x, y;
    int n_left = 0;
    int n_right = 0;
    int n_top = 0;
    int n_bottom = 0;

    for (x = chunk_render_left; x < _chunk_render_left; x++) {
        n_left++;
        for (y = chunk_render_top; y < chunk_render_bottom; y++) {
            _chunk_queue.push_back({x, y});
        }
    }

    for (x = _chunk_render_right; x < chunk_render_right; x++) {
        n_right++;
        for (y = chunk_render_top; y < chunk_render_bottom; y++) {
            _chunk_queue.push_back({x, y});
        }
    }

    for (y = chunk_render_top; y < _chunk_render_top; y++) {
        n_top++;
        for (x = chunk_render_left + n_left; x < chunk_render_right - n_right; x++) {
            _chunk_queue.push_back({x, y});
        }
    }

    for (y = _chunk_render_bottom; y < chunk_render_bottom; y++) {
        n_bottom++;
        for (x = chunk_render_left + n_left; x < chunk_render_right - n_right; x++) {
            _chunk_queue.push_back({x, y});
        }
    }

    sf::RectangleShape rectangle;
    rectangle.setFillColor(_background_color);

    float blit_x, blit_y;

    blit_x = (chunk_render_left * CHUNK_SIZE) % _grid_texture_size;
    if (blit_x < 0) blit_x += _grid_texture_size;
    rectangle.setPosition({blit_x, 0});
    rectangle.setSize({(float)CHUNK_SIZE * n_left, (float)_render_distance * CHUNK_SIZE});
    _grid_render_texture.draw(rectangle);

    blit_x = (_chunk_render_right * CHUNK_SIZE) % _grid_texture_size;
    if (blit_x < 0) blit_x += _grid_texture_size;
    rectangle.setPosition({blit_x, 0});
    rectangle.setSize({(float)CHUNK_SIZE * n_right, (float)_render_distance * CHUNK_SIZE});
    _grid_render_texture.draw(rectangle);

    blit_y = (chunk_render_top * CHUNK_SIZE) % _grid_texture_size;
    if (blit_y < 0) blit_y += _grid_texture_size;
    rectangle.setPosition({0, blit_y});
    rectangle.setSize({(float)_render_distance * CHUNK_SIZE, (float)CHUNK_SIZE * n_top});
    _grid_render_texture.draw(rectangle);

    blit_y = (_chunk_render_bottom * CHUNK_SIZE) % _grid_texture_size;
    if (blit_y < 0) blit_y += _grid_texture_size;
    rectangle.setPosition({0, blit_y});
    rectangle.setSize({(float)_render_distance * CHUNK_SIZE, (float)CHUNK_SIZE * n_bottom});
    _grid_render_texture.draw(rectangle);

    _chunk_render_left = chunk_render_left;
    _chunk_render_right = chunk_render_right;
    _chunk_render_top = chunk_render_top;
    _chunk_render_bottom = chunk_render_bottom;
}

void Grid::updateChunks() {
    if (!_chunk_queue.size())
        return;

    while (_chunk_queue.size()) {
        uint64_t idx = (uint64_t)_chunk_queue[0].first << 32 | (uint32_t)_chunk_queue[0].second;

        auto chunk = _chunks[_buffer_idx].find(idx);

        if (chunk != _chunks[_buffer_idx].end()) {
            int blit_x = (_chunk_queue[0].first * CHUNK_SIZE) % _grid_texture_size;
            if (blit_x < 0) blit_x += _grid_texture_size;

            int blit_y = (_chunk_queue[0].second * CHUNK_SIZE) % _grid_texture_size;
            if (blit_y < 0) blit_y += _grid_texture_size;

            _chunk_texture.update(chunk->second.pixels);
            _chunk_sprite.setPosition(blit_x, blit_y);
            _grid_render_texture.draw(_chunk_sprite);
        }

        _chunk_queue.erase(_chunk_queue.begin());
        if (_clock.getElapsedTime().asSeconds() > _frame_duration)
            break;
    }
}

void Grid::drawScreen() {
    _chunk_queue.clear();

    int n_chunks_width = ceil(_screen_width / (CHUNK_SIZE * _scale));
    int spare_chunks_x = _render_distance - n_chunks_width;
    if (spare_chunks_x < 0) spare_chunks_x = 0;
    int _chunk_render_left = _chunk_x - spare_chunks_x / 2;
    int _chunk_render_right = _chunk_render_left + _render_distance;

    int n_chunks_height = ceil(_screen_height / (CHUNK_SIZE * _scale));
    int spare_chunks_y = _render_distance - n_chunks_height;
    if (spare_chunks_y < 0) spare_chunks_y = 0;
    int _chunk_render_top = _chunk_y - spare_chunks_y / 2;
    int _chunk_render_bottom = _chunk_render_top + _render_distance;

    for (int x = _chunk_render_left; x < _chunk_render_right; x++) {
        for (int y = _chunk_render_top; y < _chunk_render_bottom; y++) {
            _chunk_queue.push_back({x, y});
        }
    }
}
