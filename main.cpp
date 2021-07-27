
#include <SFML/Graphics.hpp>
#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <math.h>
#include <stdlib.h>

using namespace std;

#define RAND() static_cast <float> (rand()) / static_cast <float> (RAND_MAX)

#define ADD_VERTEX(x, y, color) _vertex_array.append(sf::Vertex({x, y}, color))
#define ADD_VERTEX_TEX(x, y, color, tx, ty) _vertex_array.append(sf::Vertex({x, y}, color, {tx, ty}))
#define ADD_VERTEX_LINE(x, y) _line_array.append(sf::Vertex({x, y}));

void print() {
    cout<<'\n';
}

template<typename T, typename ...TAIL>
void print(const T &t, TAIL... tail) {
    cout << t << ' ';
    print(tail...);
}

int randint(int min, int max) {
    return rand() % (max - min) + min;
}


sf::Color colorMix(sf::Color a, sf::Color b, float perc) {
    float alt_perc = 1 - perc;

    return sf::Color{
        sf::Uint8(a.r * perc + b.r * alt_perc),
        sf::Uint8(a.g * perc + b.b * alt_perc),
        sf::Uint8(a.b * perc + b.g * alt_perc),
    };
}

float quadraticBezier(float p0, float p1, float p2, float t) {
    return pow(1 - t,  2) * p0 + (1 - t) * 2 * t * p1 + t * t * p2;
}

// rendering text
// cells are stored as structs
// cell.r cell.b cell.g cell.l
// cell.l is letter
// when cell is loaded and it has a letter, the letter and coords are stored in a vector
// when rendering, iterate over vector, draw all letters at coords. if coords are out of bounds, remove from vector

// TODO
// [x] only drawIntroducedCells() once per frame
// [ ] comments + cleanup
// [ ] split into multiple files (cleaner?)
// [x] don't redraw intersections of rows/columns
// [x] gridlines + grid effects
// [x] grid pan velocity
// [x] universal delta time (avoid things being faster when higher fps)
// [ ] cell deletion
// [x] cleanup events
// [x] cleanup draw rows/columns
// [ ] animations
// [ ] rect cell draw/deletion
// [x] zoom limits
// [x] resizing
// [ ] extendability + threading



class Grid {
public:
    Grid(const char * title, int n_cols, int n_rows, float scale) {
        _title = title;
        _screen_width = scale * n_cols;
        _screen_height = scale * n_rows;
        setScale(scale);
        setGridThickness(0.08);
        setGridlinesFade(7, 20);
    }

    void addCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g) {
        int chunk_x = x / _chunk_size;
        int chunk_y = y / _chunk_size;

        sf::Uint8 * cell;
        cell = _columns[x][chunk_y][y];
        cell[0] = r;
        cell[1] = g;
        cell[2] = b;
        cell[3] = 255;

        cell = _rows[y][chunk_x][x];
        cell[0] = r;
        cell[1] = g;
        cell[2] = b;
        cell[3] = 255;

        if (_col_start <= x && x <= _col_end && _row_start <= y && y <= _row_end) {
            int blit_x = x % _grid_texture_width;
            if (blit_x < 0) blit_x += _grid_texture_width;

            int blit_y = y % _grid_texture_height;
            if (blit_y < 0) blit_y += _grid_texture_height;

            _grid_texture.update(&cell[0], 1, 1, blit_x, blit_y);
            _screen_changed = true;
        }
    }

    int start() {
        int err_code = initialize();
        if (err_code != 0)
            return err_code;

        onStart();
        mainloop();
        return 0;
    }

    void setGridThickness(float value) {
        _screen_changed = true;

        // value <= 0 will disable gridlines entirely
        if (value <= 0) {
            _grid_thickness = 0;
            _cell_gridline_ratio = 0;
        }

        else {
           // 0 < value < 1 will set gridline thickness to be a ratio of a cell
           // for example 0.1 will mean gridlines account for 10% of a cell
            if (value >= 1) {
                _cell_gridline_ratio = value;
                _gridline_shrink = false;
            }

            // value >= 1 will set gridline thickness to that value
            else {
                _cell_gridline_ratio = value;
                _gridline_shrink = true;
            }

            applyGridEffects();
        }
    }

    void setGridlinesFade(float start_scale, float end_scale) {
        // sets at what cell scale gridlines should begin and end fading at

        // fade_start must be less than fade_end
        // if it's not, switch them around
        if (start_scale > end_scale) {
            _fade_start_scale = end_scale;
            _fade_end_scale = start_scale;
        }
        else {
            _fade_start_scale = start_scale;
            _fade_end_scale = end_scale;
        }

        _screen_changed = true;
    }

    void setGridlineAlpha(int alpha) {
        _grid_default_max_alpha = alpha;
        _screen_changed = true;
    }

    void toggleGridlines() {
        if (_grid_fading == 1)
            _grid_fading = -1;
        else if (_grid_fading == -1)
            _grid_fading = 1;
        else if (_display_grid)
            _grid_fading = -1;
        else
            _grid_fading = 1;
    }

    void useAntialiasing(bool value) {
        _antialias_enabled = value;
        _grid_texture.setSmooth(_antialias_enabled);
        _screen_changed = true;
    }

