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

    if (_scale < _min_scale)
        bounceIn(delta_time);

    else if (_scale > _max_scale)
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
	if (_zoom_vel <= 0) {
		float target_factor = (_scale * 1.05) / _max_scale - 1;
		float min_zoom_vel = -(target_factor / pow(1 - _zoom_friction, _bounce_duration));
		if (_zoom_vel > min_zoom_vel) {
			_zoom_vel = min_zoom_vel;
		}

		_zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);
		float zoom_factor = 1.0 + _zoom_vel * delta_time;
		zoom(zoom_factor, _zoom_x, _zoom_y);
		if (abs(_zoom_vel) < _min_zoom_vel)
			_zoom_vel = 0;
	}
	else {
		float min = 300;
		float max = 400;
		float difficulty = 1 - ((_scale - min) / (max - min));
		_zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);
		float zoom_factor = 1.0 + (_zoom_vel * delta_time) * difficulty;

		zoom(zoom_factor, _zoom_x, _zoom_y);
		if (abs(_zoom_vel) < (2 - difficulty*2))
			_zoom_vel = 0;
	}
}

void Grid::bounceIn(float delta_time) {
	if (_zoom_vel >= 0) {
		float target_factor = (_min_scale * 1.05) / _scale - 1;
		float min_zoom_vel = target_factor / pow(1 - _zoom_friction, _bounce_duration);
		if (_zoom_vel < min_zoom_vel) {
			_zoom_vel = min_zoom_vel;
		}

		_zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);
		float zoom_factor = 1.0 + _zoom_vel * delta_time;
		zoom(zoom_factor, _zoom_x, _zoom_y);
		if (abs(_zoom_vel) < _min_zoom_vel)
			_zoom_vel = 0;
	}
	else {
		float min = 1.5;
		float max = 2;
		float difficulty = (_scale - min) / (max - min);

		_zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);
		float zoom_factor = 1.0 + (_zoom_vel * delta_time) * difficulty;

		zoom(zoom_factor, _zoom_x, _zoom_y);
		if (abs(_zoom_vel) < 1.0)
			_zoom_vel = 0;
	}
}
