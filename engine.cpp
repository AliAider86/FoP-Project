#include <iostream>
#include "engine.h"
#include <algorithm>

void update(GameState& game)
{
    if (!game.isRunningCode)
        return;

    const double speed = 2.0;
    int safetyCounter = 0;
    const int MAX_OPERATIONS_PER_FRAME = 1000;

    while (safetyCounter < MAX_OPERATIONS_PER_FRAME)
    {
        if (game.currentBlockIndex >= game.program.size())
        {
            game.isRunningCode = false;
            break;
        }

        Block& b = game.program[game.currentBlockIndex];

        if (b.type == REPEAT)
        {
            game.repeatCountStack.push_back(b.repeatCount);
            game.repeatStartStack.push_back(game.currentBlockIndex + 1);
            game.currentBlockIndex++;
            safetyCounter++;
            continue;
        }

        if (b.type == END_REPEAT)
        {
            if (!game.repeatCountStack.empty())
            {
                game.repeatCountStack.back()--;

                if (game.repeatCountStack.back() > 0)
                {
                    game.currentBlockIndex =
                            game.repeatStartStack.back();
                }
                else
                {
                    game.repeatCountStack.pop_back();
                    game.repeatStartStack.pop_back();
                    game.currentBlockIndex++;
                }
            }
            safetyCounter++;
            continue;
        }

        if (b.type == FOREVER)
        {
            game.repeatStartStack.push_back(game.currentBlockIndex + 1);
            game.currentBlockIndex++;
            return;
        }

        if (b.type == END_FOREVER)
        {
            if (!game.repeatStartStack.empty())
            {
                game.currentBlockIndex = game.repeatStartStack.back();
            }
            return;
        }

        if (b.type == WAIT)
        {
            if (!game.isWaiting)
            {
                game.waitStartTime = SDL_GetTicks();
                game.waitDuration = b.value * 1000; // ثانیه
                game.isWaiting = true;
            }

            if (SDL_GetTicks() - game.waitStartTime >= game.waitDuration)
            {
                game.isWaiting = false;
                game.currentBlockIndex++;
            }

            return;
        }

        if (!game.isExecutingBlock)
        {
            game.remainingMove = b.value;
            game.isExecutingBlock = true;
        }

        double moveAmount =
                min(speed, game.remainingMove);

        switch (b.type)
        {
            case MOVE_UP:    game.player.y -= moveAmount; break;
            case MOVE_DOWN:  game.player.y += moveAmount; break;
            case MOVE_LEFT:  game.player.x -= moveAmount; break;
            case MOVE_RIGHT: game.player.x += moveAmount; break;
            default: break;
        }

        if (game.player.x < 0)
            game.player.x = 0;

        if (game.player.y < 0)
            game.player.y = 0;

        if (game.player.x + game.player.w > game.screenWidth)
            game.player.x = game.screenWidth - game.player.w;

        if (game.player.y + game.player.h > game.screenHeight)
            game.player.y = game.screenHeight - game.player.h;

        game.remainingMove -= moveAmount;

        if (game.remainingMove <= 0)
        {
            game.isExecutingBlock = false;
            game.currentBlockIndex++;
        }

        break;
    }

    if (safetyCounter >= MAX_OPERATIONS_PER_FRAME)
    {
        game.isRunningCode = false;
        std::cout << "Infinite loop prevented!\n";
    }
}

