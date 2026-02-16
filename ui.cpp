#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfx.h>

void handleEvents(bool &running, GameState& game)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            running = false;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            running = false;

        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            int mx = e.button.x;
            int my = e.button.y;

            if (mx >= game.runButton.x && mx <= game.runButton.x + game.runButton.w &&
                my >= game.runButton.y && my <= game.runButton.y + game.runButton.h)
                game.isRunningCode = true;

            if (mx >= game.pauseButton.x && mx <= game.pauseButton.x + game.pauseButton.w &&
                my >= game.pauseButton.y && my <= game.pauseButton.y + game.pauseButton.h)
                game.isRunningCode = false;

            if (mx >= game.stepButton.x && mx <= game.stepButton.x + game.stepButton.w &&
                my >= game.stepButton.y && my <= game.stepButton.y + game.stepButton.h)
            {
                game.stepMode = true;
                game.isRunningCode = true;
            }

            if (mx >= game.resetButton.x &&
                mx <= game.resetButton.x + game.resetButton.w &&
                my >= game.resetButton.y &&
                my <= game.resetButton.y + game.resetButton.h)
            {
                game.isRunningCode = false;
                game.currentBlockIndex = 0;
                game.isExecutingBlock = false;
                game.remainingMove = 0;

                game.repeatCountStack.clear();
                game.repeatStartStack.clear();

                game.player.x = game.screenWidth / 2 - game.player.w / 2;
                game.player.y = game.screenHeight / 2 - game.player.h / 2;
            }
        }
    }
}

void render(SDL_Renderer* renderer, GameState& game)
{
    int green, red, blue;

    if (game.stepMode)
    {
        blue = 220;
        green = 220;
        red = 80;
    }
    else if (game.isRunningCode)
    {
        blue = 80;
        green = 220;
        red = 80;
    }
    else
    {
        blue = 80;
        green = 80;
        red = 220;
    }

    SDL_Rect topBar = {0, 0, game.screenWidth, 40};

    int startX = 50;
    int startY = 80;
    int blockWidth = 250;
    int blockHeight = 40;
    int spacing = 10;

    for (int i = 0; i < game.program.size(); i++)
    {
        SDL_Rect blockRect = {
                startX,
                startY + i * (blockHeight + spacing),
                blockWidth,
                blockHeight
        };

        if (i == game.currentBlockIndex && game.isRunningCode)
            SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
        else
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);

        SDL_RenderFillRect(renderer, &blockRect);
    }

    if (game.isRunningCode)
        SDL_SetRenderDrawColor(renderer, 0, 180, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);

    SDL_RenderFillRect(renderer, &topBar);

    SDL_Rect r1 = {game.runButton.x, game.runButton.y, game.runButton.w, game.runButton.h};
    SDL_SetRenderDrawColor(renderer, 0, green, 0, 255);
    SDL_RenderFillRect(renderer, &r1);

    SDL_Rect r2 = {game.pauseButton.x, game.pauseButton.y, game.pauseButton.w, game.pauseButton.h};
    SDL_SetRenderDrawColor(renderer, red, 0, 0, 255);
    SDL_RenderFillRect(renderer, &r2);

    SDL_Rect r3 = {game.stepButton.x, game.stepButton.y, game.stepButton.w, game.stepButton.h};
    SDL_SetRenderDrawColor(renderer, 0, 0, blue, 255);
    SDL_RenderFillRect(renderer, &r3);

    if (!game.player.visible) return;

    SDL_Rect rect;
    rect.x = (int)game.player.x;
    rect.y = (int)game.player.y;
    rect.w = game.player.w;
    rect.h = game.player.h;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &rect);

    boxRGBA(renderer,
            game.resetButton.x + 3,
            game.resetButton.y + 3,
            game.resetButton.x + game.resetButton.w + 3,
            game.resetButton.y + game.resetButton.h + 3,
            90, 30, 30, 255);

    boxRGBA(renderer,
            game.resetButton.x,
            game.resetButton.y,
            game.resetButton.x + game.resetButton.w,
            game.resetButton.y + game.resetButton.h,
            139, 69, 19, 255);
}
