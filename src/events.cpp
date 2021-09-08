#include <SFML/Graphics.hpp>
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
                onKeyReleaseEvent(event.key.code);
                break;

            default:
                break;
        }
    }
}

void Grid::onResize(unsigned int new_width, unsigned int new_height) {
    _view.setSize({
        static_cast<float>(new_width),
        static_cast<float>(new_height)
    });
    _view.setCenter(new_width / 2.0, new_height / 2.0);
    _window.setView(_view);

    _screen_width = new_width;
    _screen_height = new_height;

	float min_scale_width = _screen_width / ((_render_distance - 1) * _chunk_size);
	float min_scale_height = _screen_height / ((_render_distance - 1) * _chunk_size);

	float smallest_scale = (min_scale_width > min_scale_height) ? min_scale_width : min_scale_height;

	_min_scale = (smallest_scale < _min_scale_cap) ? _min_scale_cap : smallest_scale;

    setScale(_scale);
}

void Grid::onKeyPress(int key_code) {
    switch (key_code)
    {
        case sf::Keyboard::F:
            useAntialiasing(!_antialias_enabled);
            break;

        case sf::Keyboard::G:
            toggleGridlines();
            break;

        default:
            onKeyPressEvent(key_code);
            break;
    }
}

void Grid::onWindowClose() {
    endThread();
    _window.close();
}
