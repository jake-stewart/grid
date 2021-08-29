#include <SFML/Graphics.hpp>
#include <math.h>
#include <climits>
#include "grid.h"
#include <iostream>

#define RAND() static_cast <float> (rand()) / static_cast <float> (RAND_MAX)

// rendering text
// cells are stored as structs
// cell.r cell.b cell.g cell.l
// cell.l is letter
// when cell is loaded and it has a letter, the letter and coords are stored in a vector
// when rendering, iterate over vector, draw all letters at coords. if coords are out of bounds, remove from vector

// TODO
// [x] only drawIntroducedCells() once per frame
// [x] split into multiple files (cleaner?)
// [x] don't redraw intersections of rows/columns
// [x] gridlines + grid effects
// [x] grid pan velocity
// [x] universal delta time (avoid things being faster when higher fps)
// [x] cleanup events
// [x] cleanup draw rows/columns
// [x] animations
// [x] zoom limits
// [x] resizing
// [ ] rect cell draw use clearArea()
// [ ] comments + cleanup
// [ ] cell deletion
// [ ] extendability

Grid::Grid(const char * title, int n_cols, int n_rows, float scale) {
    //_cam_x = 2147483648;
    //_cam_y = 2147483648;
    _cam_x = 0;
    _cam_y = 0;

    _title = title;
    _screen_width = scale * n_cols;
    _screen_height = scale * n_rows;

    _mouse_cell_x = _cam_x;
    _mouse_cell_y = _cam_y;

    _col_start = _cam_x;
    _row_start = _cam_y;
    _col_end = _cam_x;
    _row_end = _cam_y;

    _grid_fading = 0;
    _grid_fade_duration = 0.15;

    _antialias_enabled = true;

    _grid_texture_width = 2048;
    _grid_texture_height = 2048;

    _display_grid = true;
    _timer_interval = 1;

    _t_per_mouse_pos = 0.01;

    _max_fps = 150;
    _frame_duration = 1.0 / _max_fps;

    _pan_button = sf::Mouse::Middle;
    _min_pan_vel = 0.01;
    _weak_pan_friction = 5;
    _strong_pan_friction = 20;

    _scale = scale;
    _max_scale = 300;
    _min_scale = 2;

    _zoom_friction = 5;
    _zoom_speed = 1;
    _min_zoom_vel = 0.1;

    _zoom_bounce_duration = 0.2;

    // default
    _foreground_color = sf::Color{0x54, 0x5d, 0x66};
    _background_color = sf::Color{0xd0, 0xd5, 0xde};
    _gridline_color   = sf::Color{0x75, 0x83, 0x8e};

    // solarized
    //_foreground_color = sf::Color{0xEE, 0xE8, 0xD5};
    //_background_color = sf::Color{0x00, 0x2b, 0x36};
    //_gridline_color   = sf::Color{0x1c, 0x41, 0x4a};

    // gruvbox light
    //_foreground_color = sf::Color{0x3C, 0x38, 0x36};
    //_background_color = sf::Color{0xFB, 0xF1, 0xC7};
    //_gridline_color   = sf::Color{0xC6, 0xBA, 0x9D};

    // gruvbox dark
    //_foreground_color = sf::Color{0xEB, 0xDB, 0xB2};
    //_background_color = sf::Color{0x28, 0x28, 0x28};
    //_gridline_color   = sf::Color{0x45, 0x41, 0x3D};

    // one dark
    //_foreground_color = sf::Color{0xAB, 0xB2, 0xBF};
    //_background_color = sf::Color{0x28, 0x2C, 0x34};
    //_gridline_color   = sf::Color{0x38, 0x3C, 0x46};

    // boring light
    // _foreground_color = sf::Color{0x00, 0x00, 0x00};
    // _background_color = sf::Color{0xff, 0xff, 0xff};
    // _gridline_color   = sf::Color{0x44, 0x44, 0x44};

    // boring dark
    // _foreground_color = sf::Color{0xff, 0xff, 0xff};
    // _background_color = sf::Color{0x00, 0x00, 0x00};
    // _gridline_color   = sf::Color{0x22, 0x22, 0x22};

    // ubuntu
    // _foreground_color = sf::Color{0xaa, 0xaa, 0xaa};
    // _background_color = sf::Color{0x1b, 0x1b, 0x1b};
    // _gridline_color   = sf::Color{0x44, 0x44, 0x44};

    _aa_color_l = _gridline_color;
    _aa_color_r = _gridline_color;

    _grid_default_max_alpha = 255;
    _grid_max_alpha = _grid_default_max_alpha;
}

