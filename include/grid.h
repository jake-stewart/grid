#ifndef GRID_H
#define GRID_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "robin_hood.h"
#include <thread>
#include <mutex>
#include <condition_variable>



const static int _chunk_size = 256;


enum ThreadState {
    inactive,
    active,
    swapping,
    swapped,
    joining
};

struct QueuedCell
{
    int x;
    int y;
    sf::Color color;
};

struct Letter
{
    char letter;
    sf::Color color;
    int style;
};


struct Coord
{
    int x;
    int y;
 
    // constructor
    Coord(int x, int y)
    {
        this->x = x;
        this->y = y;
    }
 
    // `operator==` is required to compare keys in case of a hash collision
    bool operator==(const Coord &coord) const {
        return x == coord.x && y == coord.y;
    }
};
 
struct Chunk
{
    sf::Uint8 pixels [_chunk_size * _chunk_size * 4];
    robin_hood::unordered_map<uint64_t, Letter> letters;
};

// The specialized hash function for `unordered_map` keys
struct hash_fn
{
    std::size_t operator() (const Coord &node) const
    {
        std::size_t h1 = std::hash<int>()(node.x);
        std::size_t h2 = std::hash<int>()(node.y);
 
        return h1 ^ h2;
    }
};

struct AnimatedCell {
    sf::Color start_color;
    sf::Color end_color;
    float anim_progress;
    float anim_duration;
};

class Grid {
public:
    // interface.cpp
    Grid(const char * title, int n_cols, int n_rows, float scale);
    int start();
	void setFPS(int fps);
    void drawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g);
    void drawCell(int x, int y, sf::Color color);

    void threadDrawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g);
    void threadDrawCell(int x, int y, sf::Color color);
    void drawCellQueue();

    int _queue_max_idx = 0;
    int _current_queue_idx = 0;
    std::vector<QueuedCell> _cell_draw_queue;

    sf::Color getCell(int x, int y);
    void setScale(float scale);
    void setGridThickness(float value);
    void setGridlinesFade(float start_scale, float end_scale);
    void setGridlineAlpha(int alpha);
    void toggleGridlines();
    void useAntialiasing(bool value);

    // animations.cpp
    void drawCell(int x, int y, sf::Uint8 r, sf::Uint8 b, sf::Uint8 g, float anim_duration);
    void drawCell(int x, int y, sf::Color, float anim_duration);

