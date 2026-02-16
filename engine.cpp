#include "engine.h"
#include <algorithm>

void update(GameState& game)
{
    if (!game.isRunningCode)
        return;

    const double speed = 2.0;

    if (game.currentBlockIndex >= game.program.size())
    {
        game.isRunningCode = false;
        return;
    }

    Block& b = game.program[game.currentBlockIndex];

    if (b.type == REPEAT)
    {
        game.repeatRemaining = b.repeatCount;
        game.repeatBlockIndex = game.currentBlockIndex + 1;
        game.currentBlockIndex++;
        return;
    }

    if (!game.isExecutingBlock)
    {
        game.remainingMove = b.value;
        game.isExecutingBlock = true;
    }

    double moveAmount = std::min(speed, game.remainingMove);

    switch (b.type)
    {
        case MOVE_UP:
            game.player.y -= moveAmount;
            break;

        case MOVE_DOWN:
            game.player.y += moveAmount;
            break;

        case MOVE_LEFT:
            game.player.x -= moveAmount;
            break;

        case MOVE_RIGHT:
            game.player.x += moveAmount;
            break;

        default:
            break;
    }

    game.remainingMove -= moveAmount;

    if (game.remainingMove <= 0)
    {
        game.isExecutingBlock = false;

        if (game.repeatRemaining > 0)
        {
            game.repeatRemaining--;

            if (game.repeatRemaining > 0)
                game.currentBlockIndex = game.repeatBlockIndex;
            else
                game.currentBlockIndex++;
        }
        else
        {
            game.currentBlockIndex++;
        }

        if (game.stepMode)
        {
            game.isRunningCode = false;
            game.stepMode = false;
        }
    }
}
