#include <SFML/Graphics.hpp>
#include <iostream>
#include "grid.h"

void Grid::handleEvents() {
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

void Grid::onStart() {
    int x, y, radius;
    float density;

    for (int y = 5; y < 100; y += 5) {
        for (int x = 0; x < 100; x += 1) {
            drawCell(x, y, 100, 100, 100);
        }
    }
    for (int x = 5; x < 100; x += 5) {
        for (int y = 0; y < 100; y += 1) {
            drawCell(x, y, 100, 100, 100);
        }
    }

    // drawCell( 0,  0, 255, 255, 255);
    // drawCell( 1,  0, 128, 128, 128);
    // drawCell(-1,  0, 128, 128, 128);
    // drawCell( 0,  1, 128, 128, 128);
    // drawCell( 0, -1, 128, 128, 128);
    // drawCell( 1,  1,  64,  64,  64);
    // drawCell( 1, -1,  64,  64,  64);
    // drawCell(-1,  1,  64,  64,  64);
    // drawCell(-1, -1,  64,  64,  64);
    // drawCell( 2,  0,  32,  32,  32);
    // drawCell(-2,  0,  32,  32,  32);
    // drawCell( 0,  2,  32,  32,  32);
    // drawCell( 0, -2,  32,  32,  32);
}

void Grid::onResize(unsigned int new_width, unsigned int new_height) {
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

void Grid::onKeyRelease(int key_code) {
}

void Grid::onKeyPress(int key_code) {
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

void Grid::onWindowClose() {
    _window.close();
}

void Grid::onTimer() {
    if (_stress_test)
        std::cout << "FPS: " << _n_frames << std::endl;
    _n_frames = 0;
}
