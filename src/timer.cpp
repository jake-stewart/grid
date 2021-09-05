#include "grid.h"
#include <thread>
#include <math.h>
#include <iostream>
#include <chrono>
#include <mutex>

void Grid::drawCellQueue() {
    for (auto cell: _cell_draw_queue) {
        int chunk_idx_x = floor(cell.x / (float)_chunk_size);
        int chunk_idx_y = floor(cell.y / (float)_chunk_size);

        uint64_t chunk_idx = (uint64_t)chunk_idx_x << 32 | (uint32_t)chunk_idx_y;

        auto chunk = _chunks[!_buffer_idx].find(chunk_idx);

        if (chunk == _chunks[!_buffer_idx].end()) {
            auto pixels = _chunks[!_buffer_idx][chunk_idx].pixels;

            for (int i = 0; i < _chunk_size * _chunk_size * 4; i += 4) {
                pixels[i] = _background_color.r;
                pixels[i + 1] = _background_color.g;
                pixels[i + 2] = _background_color.b;
                pixels[i + 3] = 255;
            }
        }

        int pixel_y = cell.y % _chunk_size;
        if (pixel_y < 0) pixel_y += _chunk_size;

        int pixel_x = cell.x % _chunk_size;
        if (pixel_x < 0) pixel_x += _chunk_size;

        int pixel_idx = (pixel_y * _chunk_size + pixel_x) * 4;

        auto pixels = _chunks[!_buffer_idx][chunk_idx].pixels;
        pixels[pixel_idx] = cell.color.r;
        pixels[pixel_idx + 1] = cell.color.g;
        pixels[pixel_idx + 2] = cell.color.b;
        pixels[pixel_idx + 3] = 255;
    }
    _cell_draw_queue.clear();
}

void Grid::startThread() {
    if (!_thread_active) {
        _thread_state = inactive;
        _thread_active = true;
        _thread = std::thread(&Grid::threadFunc, this);
    }
}
void Grid::endThread() {
    if (_thread_active) {
        _thread_active = false;
        _thread_state = joining;
        _cv.notify_one();
        _thread.join();
    }
}

void Grid::threadFunc() {
    while (true) {
        {
            // wait for thread to be triggered
            std::unique_lock<std::mutex> lock(_mutex);

            if (!_thread_active)
                return;

            _thread_state = inactive;
            while (_thread_state == inactive) {
                _cv.wait(lock);
            }
        }

        // run the thread
        onTimerEvent(_n_iterations);

        {
            // wait for main thread to have swapped buffer
            std::unique_lock<std::mutex> lock(_mutex);

            if (!_thread_active)
                return;

            _thread_state = swapping;
            while (_thread_state == swapping) {
                _cv.wait(lock);
            }
        }

        if (!_thread_active)
            return;

        // duplicate the new cells onto the buffer
        drawCellQueue();
    }
    _thread_state = joining;
}

void Grid::startTimer() {
    if (_timer_active)
        return;

    _old_chunk_render_left = _chunk_render_left;
    _old_chunk_render_right = _chunk_render_right;
    _old_chunk_render_top = _chunk_render_top;
    _old_chunk_render_bottom = _chunk_render_bottom;

    finishAnimations();
    _timer_active = true;
    _timer.restart();
}

void Grid::setTimer(float timer_interval) {
    _timer_interval = timer_interval;
    _timer.restart();
}

void Grid::stopTimer() {
    if (!_timer_active)
        return;

    _timer_active = false;
}

void Grid::incrementTimer() {
    if (!_timer_active)
        return;

    if (_chunk_queue.size())
        return;
    
    if (_timer.getElapsedTime().asSeconds() < _timer_interval)
        return;

    if (_thread_state != inactive) {
        return;
    }

    std::unique_lock<std::mutex> lock(_mutex);

    _old_chunk_render_left = _chunk_render_left;
    _old_chunk_render_right = _chunk_render_right;
    _old_chunk_render_top = _chunk_render_top;
    _old_chunk_render_bottom = _chunk_render_bottom;


    _n_iterations = (_timer_interval == 0) ? -1 : round(_timer.getElapsedTime().asSeconds() / _timer_interval);

    _timer.restart();

    _thread_state = active;
    _cv.notify_one();
}

