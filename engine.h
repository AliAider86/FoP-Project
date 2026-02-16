#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include "blocks.h"
#include <SDL2/SDL.h>

using namespace std;

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

    vector<Block> program;

    int currentBlockIndex = 0;

    bool isRunningCode = false;
    bool stepMode = false;

    double remainingMove = 0;
    bool isExecutingBlock = false;

    int screenWidth;
    int screenHeight;

    Button runButton;
    Button pauseButton;
    Button stepButton;

    vector<int> repeatCountStack;
    vector<int> repeatStartStack;

    SDL_Rect resetButton;

    Uint32 waitStartTime;
    Uint32 waitDuration;
    bool isWaiting;

};

void update(GameState& game);

#endif
