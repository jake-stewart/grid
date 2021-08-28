#include <math.h>
#include <iostream>
#include "grid.h"

void Grid::recordMousePos() {
    // TODO: clean + comments
    _mouse_timer.restart();
    _mouse_x_positions[_mouse_pos_idx] = _mouse_x;
    _mouse_y_positions[_mouse_pos_idx] = _mouse_y;
    _mouse_pos_idx = (_mouse_pos_idx + 1) % _n_mouse_positions;
}

void Grid::onMouseMotion(int x, int y) {
    if (_pan_button_pressed)
        pan((_mouse_x - x) / _scale,
            (_mouse_y - y) / _scale);

    int mouse_cell_x = _chunk_x_cell + (int)floor(_mouse_x / _scale + _cam_x_decimal);
    int mouse_cell_y = _chunk_y_cell + (int)floor(_mouse_y / _scale + _cam_y_decimal);

    calculateTraversedCells(mouse_cell_x, mouse_cell_y);

    _mouse_cell_x = mouse_cell_x;
    _mouse_cell_y = mouse_cell_y;
    _mouse_x = x;
    _mouse_y = y;
}

void Grid::calculateTraversedCells(int target_x, int target_y) {
    float angle = atan2(target_x - _mouse_cell_x, target_y - _mouse_cell_y);
    float x_vel = sin(angle);
    float y_vel = cos(angle);

    // if there is no x movement or no y movement, we do not need their respective vel
    if (_mouse_cell_x == target_x)
        x_vel = 0;

    // possibly change to just if??
    else if (_mouse_cell_y == target_y)
        y_vel = 0;

    float x = 0;
    int last_x = _mouse_cell_x;
    int cell_x = _mouse_cell_x;

    float y = 0;
    int last_y = _mouse_cell_y;
    int cell_y = _mouse_cell_y;


    while (true) {
        if (cell_x == target_x) {
            if (cell_y == target_y)
                // both cells have reached their targets
                // loop can end
                break;
            y += y_vel;
        }
        else {
            if (cell_y != target_y)
                y += y_vel;
            x += x_vel;
        }

        // you can use int() instead of round(), but round() is smoother
        cell_x = _mouse_cell_x + (int)round(x);
        cell_y = _mouse_cell_y + (int)round(y);

        // avoid multiple mouse events over the same cell
        if (cell_x == last_x && cell_y == last_y)
            continue;

        onMouseDragEvent(cell_x, cell_y);

        // if the cell jumped diagonally, we want to fill it in
        // whether we fill the cell on the x axis or y axis is important
        // if we always fill one axis, it will look choppy
        // filling follows the rule: always fill y, unless y has had > 2 cells for the current x

        if (last_x != cell_x && last_y != cell_y) {
            if (_fill_x)
                onMouseDragEvent(last_x, cell_y);
            else
                onMouseDragEvent(cell_x, last_y);
        }
        else
            _fill_x = last_y == cell_y;

        last_x = cell_x;
        last_y = cell_y;
    }
}

void Grid::onMousePress(int button) {
    if (button == _pan_button)
        onPanButtonPress();

    else onMousePressEvent(_mouse_cell_x, _mouse_cell_y, button);
}

void Grid::onPanButtonPress() {
    _pan_button_pressed = true;

    // increase pan friction, so that if the screen is
    // busy panning, it slows down quickly
    _pan_friction = _strong_pan_friction;

    // clear old mouse positions
    _mouse_pos_idx = 0;
    for (int i = 0; i < _n_mouse_positions; i++) {
        _mouse_x_positions[i] = _mouse_x;
        _mouse_y_positions[i] = _mouse_y;
    }
}

void Grid::onPanButtonRelease() {
    _pan_button_pressed = false;

    // set pan friction back to normal
    _pan_friction = _weak_pan_friction;

    // calculate the velocity
    int idx = (_mouse_pos_idx + 1) % _n_mouse_positions;
    _pan_vel_x = (_mouse_x_positions[idx] - _mouse_x) / (_t_per_mouse_pos * _n_mouse_positions);
    _pan_vel_y = (_mouse_y_positions[idx] - _mouse_y) / (_t_per_mouse_pos * _n_mouse_positions);
}

void Grid::onMouseRelease(int button) {
    if (button == _pan_button)
        onPanButtonRelease();
    else
        onMouseReleaseEvent(_mouse_cell_x, _mouse_cell_y, button);
}

void Grid::onMouseScroll(int wheel, float delta) {
    if (wheel == sf::Mouse::VerticalWheel) {
        _zoom_x = _mouse_x;
        _zoom_y = _mouse_y;
        _zoom_vel += delta * _zoom_speed;
    }
}
