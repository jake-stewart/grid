#include "grid.h"
#include <math.h>
#include <iostream>

void Grid::zoom(float factor, int x, int y) {
    float x_pos = x / _scale;
    float y_pos = y / _scale;

    setScale(_scale * factor);

    pan(x_pos - x / _scale,
        y_pos - y / _scale);
}

void Grid::applyZoomVel(float delta_time) {
    if (_pan_button_pressed) {
        _zoom_x = _mouse_x;
        _zoom_y = _mouse_y;
    }


    if (_zoom_vel) {
        _zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);

        float zoom_rate = 1.0;

        if (_scale < _min_scale * (1 + _decel_distance) && _zoom_vel < 0) {
            zoom_rate = decelerateOut(delta_time);
        }

        else if (_scale > _max_scale / (1 + _decel_distance) && _zoom_vel > 0) {
            zoom_rate = decelerateIn(delta_time);
        }

        zoom(1.0 + _zoom_vel * zoom_rate * delta_time, _zoom_x, _zoom_y);

        if (abs(_zoom_vel) < _min_zoom_vel)
            _zoom_vel = 0;
    }
}
float Grid::decelerateOut(float delta_time) {
    float end = _min_scale * (1 + _decel_distance);
    float progress = (_scale - end) / (_min_scale - end);
    float difficulty = 1.0 - pow(progress, _decel_rate);

    return difficulty;
}

float Grid::decelerateIn(float delta_time) {
    float start = _max_scale / (1 + _decel_distance);
    float progress = (_scale - start) / (_max_scale - start);
    float difficulty = 1.0 - pow(progress, _decel_rate);

    return difficulty;
}
