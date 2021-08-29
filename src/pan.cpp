#include "grid.h"
#include <iostream>
#include <math.h>

void Grid::pan(float x, float y) {
    _cam_x_decimal += x;
    _cam_y_decimal += y;

    int chunk_pan = floor(_cam_x_decimal / _chunk_size);
    if (chunk_pan) {
        _chunk_x += chunk_pan;
        _cam_x_decimal -= chunk_pan * _chunk_size;
        _chunk_x_cell = _chunk_x * _chunk_size;
    }

    chunk_pan = floor(_cam_y_decimal / _chunk_size);
    if (chunk_pan) {
        _chunk_y += chunk_pan;
        _cam_y_decimal -= chunk_pan * _chunk_size;
        _chunk_y_cell = _chunk_y * _chunk_size;
    }

    _grid_moved = true;
}

void Grid::applyPanVel(float delta_time) {
    if (!_pan_vel_x && !_pan_vel_y)
        return;

    pan(_pan_vel_x * delta_time / _scale,
        _pan_vel_y * delta_time / _scale);

    _pan_vel_x -= _pan_vel_x * _pan_friction * delta_time;
    _pan_vel_y -= _pan_vel_y * _pan_friction * delta_time;

    if (abs(_pan_vel_x) < _min_pan_vel && abs(_pan_vel_y) < _min_pan_vel) {
        _pan_vel_x = 0;
        _pan_vel_y = 0;
    }
    _mouse_moved = true;
}