private:
    int _grid_fading;
    float _grid_fade_duration;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    ThreadState _thread_state = inactive;

    bool _kill_thread = false;
    bool _thread_running = false;

    bool _antialias_enabled;

    const char * _title;

    robin_hood::unordered_map<Coord, AnimatedCell, hash_fn> _animated_cells;

    sf::Shader _shader;

    int _max_chunk;

    int _grid_texture_width;
    int _grid_texture_height;
    int _max_cells_x;
    int _max_cells_y;

    // mouse button used for panning/dragging screen
    int _pan_button = -1;
    bool _pan_button_pressed = false;

    // TODO: rename
    int _fill_x;
    bool _mouse_moved = false;
    int _mouse_cell_x = 0;
    int _mouse_cell_y = 0;

    bool _screen_changed = true;

    // last mouse location on screen
    int _new_mouse_x = 0;
    int _new_mouse_y = 0;
    int _mouse_x = 0;
    int _mouse_y = 0;

    int _screen_width;
    int _screen_height;

	float _frame_duration;

    // screen's top left corner coordinates of grid
    int _cam_x = 0;
    int _cam_y = 0;
    float _cam_x_decimal = 0;
    float _cam_y_decimal = 0;
    float _blit_x_offset;
    float _blit_y_offset;

    int _col_start = 0;
    int _col_end = 0;
    int _row_start = 0;
    int _row_end = 0;

    float _pan_vel_x = 0;
    float _pan_vel_y = 0;

    // pan velocity decreases faster once the user has clicked the screen
    // to drag again. stopping the velocity instantly can be jarring
    float _weak_pan_friction;
    float _strong_pan_friction;
    float _pan_friction;

    float _min_pan_vel;

    bool _grid_moved;

    float _scale;
    float _max_scale;
    float _min_scale;
    float _min_scale_cap;
	float _decelerate_out_space;
	float _decelerate_in_space;

    float _zoom_vel = 0;
    int _zoom_x = 0;
    int _zoom_y = 0;

    float _zoom_friction;
    float _heavy_zoom_friction;
    float _zoom_speed;
    float _pan_speed;
    float _min_zoom_vel;
    float _max_zoom_vel;

    void updateChunkQueue();
    void updateChunks();
    void drawScreen();

    sf::Texture _chunk_texture;
    sf::Sprite _chunk_sprite;

    int _n_frames = 0;
    int _n_iterations = 0;
    int _render_distance;
    int _chunk_x_cell = 0;
    int _chunk_y_cell = 0;
    int _chunk_x = 0;
    int _chunk_y = 0;
    int _n_chunks_width = 0;
    int _n_chunks_height = 0;
    robin_hood::unordered_map<uint64_t, Chunk> _chunks[2];
    bool _buffer_idx = 0;
    std::vector<std::pair<int, int>> _chunk_queue;

    int _chunk_render_left = 0;
    int _chunk_render_right = 0;
    int _chunk_render_top = 0;
    int _chunk_render_bottom = 0;

    int _old_chunk_render_left = 0;
    int _old_chunk_render_right = 0;
    int _old_chunk_render_top = 0;
    int _old_chunk_render_bottom = 0;

    // a pixel buffer for drawing rows/columns. these pixels are used to update the grid texture
    // the grid texture is located in graphics memory, so the pixels should not be edited directly,
    // since calls are slow.
    // sf::Uint8 * _pixels;

    sf::Color _background_color;
    sf::Color _foreground_color;
    sf::Color _gridline_color;

    sf::Color _aa_color_l;
    sf::Color _aa_color_r;

    float _start_offset;
    float _end_offset;

    bool _display_grid;
    float _grid_thickness;
    bool _gridline_shrink;

    float _cell_gridline_ratio;
    int _grid_default_max_alpha;
    int _grid_max_alpha;
    int _fade_end_scale;
    int _fade_start_scale;

    // SFML window
    sf::RenderWindow _window;
    sf::View _view;

    // timer that keeps track of the user's timer.
    // each time this timer ticks, onTimer() is called
    sf::Clock _clock, _timer, _fps_clock;
    float _timer_interval;
    bool _lagging = false;
    bool _timer_active = false;
    bool _thread_active;

    sf::Clock _mouse_timer;
    float _mouse_dt;
    float _mouse_vel_x = 0;
    float _mouse_vel_y = 0;

    sf::RenderTexture _grid_render_texture;
    sf::Sprite _grid_sprite;

    sf::VertexArray _cell_vertexes;
    sf::VertexArray _gridline_vertexes;

    int initialize();
    void mainloop();


    // gridlines.cpp
    void calcGridThickness();
    void applyGridEffects();
    void applyGridFading(float delta_time);
    void renderGridlines();
    void calcGridlineAA(float p);
    void renderGridlinesAA();

    // grid.cpp
    sf::Font _font;
    void render();
    void addText(int x, int y, std::string text, sf::Color color, int style);
    void renderText();

    // events.cpp
    void handleEvents();
    void onResize(unsigned int new_width, unsigned int new_height);
    void onKeyPress(int key_code);
    void onWindowClose();

    // zoom.cpp
    void zoom(float factor, int x, int y);
    void applyZoomVel(float delta_time);
    float decelerateOut(float delta_time);
    float decelerateIn(float delta_time);

    // timer.cpp
    void startTimer();
    void setTimer(float timer_interval);
    void stopTimer();
    void incrementTimer();
    void endThread();
    void startThread();
    void threadFunc();

    // pan.cpp
    void pan(float x, float y);
    void applyPanVel(float delta_time);

    // mouse.cpp
    void onMouseMotion(int x, int y);
    void onMousePress(int button);
    void onMouseRelease(int button);
    void onMouseScroll(int wheel, float delta);
    void calculateTraversedCells();
    void onPanButtonPress();
    void onPanButtonRelease();

    // animations.cpp
    void animateCells(float delta_time);
    void finishAnimations();

    // main.cpp
    void onMouseDragEvent(int cell_x, int cell_y);
    void onMousePressEvent(int x, int y, int button);
    void onMouseReleaseEvent(int x, int y, int button);
    void onKeyPressEvent(int key_code);
    void onKeyReleaseEvent(int key_code);
    void onStartEvent();
    void onTimerEvent(int n_iterations);
};

#endif
