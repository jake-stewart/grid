#include "grid.h"

void Grid::pan(float x, float y) {
    _cam_x_decimal += x;
    _cam_y_decimal += y;

    int cell_pan = floor(_cam_x_decimal);
    _cam_x += cell_pan;
    _cam_x_decimal -= cell_pan;

    cell_pan = floor(_cam_y_decimal);
    _cam_y += cell_pan;
    _cam_y_decimal -= cell_pan;

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
    onMouseMotion(_mouse_x, _mouse_y);
}

