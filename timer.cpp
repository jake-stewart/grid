#include "grid.h"
#include <thread>


void Grid::endThread() {
    if (!_thread_running)
        return;
    _thread_running = false;
    _thread.join();
}

void Grid::startThread() {
    if (_thread_running)
        return;

    _thread_running = true;
    _thread = std::thread(&Grid::threadFunc, this);
}

void Grid::threadFunc() {
    while (_thread_running) {
        if (_start_thread) {
            _start_thread = false;
            onTimerEvent();
            _thread_finished = true;
        }
    }
}

void Grid::startTimer() {
    if (_timer_active)
        return;

    _timer_active = true;
    _timer.restart();
    startThread();
}

void Grid::setTimer(float timer_interval) {
    _timer_interval = timer_interval;
    _timer.restart();
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

    if (_thread_finished && _timer.getElapsedTime().asSeconds() > _timer_interval) {
        _cell_draw_queue.erase(
            _cell_draw_queue.begin(),
            _cell_draw_queue.begin() + _current_queue_idx
        );
        finishAnimations();
        _current_queue_idx = 0;
        _timer.restart();
        _thread_finished = false;
        _start_thread = true;
    }
}

