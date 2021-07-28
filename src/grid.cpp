#include <SFML/Graphics.hpp>
#include <math.h>
#include "grid.h"

#define RAND() static_cast <float> (rand()) / static_cast <float> (RAND_MAX)

// rendering text
// cells are stored as structs
// cell.r cell.b cell.g cell.l
// cell.l is letter
// when cell is loaded and it has a letter, the letter and coords are stored in a vector
// when rendering, iterate over vector, draw all letters at coords. if coords are out of bounds, remove from vector

// TODO
// [x] only drawIntroducedCells() once per frame
// [x] comments + cleanup
// [x] split into multiple files (cleaner?)
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

void Grid::randomCircle(float center_x, float center_y, float radius, float density) {
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

int Grid::initialize() {
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

    _shader.loadFromMemory(shader_source, sf::Shader::Fragment);
    _shader.setUniform("tex", _grid_texture);
    _shader.setUniform("geometry", sf::Vector2f(_grid_texture_width, _grid_texture_height));

    return 0;
}

void Grid::mainloop() {
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
