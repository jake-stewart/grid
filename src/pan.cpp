#include "grid.h"
#include <iostream>
#include <math.h>

void Grid::pan(float x, float y) {
    _cam_x_decimal += x;
    _cam_y_decimal += y;

    int chunk_pan = floor(_cam_x_decimal / CHUNK_SIZE);
    if (chunk_pan) {
        _chunk_x += chunk_pan;
        _cam_x_decimal -= chunk_pan * CHUNK_SIZE;
        _chunk_x_cell = _chunk_x * CHUNK_SIZE;
    }

    chunk_pan = floor(_cam_y_decimal / CHUNK_SIZE);
    if (chunk_pan) {
        _chunk_y += chunk_pan;
        _cam_y_decimal -= chunk_pan * CHUNK_SIZE;
        _chunk_y_cell = _chunk_y * CHUNK_SIZE;
    }

    _grid_moved = true;
}
void Grid::applyPanVel(float delta_time) {
    if (!_pan_vel_x && !_pan_vel_y)
        return;

    if (_pan_vel_x)
        _pan_vel_x = _pan_vel_x * pow(1 - _pan_friction, delta_time);

    if (_pan_vel_y)
        _pan_vel_y = _pan_vel_y * pow(1 - _pan_friction, delta_time);

    pan(_pan_vel_x * delta_time,
        _pan_vel_y * delta_time);

    if (abs(_pan_vel_x) < _min_pan_vel && abs(_pan_vel_y) < _min_pan_vel) {
        _pan_vel_x = 0;
        _pan_vel_y = 0;
    }
    _mouse_moved = true;
}