int Grid::initialize() {
    _window.create(sf::VideoMode(_screen_width, _screen_height), _title);
    _window.setFramerateLimit(_max_fps);

    _view = _window.getDefaultView();

    // vertices used for batch drawing multiple grid lines efficiently
    _vertex_array.setPrimitiveType(sf::Quads);

    _chunk_texture.create(_chunk_size, _chunk_size);
    _chunk_sprite.setTexture(_chunk_texture, true);

    // texture stored in graphics memory where each pixel is a cell
    int max_size = sf::Texture::getMaximumSize();
    if (max_size < _grid_texture_width || max_size < _grid_texture_height) {
        _grid_texture_width = max_size;
        _grid_texture_height = max_size;
    }
    if (_grid_texture_width < _grid_texture_height)
        _render_distance = _grid_texture_width / _chunk_size;
    else
        _render_distance = _grid_texture_height / _chunk_size;

    if (!_font.loadFromFile("NotoSansMono-Bold.ttf"))
        return 1;

    _max_cells_x = _grid_texture_width - 2;
    _max_cells_y = _grid_texture_height - 2;
    _grid_render_texture.create(_grid_texture_width, _grid_texture_height);
    _grid_render_texture.clear(_background_color);
    //_grid_texture = _grid_render_texture.getTexture();


    _grid_sprite.setTexture(_grid_render_texture.getTexture());

    // when the grid is panned beyond the bounds of the grid texture,
    // the cells are wrapped to the other side of the texture.
    // setting the texture to be repeated allows for the texture to automatically
    // wrap when rendering
    _grid_render_texture.setRepeated(true);

    // pixel which are manipulated before being used to update the grid texture
    // int n_pixels = _grid_texture_width * _grid_texture_height * 4;
    // _pixels = new sf::Uint8[n_pixels];
    // for (int i = 0; i < n_pixels; i++)
    //     _pixels[i] = 255;

    const char * shader_source =
        "#version 130\n"
        "uniform sampler2D tex;"
        "uniform float scale;"
        "uniform vec2 geometry;"
        "out vec4 frag_color;"
        "void main() {"
        "    vec2 pixel = gl_TexCoord[0].xy * geometry;"
        "    vec2 uv = floor(pixel) + 0.5;"
        "    uv += (1.0 - clamp((1.0 - fract(pixel)) * scale, 0.0, 1.0));"
        "    frag_color = texture(tex, uv / geometry);"
        "}";

    if (sf::Shader::isAvailable()) {
        _shader.loadFromMemory(shader_source, sf::Shader::Fragment);
        _shader.setUniform("tex", _grid_render_texture.getTexture());
        _shader.setUniform("geometry", sf::Vector2f(_grid_texture_width, _grid_texture_height));
        _antialias_enabled = true;
    }

    _grid_render_texture.setSmooth(_antialias_enabled);

    setGridlinesFade(7, 20);
    setGridThickness(0.08);

    return 0;
}

void Grid::mainloop() {
    // TODO: clean + comments

    float delta_time;

    while (_window.isOpen()) {
        delta_time = _clock.restart().asSeconds();
        handleEvents();

        if (_grid_fading) {
          applyGridFading(delta_time);
        }

        applyZoomVel(delta_time);
        applyPanVel(delta_time);

        if (_mouse_moved) {
            calculateTraversedCells();
            _mouse_moved = false;
        }

        if (_pan_button_pressed && _mouse_timer.getElapsedTime()
                .asSeconds() > _t_per_mouse_pos)
            recordMousePos();


        if (_grid_moved) {
            updateChunkQueue();
            _grid_moved = false;
        }
        updateChunks();

        if (_animated_cells.size()) 
          animateCells(delta_time);

        //if (_cell_draw_queue.size())
        //    drawCellQueue();

        if (_vertex_array.getVertexCount()) {
            _vertex_array.setPrimitiveType(sf::Points);
            _grid_render_texture.draw(_vertex_array);
            _vertex_array.clear();
        }

        //_window.clear(_background_color);
        render();
        renderText();

        if (_display_grid && _grid_thickness > 0) {
          if (_antialias_enabled)
            renderGridlinesAA();
          else
            renderGridlines();
        }
        _window.display();
        incrementTimer();
        _n_frames++;

    }
}
