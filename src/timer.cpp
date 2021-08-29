#include "grid.h"
#include <thread>
#include <iostream>

void Grid::startThread() {
    _thread_running = true;

    if (_thread_state == _THREAD_WAITING_FOR_JOIN)
        _thread.join();

    else if (_thread_state)
        return;

    _thread_state = _THREAD_FINISHED;
    _thread = std::thread(&Grid::threadFunc, this);
}

void Grid::threadFunc() {
    while (!_kill_thread && (_thread_running || _thread_state != _THREAD_FINISHED)) {
        if (_thread_state == _THREAD_STARTING) {
            _thread_state = _THREAD_STARTED;
            onTimerEvent();
            _thread_state = _THREAD_WAITING_FOR_SWAP;
        }
        else if (_thread_state == _THREAD_SWAPPING) {
            copyCellDrawQueue();
            _thread_state = _THREAD_FINISHED;
        }
    }
    _thread_state = _THREAD_WAITING_FOR_JOIN;
}

void Grid::startTimer() {
    if (_timer_active)
        return;

    finishAnimations();
    _timer_active = true;
    _timer.restart();
    startThread();
}

void Grid::setTimer(float timer_interval) {
    _timer_interval = timer_interval;
    _timer.restart();
}

void Grid::endThread() {
    _thread_running = false;
}

void Grid::stopTimer() {
    if (!_timer_active)
        return;

    _timer_active = false;
    endThread();
}

void Grid::incrementTimer() {
    if (!_timer_active)
        return;

    if (_thread_state == _THREAD_WAITING_FOR_SWAP) {
        if (_cell_draw_queue.size() < 500)
            drawCellQueue();
        else
            drawScreen();
        if (_chunks_pointer == &_chunks) {
            _chunks_pointer = &_chunks_buffer;
            _thread_chunks_pointer = &_chunks;
        }
        else {
            _chunks_pointer = &_chunks;
            _thread_chunks_pointer = &_chunks_buffer;
        }
        _thread_state = _THREAD_SWAPPING;
    }

    if (_thread_state == _THREAD_FINISHED && !_chunk_queue.size() && _timer.getElapsedTime().asSeconds() > _timer_interval) {
        _timer.restart();
        _thread_state = _THREAD_STARTING;
    }
}

