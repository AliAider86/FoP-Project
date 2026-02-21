#ifndef LOOKS_H
#define LOOKS_H

#include "engine.h"
#include "blocks.h"
#include <SDL2/SDL.h>
#include "logger.h"

using namespace std;

void executeLooksBlock(Block& b, GameState& game, SDL_Renderer* renderer)
{
    // دریافت اسپرایت فعال
    Sprite* activeSprite = nullptr;
    if (game.activeSpriteIndex >= 0 && game.activeSpriteIndex < game.sprites.size())
    {
        activeSprite = &game.sprites[game.activeSpriteIndex];
    }
    else
    {
        return; // اسپرایت فعالی وجود ندارد
    }

    switch (b.type)
    {
        case SAY:
        case SAY_FOR:
        case THINK:
        case THINK_FOR:
        {
            if (!b.parameters.empty())
            {
                activeSprite->message = b.parameters[0].asString();
                activeSprite->isThinking = (b.type == THINK || b.type == THINK_FOR);
                game.messageStartTime = SDL_GetTicks();

                if (b.type == SAY_FOR || b.type == THINK_FOR)
                {
                    if (b.parameters.size() >= 2)
                        game.messageDuration = b.parameters[1].asNumber() * 1000;
                    else
                        game.messageDuration = 2000;

                    game.isShowingMessage = true;
                }
                else
                {
                    game.messageDuration = 0;
                }

                string msgType = activeSprite->isThinking ? "THINK" : "SAY";
                log_info((msgType + ": " + activeSprite->message).c_str());
            }
            break;
        }

        case SHOW:
            activeSprite->visible = true;
            log_info(("Sprite shown: " + activeSprite->name).c_str());
            break;

        case HIDE:
            activeSprite->visible = false;
            log_info(("Sprite hidden: " + activeSprite->name).c_str());
            break;

        case CHANGE_SIZE:
            if (!b.parameters.empty())
            {
                double percent = b.parameters[0].asNumber();
                int oldW = activeSprite->w;
                int oldH = activeSprite->h;

                activeSprite->w = (int)(activeSprite->w * (1 + percent/100));
                activeSprite->h = (int)(activeSprite->h * (1 + percent/100));

                if (activeSprite->w < 5) activeSprite->w = 5;
                if (activeSprite->h < 5) activeSprite->h = 5;
            }
            break;

        case SET_SIZE:
            if (!b.parameters.empty())
            {
                double percent = b.parameters[0].asNumber();
                int newSize = (int)(percent);
                if (newSize < 5) newSize = 5;

                activeSprite->w = newSize;
                activeSprite->h = newSize;
            }
            break;

        default:
            break;
    }
}

#endif