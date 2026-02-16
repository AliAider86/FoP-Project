#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "engine.h"
#include "ui.h"

using namespace std;

int main(int argc, char* argv[])
{
    GameState game;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        cout << "SDL Init Error: " << SDL_GetError() << endl;
        return -1;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    SDL_Window* window = SDL_CreateWindow(
            "FoP Project",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            dm.w,
            dm.h,
            SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (!window)
    {
        cout << "Window Error: " << SDL_GetError() << endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        cout << "Renderer Error: " << SDL_GetError() << endl;
        return -1;
    }

    game.player.w = 100;
    game.player.h = 100;
    game.player.x = double(dm.w) / 2 - 50;
    game.player.y = double(dm.h) / 2 - 50;
    game.player.visible = true;
    game.screenWidth = dm.w;
    game.screenHeight = dm.h;
    game.runButton = {50, game.screenHeight - 100, 120, 50};
    game.pauseButton = {200, game.screenHeight - 100, 120, 50};
    game.stepButton = {350, game.screenHeight - 100, 120, 50};
    game.resetButton = {500, game.screenHeight - 100, 120, 50};
    game.program.push_back({FOREVER, 0, 0});
    game.program.push_back({MOVE_RIGHT, 5, 0});
    game.program.push_back({END_FOREVER, 0, 0});

    bool running = true;

    while (running)
    {
        handleEvents(running, game);
        update(game);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render(renderer, game);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
