#include "grid.h"
#include <math.h>
#include <iostream>

float quadraticBezier(float p0, float p1, float p2, float t) {
    return pow(1 - t,  2) * p0 + (1 - t) * 2 * t * p1 + t * t * p2;
}

float cubicBezier(float p0, float p1, float p2, float p3, float t) {
		return pow(1 - t, 3) * p0 + pow(1 - t, 2) * 3 * t * p1 +
        (1 - t) * 3 * t * t * p2 + t * t * t * p3;
}

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

    if (_scale < _min_scale && _zoom_vel <= 0)
        bounceIn(delta_time);

    else if (_scale > _max_scale && _zoom_vel >= 0)
        bounceOut(delta_time);

    else if (_zoom_vel) {
        _zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);

        float zoom_factor = 1.0 + _zoom_vel * delta_time;
        zoom(zoom_factor, _zoom_x, _zoom_y);

        if (abs(_zoom_vel) < _min_zoom_vel)
            _zoom_vel = 0;
    }
}
void Grid::bounceOut(float delta_time) {
    if (_zoom_vel) {

        float zoom_vel = _zoom_vel * pow(1 - _zoom_friction, _bounce_duration / 2) * _bounce_duration;
        float target_scale = _scale * (1 + zoom_vel);

        _bounce_p0 = _scale;
        _bounce_p2 = (target_scale > _max_bezier_cutoff) ? _max_bezier_cutoff : target_scale;
        _bounce_p1 = _bounce_p2 + (_bounce_p2 - _scale) / 2;

        _bounce_state = 0;
        _zoom_vel = 0;
        _bounce_t = 0.0;
    }

    float new_scale;

    _bounce_t += delta_time;
    if (_bounce_t >= _bounce_duration) {
        new_scale = _bounce_p2;
        _bounce_t = 0;

        if (_bounce_state == 0) {
            _bounce_state = 1;
            _bounce_p0 = new_scale;
            _bounce_p1 = _max_scale;
            _bounce_p2 = _max_scale;
        }
    }
    else
        new_scale = quadraticBezier(
            _bounce_p0,
            _bounce_p1,
            _bounce_p2,
            _bounce_t / _bounce_duration
        );

    float x_pos = _zoom_x / _scale;
    float y_pos = _zoom_y / _scale;
    setScale(new_scale);
    pan(x_pos - _zoom_x / _scale,
        y_pos - _zoom_y / _scale);
}

void Grid::bounceIn(float delta_time) {
    if (_zoom_vel) {
        float zoom_vel = _zoom_vel * pow(1 - _zoom_friction, _bounce_duration / 2) * _bounce_duration;
        float target_scale = _scale * (1 + zoom_vel);

        _bounce_p0 = _scale;
        _bounce_p1 = (target_scale < _min_bezier_cutoff) ? _min_bezier_cutoff : target_scale;
        _bounce_p2 = _bounce_p1 + (_scale - _bounce_p1) / 2;

        _bounce_t = 0.0;
        _zoom_vel = 0;
        _bounce_state = 0;
    }

    float new_scale;

    _bounce_t += delta_time;
    if (_bounce_t >= _bounce_duration) {
        new_scale = _bounce_p2;
        _bounce_t = 0;

        if (_bounce_state == 0) {
            _bounce_state = 1;
            _bounce_p0 = new_scale;
            _bounce_p1 = _min_scale;
            _bounce_p2 = _min_scale;
        }
    }
    else
        new_scale = quadraticBezier(
            _bounce_p0,
            _bounce_p1,
            _bounce_p2,
            _bounce_t / _bounce_duration
        );

    float x_pos = _zoom_x / _scale;
    float y_pos = _zoom_y / _scale;
    setScale(new_scale);
    pan(x_pos - _zoom_x / _scale,
        y_pos - _zoom_y / _scale);
}
