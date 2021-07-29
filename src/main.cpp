#include "grid.h"

int main() {
    Grid grid("Grid", 20, 20, 40);
    // grid.setGridThickness(6);
    // grid.useAntialiasing(false);
    // grid.setGridlinesFade(0, 0);
    // grid.setGridlineAlpha(50);
    return grid.start();
}
