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