private:
    int _chunk_size = 32;
    int _grid_fading = 0;
    float _grid_fade_duration = 0.15;

    bool _antialias_enabled = true;

    const char * _title;

    sf::Shader _shader;

    // int _grid_texture_width = 256;
    // int _grid_texture_height = 256;
    int _grid_texture_width = 3840;
    int _grid_texture_height = 2160;

    int _max_cells_x = _grid_texture_width - 2;
    int _max_cells_y = _grid_texture_height - 2;

    // mouse button used for panning/dragging screen
    int _pan_button = sf::Mouse::Middle;
    bool _pan_button_pressed = false;

    // last mouse location on screen
    int _mouse_x = 0;
    int _mouse_y = 0;

    // visible row geometry
    int _n_visible_rows = 0;
    int _n_visible_cols = 0;
    int _screen_width, _screen_height;

    int _max_fps = 160;
    int _n_frames = 0;

    // screen's top left corner coordinates of grid
    int _cam_x = 0;
    int _cam_y = 0;
    float _x_offset = 0;
    float _y_offset = 0;
    float _pixel_x_offset;
    float _pixel_y_offset;

    int _col_start = 0;
    int _col_end = 0;
    int _row_start = 0;
    int _row_end = 0;

    float _pan_vel_x = 0;
    float _pan_vel_y = 0;

    // pan velocity decreases faster once the user has clicked the screen
    // to drag again. stopping the velocity instantly can be jarring
    float _weak_pan_friction = 5;
    float _strong_pan_friction = 20;
    float _pan_friction;

    float _min_pan_vel = 1;

    // introduced cells should be drawn altogether once per frame.
    // this way, the grid can zoom, pan, and resize multiple times without
    // redrawing introduced cells multiple times
    bool _screen_changed = false;

    // how zoomed in grid is. eg. scale 1.5 means each cell is 1.5 pixels.
    // min and max scale determine cell size limits. cell size < 1 is possible but
    // causes jitteriness when panning (also very computationally expensive because
    // of how many cells have to be drawn)
    float _scale = 10;
    float _max_scale = 300;
    float _min_scale = 2;

    float _zoom_vel = 0;
    int _zoom_x = 0;
    int _zoom_y = 0;

    float _zoom_friction = 5;
    float _zoom_speed = 1;
    float _min_zoom_vel = 0.01;

    // when you zoom beyond the min/max cell size, the scale bounces back
    // these are the values used for the bezier curve
    float _zoom_bounce_p0;
    float _zoom_bounce_p1;
    float _zoom_bounce_p2;
    float _zoom_bounce_t;
    float _zoom_bounce_duration = 0.2;

    // if you zoom in/out while the scale is bouncing back,
    // the bezier curve will need to be recalculated
    bool _zoom_bounce_broken = true;

    // cells in grid are stored in both a rows and columns map
    // this makes it more efficient when drawing entire rows and entire columns
    // _rows[chunk_index][x] = {r, g, b, a}
    // _column[chunk_index][y] = {r, g, b, a}
    // when drawing a row or column, the chunks are iterated over, instead of individual rows.
    // then, each cell of the chunk is iterated over. this allows for empty pixels to be ignored.
    unordered_map<int, unordered_map<int, unordered_map<int, sf::Uint8[4]>>> _columns;
    unordered_map<int, unordered_map<int, unordered_map<int, sf::Uint8[4]>>> _rows;

    // a pixel buffer for drawing rows/columns. these pixels are used to update the grid texture
    // the grid texture is located in graphics memory, so the pixels should not be edited directly,
    // since calls are slow.
    sf::Uint8 * _pixels;

    // solarized
    // sf::Color _background_color{0x00, 0x2b, 0x36};
    // sf::Color _gridline_color{0x1c, 0x41, 0x4a};

    // gruvbox light
    // sf::Color _background_color{0xFB, 0xF1, 0xC7};
    // sf::Color _gridline_color{0xC6, 0xBA, 0x9D};

    // gruvbox dark
    // sf::Color _background_color{0x28, 0x28, 0x28};
    // sf::Color _gridline_color{0x45, 0x41, 0x3D};

    // one dark
    // sf::Color _background_color{0x28, 0x2C, 0x34};
    // sf::Color _gridline_color{0x38, 0x3C, 0x46};

    // boring light
    // sf::Color _background_color{0xff, 0xff, 0xff};
    // sf::Color _gridline_color{0x66, 0x66, 0x66};

    // boring dark
    // sf::Color _background_color{0x00, 0x00, 0x00};
    // sf::Color _gridline_color{0x22, 0x22, 0x22};

    // ubuntu
    sf::Color _background_color{0x1b, 0x1b, 0x1b};
    sf::Color _gridline_color{0x44, 0x44, 0x44};


    sf::Color _aa_color_l = _gridline_color;
    sf::Color _aa_color_r = _gridline_color;
    float _start_offset, _end_offset;

    bool _display_grid = true;
    float _grid_thickness = 0;
    bool _gridline_shrink = true;
    float _cell_gridline_ratio;
    int _grid_default_max_alpha = 255;
    int _grid_max_alpha = _grid_default_max_alpha;
    int _fade_end_scale;
    int _fade_start_scale;

    // SFML window
    sf::RenderWindow _window;
    sf::View _view;

    // timer that keeps track of the user's timer.
    // each time this timer ticks, onTimer() is called
    sf::Clock _clock, _timer;
    float _timer_interval = 1;

    sf::Clock _mouse_timer;
    float _t_per_mouse_pos = 0.01;
    static const int _n_mouse_positions = 4;
    int _mouse_x_positions [_n_mouse_positions];
    int _mouse_y_positions [_n_mouse_positions];
    int _mouse_pos_idx = 0;

    // grid texture stored on graphics card for quicker rendering
    sf::Texture _grid_texture;

    sf::VertexArray _vertex_array;
    sf::VertexArray _line_array;

    sf::RenderTexture _column_rendertex;

    // sprites simply wrap textures and are used for rendering instead
    // of using the textures themselves.
    sf::Sprite _grid_sprite;


    void randomCircle(float center_x, float center_y, float radius, float density) {
        float theta;
        int n_cells = density * (M_PI * (radius * radius));
        sf::Uint8 r, g, b;

        int x, y;

        for (int i = 0; i < n_cells; i++) {

            r = radius * sqrt(RAND());
            theta = RAND() * 2 * M_PI;

            x = center_x + r * cos(theta);
            y = center_y + r * sin(theta);

            // r = randint(30, 100);
            // g = randint(30, 100);
            // b = randint(30, 100);
            r = 50;
            g = 50;
            b = 50;

            addCell(x, y, r, g, b);
        }
    }

    void onStart() {
        int n_circles = randint(2, 5);
        int x, y, radius;
        float density;

        // for (int i = 0; i < n_circles; i++) {
        //     x = randint(-200, 200);
        //     y = randint(-200, 200);
        //     radius = randint(10, 200);
        //     density = 0.1 + RAND();
        //     randomCircle(x, y, radius, density);
        // }

        randomCircle(-50, -50, 100, 0.5);


        for (int y = 5; y < 100; y += 5) {
            for (int x = 0; x < 100; x += 1) {
                addCell(x, y, 100, 100, 100);
            }
        }
        for (int x = 5; x < 100; x += 5) {
            for (int y = 0; y < 100; y += 1) {
                addCell(x, y, 100, 100, 100);
            }
        }

        addCell( 0,  0, 255, 255, 255);
        addCell( 1,  0, 128, 128, 128);
        addCell(-1,  0, 128, 128, 128);
        addCell( 0,  1, 128, 128, 128);
        addCell( 0, -1, 128, 128, 128);
        addCell( 1,  1,  64,  64,  64);
        addCell( 1, -1,  64,  64,  64);
        addCell(-1,  1,  64,  64,  64);
        addCell(-1, -1,  64,  64,  64);
        addCell( 2,  0,  32,  32,  32);
        addCell(-2,  0,  32,  32,  32);
        addCell( 0,  2,  32,  32,  32);
        addCell( 0, -2,  32,  32,  32);

    }


    bool initialize() {
        _window.create(sf::VideoMode(_screen_width, _screen_height), _title);
        _window.setFramerateLimit(_max_fps); // FPS

        _view = _window.getDefaultView();

        // vertices used for batch drawing multiple grid lines efficiently
        _vertex_array.setPrimitiveType(sf::Quads);
        _line_array.setPrimitiveType(sf::Lines);

        // texture stored in graphics memory where each pixel is a cell
        _grid_texture.create(_grid_texture_width, _grid_texture_height);
        _grid_sprite.setTexture(_grid_texture);

        // when the grid is panned beyond the bounds of the grid texture,
        // the cells are wrapped to the other side of the texture.
        // setting the texture to be repeated allows for the texture to automatically
        // wrap when rendering
        _grid_texture.setRepeated(true);
        _grid_texture.setSmooth(_antialias_enabled);

        // pixel which are manipulated before being used to update the grid texture
        int n_pixels = _grid_texture_width * _grid_texture_height * 4;
        _pixels = new sf::Uint8[n_pixels];
        for (int i = 0; i < n_pixels; i++)
            _pixels[i] = 255;

        // since row and column lines intersect, the column lines are drawn
        // with gaps where the intersections would occur.
        // to make this efficient, one column line is created and then this
        // line is used for each column
        _column_rendertex.create(1, _screen_height);

        _shader.loadFromFile("scale.frag", sf::Shader::Fragment);
        _shader.setUniform("tex", _grid_texture);
        _shader.setUniform("geometry", sf::Vector2f(_grid_texture_width, _grid_texture_height));

        return 0;
    }

    void calcGridSize() {
        _col_start = _cam_x;
        _row_start = _cam_y;

        // 1 is added, since half of two cells could be visible
        _n_visible_cols = ceil(_screen_width / _scale) + 1;
        _n_visible_rows = ceil(_screen_height / _scale) + 1;

        _col_end = _col_start + _n_visible_cols;
        _row_end = _row_start + _n_visible_rows;
    }

    void applyGridEffects() {
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

    void calcGridThickness() {
        if (_gridline_shrink) {
            _grid_thickness = _scale * _cell_gridline_ratio;
            if (_grid_thickness < 1)
                _grid_thickness = 1;
        }
        else
            _grid_thickness = _cell_gridline_ratio;
    }


    void render() {
        _grid_sprite.setScale(_scale, _scale);

        _grid_sprite.setTextureRect({
            _cam_x % _grid_texture_width,
            _cam_y % _grid_texture_height,
            _n_visible_cols,
            _n_visible_rows
        });

        _pixel_x_offset = (-1 + (1 - _x_offset)) * _scale;
        _pixel_y_offset = (-1 + (1 - _y_offset)) * _scale;
        _grid_sprite.setPosition(_pixel_x_offset, _pixel_y_offset);

        if (_antialias_enabled) {
            _shader.setUniform("scale", _scale);
            _window.draw(_grid_sprite, &_shader);
        }

        else _window.draw(_grid_sprite);
    }

    void renderGridlines() {
        int t = _grid_thickness;
        float start_y = _pixel_y_offset - t / 2;
        float start_x = _pixel_x_offset - t / 2;
        float blit_x, blit_y;

        int texture_height = _column_rendertex.getSize().y;
        if (texture_height < _screen_height) {
            while (texture_height < _screen_height)
                texture_height *= 2;

            _column_rendertex.create(1, texture_height);
        }

        _column_rendertex.clear(
            sf::Color(0, 0, 0, 0)
        );

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

    void renderGridlinesAA() {
        float start_x = _pixel_x_offset - _grid_thickness / 2;
        float start_y = _pixel_y_offset - _grid_thickness / 2;

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

    void calcGridlineAA(float p) {
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

    void pan(float x, float y) {
        _x_offset += x;
        _y_offset += y;

        int cell_pan;

        if (_x_offset < 0 || _x_offset >= 1) {
            cell_pan = floor(_x_offset);
            _cam_x += cell_pan;
            _x_offset -= cell_pan;
        }
        if (_y_offset < 0 || _y_offset >= 1) {
            cell_pan = floor(_y_offset);
            _cam_y += cell_pan;
            _y_offset -= cell_pan;
        }

        _screen_changed = true;
    }

    void setScale(float scale) {
        // calculate how many cells would be visible at the new scale
        float n_cells_x = _screen_width / scale;
        float n_cells_y = _screen_height / scale;

        // if grid texture is not large enough to support this many cells at this scale
        if (n_cells_x > _max_cells_x || n_cells_y > _max_cells_y) {
            float min_scale_x = (float)_screen_width / _max_cells_x;
            float min_scale_y = (float)_screen_height / _max_cells_y;

            // then set the scale to be as small as possible
            // and stop zooming, since there is no point continuing to zoom out
            _scale = (min_scale_x > min_scale_y) ? min_scale_x : min_scale_y;
            _zoom_vel = 0;
        }
        else {
            // otherwise, set the scale to the desired value
            _scale = scale;
        }

        applyGridEffects();
    }

    void zoom(float factor, int x, int y) {
        float x_pos = x / _scale;
        float y_pos = y / _scale;

        setScale(_scale * factor);

        pan(x_pos - x / _scale,
            y_pos - y / _scale);

        _screen_changed = true;
    }

    void drawIntroducedCells() {
        // by comparing the old visible grid position and size to the new size,
        // we are able to calculate how many new rows/columns are visible
        // this way, instead of drawing every cell, only the introduced cells
        // have to be drawn

        // rows and columns intersect in the corners
        // to avoid drawing cells twice, we keep track of how many columns
        // are drawn, and skip them when drawing rows
        int n_cols_drawn_left = 0;
        int n_cols_drawn_right = 0;

        // store old visible grid
        int old_col_start = _col_start;
        int old_col_end = _col_end;
        int old_row_start = _row_start;
        int old_row_end = _row_end;

        // calculate new visible grid
        calcGridSize();

        // draw new columns introduced on the left
        if (_col_start < old_col_start) {
            n_cols_drawn_left = old_col_start - _col_start;
            drawColumns(
                _col_start,
                n_cols_drawn_left
            );
        }

        // draw new columns introduced on the right
        if (old_col_end < _col_end) {
            n_cols_drawn_right = _col_end - old_col_end;
            drawColumns(
                old_col_end,
                n_cols_drawn_right
            );
        }

        // draw new rows introduced on the top
        if (_row_start < old_row_start)
            drawRows(
                _row_start,
                old_row_start - _row_start,
                _col_start + n_cols_drawn_left,
                _n_visible_cols - n_cols_drawn_left - n_cols_drawn_right
            );

        // draw new rows introduced on the bottom
        if (old_row_end < _row_end)
            drawRows(
                old_row_end,
                _row_end - old_row_end,
                _col_start + n_cols_drawn_left,
                _n_visible_cols - n_cols_drawn_left - n_cols_drawn_right
            );
    }

    void drawRows(int y, int n_rows, int x, int n_cols) {
        // calculate where on the grid texture the columns will be drawn
        int blit_y = y % _grid_texture_height;
        if (blit_y < 0) blit_y += _grid_texture_height;

        int blit_x = x % _grid_texture_width;
        if (blit_x < 0) blit_x += _grid_texture_width;

        // if the columns extend beyond the grid texture, then the
        // extra columns will be drawn at the start of the texture
        if (blit_x + n_cols > _grid_texture_width) {
            int excess_cols = (blit_x + n_cols) - _grid_texture_width;
            n_cols -= excess_cols;
            drawRows(y, n_rows, x + n_cols, excess_cols);
        }

        // the pixels, which will be used to draw cells before being sent
        // to the graphics card must be cleared to be the background color
        clearPixels(n_rows, n_cols);

        unordered_map<int, unordered_map<int, unordered_map
            <int, sf::Uint8[4]>>>::const_iterator row_iter;

        unordered_map<int, unordered_map<int, sf::Uint8[4]>>
            ::const_iterator chunk_iter;

        int chunk_start_idx = x / _chunk_size;
        int chunk_end_idx = chunk_start_idx + ceil(n_cols / (float)_chunk_size) + 1;
        int chunk_idx;

        // variables used to calculate where in the pixel array a cell belongs
        int cell_idx;
        int y_offset = 0;
        int row_size = n_cols * 4;

        // for every row, iterate over every visible chunk within that row
        // for every chunk, iterate over every cell contained within that chunk
        // for every cell, add it to the pixel array
        for (int y_idx = y; y_idx < y + n_rows; y_idx++) {
            row_iter = _rows.find(y_idx);

            if (row_iter != _rows.end()) {
                for (chunk_idx = chunk_start_idx; chunk_idx < chunk_end_idx; chunk_idx++) {
                    chunk_iter = row_iter->second.find(chunk_idx);
                    if (chunk_iter == row_iter->second.end())
                        continue;

                    for (auto& cell: chunk_iter->second) {
                        cell_idx = cell.first - x;
                        if (cell_idx < 0 || cell_idx >= n_cols)
                            continue;

                        cell_idx = cell_idx * 4 + y_offset;
                        _pixels[cell_idx] = cell.second[0];
                        _pixels[cell_idx + 1] = cell.second[1];
                        _pixels[cell_idx + 2] = cell.second[2];
                    }
                }
            }
            y_offset += row_size;
        }

        // use generated pixels to update the grid texture
        updateTexture(blit_x, blit_y, n_rows, n_cols);
    }

    void drawColumns(int x, int n_cols) {
        // calculate where on the grid texture the columns will be drawn
        int blit_x = x % _grid_texture_width;
        if (blit_x < 0) blit_x += _grid_texture_width;

        int blit_y = _cam_y % _grid_texture_height;
        if (blit_y < 0) blit_y += _grid_texture_height;

        // if the columns extend beyond the grid texture, then the
        // extra columns will be drawn at the start of the texture
        if (blit_x + n_cols > _grid_texture_width) {
            int excess_cols = (blit_x + n_cols) - _grid_texture_width;
            n_cols -= excess_cols;
            drawColumns(x + n_cols, excess_cols);
        }

        // the pixels, which will be used to draw cells before being sent
        // to the graphics card must be cleared to be the background color
        clearPixels(n_cols, _n_visible_rows);

        unordered_map<int, unordered_map<int, unordered_map
            <int, sf::Uint8[4]>>>::const_iterator col_iter;

        unordered_map<int, unordered_map<int, sf::Uint8[4]>>
            ::const_iterator chunk_iter;

        int chunk_start_idx = _cam_y / _chunk_size;
        int chunk_end_idx = chunk_start_idx + ceil(_n_visible_rows / (float)_chunk_size) + 1;
        int chunk_idx;

        // variables used to calculate where in the pixel array a cell belongs
        int cell_idx;
        int x_offset = 0;
        int row_size = n_cols * 4;

        // for every column, iterate over every visible chunk within that column
        // for every chunk, iterate over every cell contained within that chunk
        // for every cell, add it to the pixel array
        for (int x_idx = x; x_idx < x + n_cols; x_idx++) {
            col_iter = _columns.find(x_idx);

            if (col_iter != _columns.end()) {
                for (chunk_idx = chunk_start_idx; chunk_idx < chunk_end_idx; chunk_idx++) {
                    chunk_iter = col_iter->second.find(chunk_idx);
                    if (chunk_iter == col_iter->second.end())
                        continue;

                    for (auto& cell: chunk_iter->second) {
                        cell_idx = (cell.first - _cam_y);
                        if (cell_idx < 0 || cell_idx >= _n_visible_rows) continue;
                        cell_idx = cell_idx * row_size + x_offset;
                        _pixels[cell_idx] = cell.second[0];
                        _pixels[cell_idx + 1] = cell.second[1];
                        _pixels[cell_idx + 2] = cell.second[2];
                    }
                }
            }
            x_offset += 4;
        }

        // use generated pixels to update the grid texture
        updateTexture(blit_x, blit_y, _n_visible_rows, n_cols);
    }


    void clearPixels(int n_rows, int n_cols) {
        int pixels_size = n_rows * n_cols * 4;

        for (int i = 0; i < pixels_size; i += 4) {
            _pixels[i] = _background_color.r;
            _pixels[i + 1] = _background_color.g;
            _pixels[i + 2] = _background_color.b;
        }
    }

    void updateTexture(int blit_x, int blit_y, int n_rows, int n_cols) {
        // if the pixels extend beyond the grid texture's height,
        // then the excess pixels will need to be wrapped to the start
        // of the texture
        if (blit_y + n_rows > _grid_texture_height) {
            int n = n_rows - ((blit_y + n_rows) - _grid_texture_height);
            _grid_texture.update(_pixels, n_cols, n, blit_x, blit_y);
            _grid_texture.update(
                &_pixels[n * n_cols * 4],
                n_cols, n_rows - n, blit_x, 0
            );
        }
        else {
            // otherwise, the pixels can be updated all at once
            _grid_texture.update(_pixels, n_cols, n_rows, blit_x, blit_y);
        }
    }

    void onMouseMotion(int x, int y) {
        if (_pan_button_pressed) {
            pan((_mouse_x - x) / _scale,
                (_mouse_y - y) / _scale);
        }
        _mouse_x = x;
        _mouse_y = y;
    }

    void onMousePress(int button) {
        if (button == _pan_button) {
            onPanButtonPress();
        }
        else {
            int x = _cam_x + floor((_mouse_x / _scale) + _x_offset);
            int y = _cam_y + floor((_mouse_y / _scale) + _y_offset);
            addCell(x, y, 255, 255, 255);
        }
    }

    void onPanButtonPress() {
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

    void onPanButtonRelease() {
        _pan_button_pressed = false;

        // set pan friction back to normal
        _pan_friction = _weak_pan_friction;

        // calculate the velocity
        int idx = (_mouse_pos_idx + 1) % _n_mouse_positions;
        _pan_vel_x = (_mouse_x_positions[idx] - _mouse_x) / (_t_per_mouse_pos * _n_mouse_positions);
        _pan_vel_y = (_mouse_y_positions[idx] - _mouse_y) / (_t_per_mouse_pos * _n_mouse_positions);
    }

    void onMouseRelease(int button) {
        if (button == _pan_button)
            onPanButtonRelease();
    }

    void onResize(unsigned int new_width, unsigned int new_height) {
        _view.setSize({
            static_cast<float>(new_width),
            static_cast<float>(new_height)
        });
        _view.setCenter(new_width / 2.0, new_height / 2.0);
        _window.setView(_view);

        // cell coords at middle of screen, relative to top left
        float middle_x = _cam_x + (_screen_width / 2.0) / _scale;
        float middle_y = _cam_y + (_screen_height / 2.0) / _scale;

        _screen_width = new_width;
        _screen_height = new_height;

        // setScale() will increase the grid's scale if the grid texture
        // is too small
        setScale(_scale);

        // new cell coords at middle of screen, relative to top left
        float new_middle_x = _cam_x + (_screen_width / 2.0) / _scale;
        float new_middle_y = _cam_y + (_screen_height / 2.0) / _scale;

        // make sure the old middle cell is still in the middle of the screen
        pan(middle_x - new_middle_x, middle_y - new_middle_y);
    }

    void onMouseScroll(int wheel, float delta) {
        if (wheel == sf::Mouse::VerticalWheel) {
            _zoom_x = _mouse_x;
            _zoom_y = _mouse_y;
            _zoom_vel += delta * _zoom_speed;
        }
    }

    void onKeyRelease(int key_code) {
    }

    void onKeyPress(int key_code) {
        switch (key_code)
        {
            case sf::Keyboard::Up:
                pan(0, -0.001);
                break;

            case sf::Keyboard::Down:
                pan(0, 0.001);
                break;

            case sf::Keyboard::Left:
                pan(-0.001, 0);
                break;

            case sf::Keyboard::Right:
                pan(0.001, 0);
                break;
            
            case sf::Keyboard::F:
                useAntialiasing(!_antialias_enabled);
                break;

            case sf::Keyboard::G:
                toggleGridlines();
                break;

            default:
                break;
        }
    }

    void onWindowClose() {
        _window.close();
    }

    void handleEvents() {
        sf::Event event;

        while (_window.pollEvent(event)) {
            switch (event.type) {

                case sf::Event::Closed:
                    onWindowClose();
                    break;

                case sf::Event::MouseMoved:
                    onMouseMotion(event.mouseMove.x, event.mouseMove.y);
                    break;

                case sf::Event::MouseButtonPressed:
                    onMousePress(event.mouseButton.button);
                    break;

                case sf::Event::MouseButtonReleased:
                    onMouseRelease(event.mouseButton.button);
                    break;

                case sf::Event::Resized:
                    onResize(event.size.width, event.size.height);
                    break;

                case sf::Event::MouseWheelScrolled:
                    onMouseScroll(
                        event.mouseWheelScroll.wheel,
                        event.mouseWheelScroll.delta
                    );
                    break;

                case sf::Event::KeyPressed:
                    onKeyPress(event.key.code);
                    break;

                case sf::Event::KeyReleased:
                    onKeyRelease(event.key.code);
                    break;

                default:
                    break;
            }
        }
    }

    void applyZoomVel(float delta_time) {
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
    void createZoomBounce() {
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

    void applyZoomBounce(float delta_time) {
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

    void onTimer() {
        // print("FPS:", _n_frames);
        _n_frames = 0;
    }
    void incrementTimer() {
        _n_frames++;
        if (_timer.getElapsedTime().asSeconds() > _timer_interval) {
            _timer.restart();
            onTimer();
        }
    }

    void applyPanVel(float delta_time) {
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
    }

    void recordMousePos() {
        // TODO: clean + comments
        _mouse_timer.restart();
        _mouse_x_positions[_mouse_pos_idx] = _mouse_x;
        _mouse_y_positions[_mouse_pos_idx] = _mouse_y;
        _mouse_pos_idx = (_mouse_pos_idx + 1) % _n_mouse_positions;
    }

    void applyGridFading(float delta_time) {
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

    void mainloop() {
        // TODO: clean + comments

        bool increasing = true;
        float delta_time;

        while (_window.isOpen())
        {
            // pan(0.01 / _scale, 0.01 / _scale);
            delta_time = _clock.restart().asSeconds();
            incrementTimer();
            handleEvents();

            if (_grid_fading)
                applyGridFading(delta_time);

            applyZoomVel(delta_time);
            applyPanVel(delta_time);

            if (_pan_button_pressed && _mouse_timer.getElapsedTime()
                    .asSeconds() > _t_per_mouse_pos)
                recordMousePos();

            if (_screen_changed) {
                _screen_changed = false;
                drawIntroducedCells();
                _window.clear();
                render();

                if (_display_grid && _grid_thickness > 0) {
                    if (_antialias_enabled)
                        renderGridlinesAA();
                    else
                        renderGridlines();
                }
            }

            _window.display();
        }
    }
};


int main() {
    Grid grid("test grid", 20, 20, 40);
    // grid.setGridThickness(6);
    // grid.useAntialiasing(false);
    // grid.setGridlinesFade(0, 0);
    // grid.setGridlineAlpha(50);
    return grid.start();
}
