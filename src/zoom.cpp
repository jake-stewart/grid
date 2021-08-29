#include "grid.h"
#include <math.h>

float quadraticBezier(float p0, float p1, float p2, float t) {
    return pow(1 - t,  2) * p0 + (1 - t) * 2 * t * p1 + t * t * p2;
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

    if (_zoom_vel) {
        _zoom_bounce_broken = true;

        float zoom_factor = 1.0 + _zoom_vel * delta_time;
        zoom(zoom_factor, _zoom_x, _zoom_y);

        if (_scale < _min_scale && _zoom_vel < 0) {
            _zoom_vel -= _zoom_vel * (_zoom_friction * ((_min_scale / _scale) * 3)) * delta_time;
            if (abs(_zoom_vel) < 0.1)
                _zoom_vel = 0;
        }

        else if (_scale > _max_scale && _zoom_vel > 0) {
            _zoom_vel -= _zoom_vel * (_zoom_friction * ((_scale / _max_scale) * 3)) * delta_time;
            if (abs(_zoom_vel) < 0.1)
                _zoom_vel = 0;
        }

        else {
            _zoom_vel -= _zoom_vel * _zoom_friction * delta_time;
            if (abs(_zoom_vel) < _min_zoom_vel)
                _zoom_vel = 0;
        }
    }

    else if (_scale < _min_scale || _scale > _max_scale)
        applyZoomBounce(delta_time);
}

void Grid::createZoomBounce() {
    _zoom_bounce_t = 0.0;

    if (_scale < _min_scale) {
        _zoom_bounce_p0 = _scale;
        _zoom_bounce_p1 = _min_scale - (_min_scale - _scale) / 2.0;
        _zoom_bounce_p2 = _min_scale;
    }

    else {
        _zoom_bounce_p0 = _scale;
        _zoom_bounce_p1 = _max_scale - (_max_scale - _scale) / 2.0;
        _zoom_bounce_p2 = _max_scale;
    }
}

void Grid::applyZoomBounce(float delta_time) {
    if (_zoom_bounce_broken) {
        _zoom_bounce_broken = false;
        createZoomBounce();
    }

    _zoom_bounce_t += delta_time;

    float new_scale = quadraticBezier(
        _zoom_bounce_p0,
        _zoom_bounce_p1,
        _zoom_bounce_p2,
        _zoom_bounce_t / _zoom_bounce_duration
    );

    float zoom_factor = new_scale / _scale;
    zoom(zoom_factor, _zoom_x, _zoom_y);
}
