#include "grid.h"

void Grid::incrementTimer() {
    _n_frames++;
    if (_timer.getElapsedTime().asSeconds() > _timer_interval) {
        _timer.restart();
        onTimer();
    }
}

