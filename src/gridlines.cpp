#include <SFML/Graphics.hpp>
#include "grid.h"

#define ADD_VERTEX(x, y, color) _vertex_array.append(sf::Vertex({x, y}, color))
#define ADD_VERTEX_TEX(x, y, color, tx, ty) _vertex_array.append(sf::Vertex({x, y}, color, {tx, ty}))
#define ADD_VERTEX_LINE(x, y) _line_array.append(sf::Vertex({x, y}));

void Grid::applyGridEffects() {
    // if cells have not begun fading in yet, or gridlines are disabled,
    // then do not display them
    if (_cell_gridline_ratio == 0 || _scale <= _fade_start_scale)
        _grid_thickness = 0;

    // otherwise, if scale is still within the fading range,
    // apply the fading effects
    else if (_scale <= _fade_end_scale) {
        float progress = (_scale - _fade_end_scale) /
            (_fade_start_scale - _fade_end_scale);

        calcGridThickness();
        _gridline_color.a = (_grid_max_alpha - progress *
                _grid_max_alpha);
    }

    // finally, if scale is not within fading range,
    // display gridlines at full opacity
    else {
        calcGridThickness();
        _gridline_color.a = _grid_max_alpha;
    }
}

void Grid::calcGridThickness() {
    if (_gridline_shrink) {
        _grid_thickness = _scale * _cell_gridline_ratio;
        if (_grid_thickness < 1)
            _grid_thickness = 1;
    }
    else
        _grid_thickness = _cell_gridline_ratio;
}

void Grid::renderGridlines() {
    int t = _grid_thickness;
    float start_y = _blit_y_offset - t / 2;
    float start_x = _blit_x_offset - t / 2;
    float blit_x, blit_y;

    int texture_height = _column_rendertex.getSize().y;
    if (texture_height < _screen_height) {
        while (texture_height < _screen_height)
            texture_height *= 2;

        _column_rendertex.create(1, texture_height);
    }

    _column_rendertex.clear(sf::Color{0, 0, 0, 0});

    for (float y = start_y; y < _screen_height; y += _scale) {
        blit_y = round(y);

        ADD_VERTEX(0,                    blit_y,     _gridline_color);
        ADD_VERTEX(0,                    blit_y + t, _gridline_color);
        ADD_VERTEX((float)_screen_width, blit_y + t, _gridline_color);
        ADD_VERTEX((float)_screen_width, blit_y,     _gridline_color);

        // at the same time, continue constructing the column lines with gaps
        ADD_VERTEX_LINE(1, blit_y + t);
        ADD_VERTEX_LINE(1, round(y + _scale));
    }

    _window.draw(_vertex_array);
    _vertex_array.clear();

    _column_rendertex.draw(_line_array);
    _column_rendertex.display();
    _line_array.clear();

    for (float x = start_x; x < _screen_width; x += _scale) {
        ADD_VERTEX_TEX(x,     0,                     _gridline_color, 0, 0);
        ADD_VERTEX_TEX(x + t, 0,                     _gridline_color, 1, 0);
        ADD_VERTEX_TEX(x + t, (float)_screen_height, _gridline_color, 1, (float)_screen_height);
        ADD_VERTEX_TEX(x,     (float)_screen_height, _gridline_color, 0, (float)_screen_height);
    }

    // draw the vertices, with the constructed column line texture
    _window.draw(_vertex_array, &_column_rendertex.getTexture());
    _vertex_array.clear();
}

void Grid::renderGridlinesAA() {
    float start_x = _blit_x_offset - _grid_thickness / 2;
    float start_y = _blit_y_offset - _grid_thickness / 2;

    for (float x = start_x; x < _screen_width; x += _scale) {
        calcGridlineAA(x);

        // left shit
        ADD_VERTEX(_start_offset,     0,                     _aa_color_l);
        ADD_VERTEX(_start_offset + 1, 0,                     _aa_color_l);
        ADD_VERTEX(_start_offset + 1, (float)_screen_height, _aa_color_l);
        ADD_VERTEX(_start_offset,     (float)_screen_height, _aa_color_l);

        // middle shit
        ADD_VERTEX(_start_offset + 1, 0,                     _gridline_color);
        ADD_VERTEX(_end_offset,       0,                     _gridline_color);
        ADD_VERTEX(_end_offset,       (float)_screen_height, _gridline_color);
        ADD_VERTEX(_start_offset + 1, (float)_screen_height, _gridline_color);

        // end shit
        ADD_VERTEX(_end_offset,       0,                     _aa_color_r);
        ADD_VERTEX(_end_offset + 1,   0,                     _aa_color_r);
        ADD_VERTEX(_end_offset + 1,   (float)_screen_height, _aa_color_r);
        ADD_VERTEX(_end_offset,       (float)_screen_height, _aa_color_r);
    }

    for (float y = start_y; y < _screen_height; y += _scale) {
        calcGridlineAA(y);

        // left shit
        ADD_VERTEX(0,                    _start_offset,     _aa_color_l);
        ADD_VERTEX(0,                    _start_offset + 1, _aa_color_l);
        ADD_VERTEX((float)_screen_width, _start_offset + 1, _aa_color_l);
        ADD_VERTEX((float)_screen_width, _start_offset,     _aa_color_l);

        // middle shit
        ADD_VERTEX(0,                    _start_offset + 1, _gridline_color);
        ADD_VERTEX(0,                    _end_offset,       _gridline_color);
        ADD_VERTEX((float)_screen_width, _end_offset,       _gridline_color);
        ADD_VERTEX((float)_screen_width, _start_offset + 1, _gridline_color);

        // end shit
        ADD_VERTEX(0,                    _end_offset,       _aa_color_r);
        ADD_VERTEX(0,                    _end_offset + 1,   _aa_color_r);
        ADD_VERTEX((float)_screen_width, _end_offset + 1,   _aa_color_r);
        ADD_VERTEX((float)_screen_width, _end_offset,       _aa_color_r);
    }

    _window.draw(_vertex_array);
    _vertex_array.clear();
}

void Grid::calcGridlineAA(float p) {
    float excess_thickness = _grid_thickness - (int)_grid_thickness;

    int start_alpha, end_alpha;

    int blit_p = round(p);
    float pixel_offset = p - blit_p;

    if (pixel_offset < 0) {
        start_alpha = (-pixel_offset) * _gridline_color.a;
        _start_offset = blit_p - 1;
        _end_offset = blit_p + (int)_grid_thickness;
    }
    else {
        start_alpha = (1.0 - pixel_offset) * _gridline_color.a;
        _start_offset = blit_p;
        _end_offset = blit_p + (int)_grid_thickness + 1;
    }

    end_alpha = excess_thickness * _gridline_color.a - start_alpha;
    if (end_alpha < 0) {
        end_alpha += _gridline_color.a;
        _end_offset -= 1;
    }

    _aa_color_l.a = start_alpha;
    _aa_color_r.a = end_alpha;
}

void Grid::applyGridFading(float delta_time) {
    _screen_changed = true;
    _display_grid = true;
    _grid_max_alpha += _grid_fading * (delta_time / _grid_fade_duration) * 255;

    if (_grid_max_alpha <= 0) {
        _grid_max_alpha = 0;
        _grid_fading = 0;
        _display_grid = false;
    }
    else if (_grid_max_alpha >= _grid_default_max_alpha) {
        _grid_max_alpha = 255;
        _grid_fading = 0;
    }
    applyGridEffects();
}

