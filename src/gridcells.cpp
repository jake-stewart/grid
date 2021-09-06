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
        (_chunk_x * _chunk_size) % _grid_texture_width,
        (_chunk_y * _chunk_size) % _grid_texture_height,
        _render_distance * _chunk_size,
        _render_distance * _chunk_size 
    });

    _blit_x_offset = (-_chunk_size + (_chunk_size - _cam_x_decimal)) * _scale;
    _blit_y_offset = (-_chunk_size + (_chunk_size - _cam_y_decimal)) * _scale;

    _grid_sprite.setPosition(_blit_x_offset, _blit_y_offset);

    if (_antialias_enabled) {
        _shader.setUniform("scale", _scale);
        _window.draw(_grid_sprite, &_shader);
    }

    else _window.draw(_grid_sprite);
}

void Grid::renderText() {
    int character_size;
	int alpha = 255;

	if (_scale < 5) return;
	else if (_scale < 9) {
		alpha = ((_scale - 5) / (9 - 5)) * 255;
		//if (alpha < 0 || alpha > 255) alpha = 255;
		character_size = 9;
	}
    else if (_scale < 15) character_size = 15;
    else if (_scale < 22) character_size = 22;
    else if (_scale < 47) character_size = 47;
    else if (_scale < 75) character_size = 75;
    else if (_scale < 160) character_size = 160;
    else character_size = 300;

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

                float blit_x = (x - (_chunk_x * _chunk_size + _cam_x_decimal) + 0.2) * _scale;
                float blit_y = (y - (_chunk_y * _chunk_size + _cam_y_decimal) - 0.125) * _scale;

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
    _n_chunks_width = ceil(_screen_width / (_chunk_size * _scale));
    int spare_chunks_x = _render_distance - _n_chunks_width;
    if (spare_chunks_x < 0) spare_chunks_x = 0;
    int chunk_render_left = _chunk_x - spare_chunks_x / 2;
    int chunk_render_right = chunk_render_left + _render_distance;

    _n_chunks_height = ceil(_screen_height / (_chunk_size * _scale));
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
        for (x = chunk_render_left + n_left; x < chunk_render_right - n_right; x++) {
            _chunk_queue.push_back({x, y});
        }
    }

    for (y = _chunk_render_bottom; y < chunk_render_bottom; y++) {
        for (x = chunk_render_left + n_left; x < chunk_render_right - n_right; x++) {
            _chunk_queue.push_back({x, y});
        }
    }

    _chunk_render_left = chunk_render_left;
    _chunk_render_right = chunk_render_right;
    _chunk_render_top = chunk_render_top;
    _chunk_render_bottom = chunk_render_bottom;
}

void Grid::updateChunks() {
    sf::RectangleShape rectangle;
    rectangle.setSize({(int)_chunk_size, (int)_chunk_size});
    rectangle.setFillColor(_background_color);

    if (!_chunk_queue.size())
        return;

    while (_chunk_queue.size()) {
        uint64_t idx = (uint64_t)_chunk_queue[0].first << 32 | (uint32_t)_chunk_queue[0].second;
        //uint64_t chunk_idx = uint64_t(x / _chunk_size) << 32 | uint32_t(y / _chunk_size);

        auto chunk = _chunks[_buffer_idx].find(idx);

        int blit_x = (_chunk_queue[0].first * _chunk_size) % _grid_texture_width;
        if (blit_x < 0) blit_x += _grid_texture_width;

        int blit_y = (_chunk_queue[0].second * _chunk_size) % _grid_texture_height;
        if (blit_y < 0) blit_y += _grid_texture_height;

        if (chunk == _chunks[_buffer_idx].end()) {
            rectangle.setPosition({(float)blit_x, (float)blit_y});
            _grid_render_texture.draw(rectangle);
        }

        else {
            //sf::Texture tex = _grid_render_texture.getTexture();
            //tex.update(
            //    chunk->second,
            //    blit_x, blit_y,
            //    _chunk_size, _chunk_size
            //);
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

    int n_chunks_width = ceil(_screen_width / (_chunk_size * _scale));
    int spare_chunks_x = _render_distance - n_chunks_width;
    if (spare_chunks_x < 0) spare_chunks_x = 0;
    int _chunk_render_left = _chunk_x - spare_chunks_x / 2;
    int _chunk_render_right = _chunk_render_left + _render_distance;

    int n_chunks_height = ceil(_screen_height / (_chunk_size * _scale));
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
