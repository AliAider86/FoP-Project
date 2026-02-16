#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include "blocks.h"

struct Button
{
    int x, y, w, h;
};

struct Sprite
{
    double x, y;
    int w, h;
    bool visible;
};

struct GameState
{
    Sprite player;

    std::vector<Block> program;

    int currentBlockIndex = 0;

    bool isRunningCode = false;
    bool stepMode = false;

    double remainingMove = 0;
    bool isExecutingBlock = false;

    int repeatRemaining = 0;
    int repeatBlockIndex = -1;

    int screenWidth;
    int screenHeight;

    Button runButton;
    Button pauseButton;
    Button stepButton;
};

void update(GameState& game);

#endif
