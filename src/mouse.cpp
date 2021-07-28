#include "grid.h"

void Grid::recordMousePos() {
    // TODO: clean + comments
    _mouse_timer.restart();
    _mouse_x_positions[_mouse_pos_idx] = _mouse_x;
    _mouse_y_positions[_mouse_pos_idx] = _mouse_y;
    _mouse_pos_idx = (_mouse_pos_idx + 1) % _n_mouse_positions;
}

void Grid::onMouseMotion(int x, int y) {
    if (_pan_button_pressed) {
        pan((_mouse_x - x) / _scale,
            (_mouse_y - y) / _scale);
    }
    _mouse_x = x;
    _mouse_y = y;
}

void Grid::onMousePress(int button) {
    if (button == _pan_button) {
        onPanButtonPress();
    }
    else {
        int x = _cam_x + floor((_mouse_x / _scale) + _cam_x_decimal);
        int y = _cam_y + floor((_mouse_y / _scale) + _cam_y_decimal);
        addCell(x, y, 255, 255, 255);
    }
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
}

void Grid::onMouseScroll(int wheel, float delta) {
    if (wheel == sf::Mouse::VerticalWheel) {
        _zoom_x = _mouse_x;
        _zoom_y = _mouse_y;
        _zoom_vel += delta * _zoom_speed;
    }
}
