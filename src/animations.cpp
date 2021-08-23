#include <SFML/Graphics.hpp>
#include "grid.h"
#include <unordered_map>

using std::unordered_map;

sf::Color colorMix(sf::Color a, sf::Color b, float perc) {
    float alt_perc = 1.0 - perc;

    return sf::Color {
        sf::Uint8(a.r * perc + b.r * alt_perc),
        sf::Uint8(a.g * perc + b.g * alt_perc),
        sf::Uint8(a.b * perc + b.b * alt_perc)
    };
}

void Grid::finishAnimations() {
    for (auto it: _animated_cells) {
        drawCell(
            it.first.x,
            it.first.y,
            it.second.end_color
        );
    }
    _animated_cells.clear();
}

void Grid::animateCells(float delta_time) {
    float progress;

    for (auto it = _animated_cells.begin(); it != _animated_cells.end();) {
        it->second.anim_progress += delta_time;

        // fade completed?
        if (it->second.anim_progress >= it->second.anim_duration) {
            drawCell(
                it->first.x,
                it->first.y,
                it->second.end_color
            );
            it = _animated_cells.erase(it);
            continue;
        }
        
        progress = it->second.anim_progress /
            it->second.anim_duration;

        drawCell(
            it->first.x,
            it->first.y,
            colorMix(
                it->second.end_color,
                it->second.start_color,
                progress
            )
        );
        it++;
    }
}

void Grid::drawCell(int x, int y, sf::Uint8 r, sf::Uint8 g, sf::Uint8 b, float anim_duration) {
    drawCell(x, y, sf::Color{r, g, b}, anim_duration);
}

void Grid::drawCell(int x, int y, sf::Color end_color, float anim_duration) {
    sf::Color start_color = getCell(x, y);

    auto it = _animated_cells.find(Coord(x, y));

    if (it == _animated_cells.end()) {
        _animated_cells[Coord(x, y)] = AnimatedCell{
            start_color, end_color,
            0, anim_duration
        };
    }
    else if (it->second.start_color == end_color) {
        it->second.start_color = it->second.end_color;
        it->second.end_color = end_color;
        it->second.anim_progress = (1.0 - (it->second.anim_progress / it->second.anim_duration)) * anim_duration;
        it->second.anim_duration = anim_duration;
    }
    else if (it->second.end_color == end_color) {
        it->second.anim_progress = (it->second.anim_progress / it->second.anim_duration) * anim_duration;
        it->second.anim_duration = anim_duration;
    }
    else if (start_color != end_color) {
        it->second.start_color = start_color;
        it->second.end_color = end_color;
        it->second.anim_progress = 0;
        it->second.anim_duration = anim_duration;
    }
}

