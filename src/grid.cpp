#include <SFML/Graphics.hpp>
#include <math.h>
#include <climits>
#include "grid.h"
#include "font.h"
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

Grid::Grid(const char * title, int n_cols, int n_rows, float scale) {
    _cam_x = 0;
    _cam_y = 0;

    _title = title;
    _screen_width = scale * n_cols;
    _screen_height = scale * n_rows;

    _mouse_cell_x = _cam_x;
    _mouse_cell_y = _cam_y;

    _grid_fading = 0;
    _grid_fade_duration = 0.15;

    _antialias_enabled = true;

    _grid_texture_size = 2048;

    _display_grid = true;
    _timer_interval = 1;

    _pan_button = sf::Mouse::Middle;
    _min_pan_vel = 0.01;
    _weak_pan_friction = 0.999;
    _strong_pan_friction = 0.999999;

    _scale = scale;
    _max_scale = 300;
    _min_scale = 2;
    _min_scale_cap = _min_scale;

    _zoom_friction = 0.995;
    _zoom_speed = 1;
    _pan_speed = 1;
    _min_zoom_vel = 0.01;
    _max_zoom_vel = 10.0;

    _decelerate_out_space = 1.0;
    _decelerate_in_space = 0.3;

    // default
    //_foreground_color = sf::Color{0x54, 0x5d, 0x66};
    //_background_color = sf::Color{0xd0, 0xd5, 0xde};
    //_gridline_color   = sf::Color{0x75, 0x83, 0x8e};

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
    _foreground_color = sf::Color{0xAB, 0xB2, 0xBF};
    _background_color = sf::Color{0x28, 0x2C, 0x34};
    _gridline_color   = sf::Color{0x38, 0x3C, 0x46};

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
    // texture stored in graphics memory where each pixel is a cell
    int max_size = sf::Texture::getMaximumSize();
    if (max_size < _grid_texture_size) {
        _grid_texture_size = max_size;
    }
    _render_distance = _grid_texture_size / CHUNK_SIZE;

    // at least render distance of 2 is required for rendering to work
    // since chunks are 128x128, the computer must support 256x256 textures
    // can't imagine a computer that doesn't support 256x256 texture
    if (_render_distance < 2)
        return 1;

    _window.create(sf::VideoMode(_screen_width, _screen_height), _title);
    setFPS(150);

    _view = _window.getDefaultView();

    // vertices used for batch drawing multiple grid lines efficiently
    _cell_vertexes.setPrimitiveType(sf::Points);
    _gridline_vertexes.setPrimitiveType(sf::Quads);

    _chunk_texture.create(CHUNK_SIZE, CHUNK_SIZE);
    _chunk_sprite.setTexture(_chunk_texture, true);

    _font.loadFromMemory(office_code_pro, office_code_pro_len);

    _grid_render_texture.create(_grid_texture_size, _grid_texture_size);
    _grid_render_texture.clear(_background_color);
    _grid_render_texture.setRepeated(true);
    _grid_sprite.setTexture(_grid_render_texture.getTexture());

    if (sf::Shader::isAvailable()) {
        _antialias_enabled = true;
        _shader.loadFromMemory(shader_source, sf::Shader::Fragment);
        _shader.setUniform("geometry", sf::Vector2f(_grid_texture_size, _grid_texture_size));
        _shader.setUniform("tex", _grid_render_texture.getTexture());
        _grid_render_texture.setSmooth(true);
    }

    setGridlinesFade(7, 20);
    setGridThickness(0.08);

    startThread();

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
            if (_pan_button_pressed) {
                int dx = _mouse_x - _new_mouse_x;
                int dy = _mouse_y - _new_mouse_y;
                pan(dx / _scale, dy / _scale);

                float dt = _mouse_timer.restart().asSeconds();

                if (!_mouse_vel_x)
                    _mouse_vel_x = dx / dt;
                else
                    _mouse_vel_x = (_mouse_vel_x + dx / dt) / 2.0;

                if (!_mouse_vel_y)
                    _mouse_vel_y = dy / dt;
                else
                    _mouse_vel_y = (_mouse_vel_y + dy / dt) / 2.0;

                _mouse_x = _new_mouse_x;
                _mouse_y = _new_mouse_y;
            }
            else {
                _mouse_x = _new_mouse_x;
                _mouse_y = _new_mouse_y;
                calculateTraversedCells();
            }
            _mouse_moved = false;
        }

        else if (_pan_button_pressed) {
            _mouse_vel_x = _mouse_vel_x / 2.0;
            _mouse_vel_y = _mouse_vel_y / 2.0;
        }

        switch (_thread_state) {
            case inactive:
                if (_animated_cells.size()) 
                    animateCells(delta_time);

                if (_cell_vertexes.getVertexCount()) {
                    _grid_render_texture.draw(_cell_vertexes);
                    _cell_vertexes.clear();
                }
                break;
            case swapping:
                if (_cell_vertexes.getVertexCount()) {
                    _grid_render_texture.draw(_cell_vertexes);
                    _cell_vertexes.clear();
                }

                _buffer_idx = !_buffer_idx;

                _chunk_render_left = _old_chunk_render_left;
                _chunk_render_right = _old_chunk_render_right;
                _chunk_render_top = _old_chunk_render_top;
                _chunk_render_bottom = _old_chunk_render_bottom;

                updateChunkQueue();
                _grid_moved = false;

                _thread_state = swapped;
                _cv.notify_one();
                break;
        }

        if (_grid_moved) {
            updateChunkQueue();
            _grid_moved = false;
        }
        updateChunks();

        //_window.clear(_background_color);
        render();
        renderText();

        if (_display_grid && _grid_thickness > 0) {
          if (_antialias_enabled)
            renderGridlinesAA();
          else
            renderGridlines();
        }

        _n_frames++;
        incrementTimer();

        _window.display();
        if (_fps_clock.getElapsedTime().asSeconds() >= 1) {
            std::cout << "FPS: " << _n_frames << std::endl;
            _n_frames = 0;
            _n_iterations = 0;
            _fps_clock.restart();
        }

    }
}
