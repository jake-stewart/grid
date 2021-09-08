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


	if (_zoom_vel) {
        _zoom_vel = _zoom_vel * pow(1 - _zoom_friction, delta_time);

		float zoom_rate = 1.0;

		if (_scale < _min_scale * (1 + _decelerate_out_space) && _zoom_vel < 0)
			zoom_rate = decelerateOut(delta_time);

		else if (_scale > _max_scale / (1 + _decelerate_in_space) && _zoom_vel > 0)
			zoom_rate = decelerateIn(delta_time);

        zoom(1.0 + _zoom_vel * zoom_rate * delta_time, _zoom_x, _zoom_y);

        if (abs(_zoom_vel) < _min_zoom_vel)
            _zoom_vel = 0;
    }
}
float Grid::decelerateOut(float delta_time) {
	float difficulty = 1 - log10(
		(_min_scale * (1 + _decelerate_out_space) - _min_scale) /
		(_scale - _min_scale)
	);

	if (isnan(difficulty) || difficulty < 0 || difficulty > 1)
		return 0;

	return difficulty;
}

float Grid::decelerateIn(float delta_time) {
	float start = _max_scale / (1 + _decelerate_in_space);
	float difficulty = -log10(
		(_scale - start + (_max_scale - start) / 10) /
		(_max_scale - start)
	);

	if (isnan(difficulty) || difficulty < 0 || difficulty > 1)
		return 0;

	return difficulty;
}
