#ifndef GRID_H
#define GRID_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <math.h>
#include <thread>

struct QueuedCell
{
    int x;
    int y;
    sf::Color color;
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
    bool _thread_running = false;
    bool _start_thread = false;
    bool _thread_finished = true;

    bool _antialias_enabled;

    const char * _title;

    std::unordered_map<Coord, AnimatedCell, hash_fn> _animated_cells;

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
    int _mouse_cell_x = 0;
    int _mouse_cell_y = 0;

	bool _screen_changed = true;

    // last mouse location on screen
    int _mouse_x = 0;
    int _mouse_y = 0;

    // visible row geometry
    int _n_visible_rows = 0;
    int _n_visible_cols = 0;
    int _screen_width;
    int _screen_height;

    int _max_fps;

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

    // how zoomed in grid is. eg. scale 1.5 means each cell is 1.5 pixels.
    // min and max scale determine cell size limits. cell size < 1 is possible but
    // causes jitteriness when panning (also very computationally expensive because
    // of how many cells have to be drawn)
    float _scale;
    float _max_scale;
    float _min_scale;

    float _zoom_vel = 0;
    int _zoom_x = 0;
    int _zoom_y = 0;

    float _zoom_friction;
    float _zoom_speed;
    float _min_zoom_vel;

    // when you zoom beyond the min/max cell size, the scale bounces back
    // these are the values used for the bezier curve
    float _zoom_bounce_p0;
    float _zoom_bounce_p1;
    float _zoom_bounce_p2;
    float _zoom_bounce_t;
    float _zoom_bounce_duration;

    // if you zoom in/out while the scale is bouncing back,
    // the bezier curve will need to be recalculated
    bool _zoom_bounce_broken = true;

    // cells in grid are stored in both a rows and columns map
    // this makes it more efficient when drawing entire rows and entire columns
    // _rows[chunk_index][x] = {r, g, b, a}
    // _column[chunk_index][y] = {r, g, b, a}
    // when drawing a row or column, the chunks are iterated over, instead of individual rows.
    // then, each cell of the chunk is iterated over. this allows for empty pixels to be ignored.

    //std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, sf::Color>>> _columns;
    //std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, sf::Color>>> _rows;
    void updateChunkQueue();
    void updateChunks();
    void copyCellDrawQueue();
    void drawScreen();

    sf::Texture _chunk_texture;
    sf::Sprite _chunk_sprite;
    int _n_frames = 0;
    const static int _chunk_size = 64;
    int _render_distance;
    int _chunk_x_cell = 0;
    int _chunk_y_cell = 0;
    int _chunk_x = 0;
    int _chunk_y = 0;
    std::unordered_map<uint64_t, sf::Uint8[_chunk_size * _chunk_size * 4]> _chunks;
    std::unordered_map<uint64_t, sf::Uint8[_chunk_size * _chunk_size * 4]> _chunks_buffer;
    std::unordered_map<uint64_t, sf::Uint8[_chunk_size * _chunk_size * 4]> * _chunks_pointer = &_chunks;
    std::unordered_map<uint64_t, sf::Uint8[_chunk_size * _chunk_size * 4]> * _thread_chunks_pointer = &_chunks_buffer;
    std::vector<std::pair<int, int>> _chunk_queue;

    float _frame_duration;

    int _chunk_render_left = 0;
    int _chunk_render_right = 0;
    int _chunk_render_top = 0;
    int _chunk_render_bottom = 0;


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
    sf::Clock _clock, _timer;
    float _timer_interval;
    bool _timer_active = false;

    sf::Clock _mouse_timer;
    float _t_per_mouse_pos;
    static const int _n_mouse_positions = 4;
    int _mouse_x_positions [_n_mouse_positions];
    int _mouse_y_positions [_n_mouse_positions];
    int _mouse_pos_idx = 0;

    // grid texture stored on graphics card for quicker rendering
    sf::RenderTexture _grid_render_texture;

    sf::VertexArray _vertex_array;

    // sprites simply wrap textures and are used for rendering instead
    // of using the textures themselves.
    sf::Sprite _grid_sprite;


    void randomCircle(float center_x, float center_y, float radius, float density);



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
    void render();

    // events.cpp
    void handleEvents();
    void onResize(unsigned int new_width, unsigned int new_height);
    void onKeyPress(int key_code);
    void onWindowClose();

    // zoom.cpp
    void zoom(float factor, int x, int y);
    void applyZoomVel(float delta_time);
    void createZoomBounce();
    void applyZoomBounce(float delta_time);

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
    void calculateTraversedCells(int target_x, int target_y);
    void recordMousePos();
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
    void onTimerEvent();
};

#endif
