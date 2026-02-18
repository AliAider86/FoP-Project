#ifndef LOOKS_H
#define LOOKS_H

#include "engine.h"
#include "blocks.h"
#include <SDL2/SDL.h>

using namespace std;

void executeLooksBlock(Block& b, GameState& game, SDL_Renderer* renderer)
{
    switch (b.type)
    {
        case SAY:
        case SAY_FOR:
        case THINK:
        case THINK_FOR:
        {
            if (!b.parameters.empty())
            {
                game.player.message = b.parameters[0].asString();
                game.player.isThinking = (b.type == THINK || b.type == THINK_FOR);

                // اینجا از GameState استفاده کن نه Sprite
                game.messageStartTime = SDL_GetTicks();

                if (b.type == SAY_FOR || b.type == THINK_FOR)
                {
                    if (b.parameters.size() >= 2)
                        game.messageDuration = b.parameters[1].asNumber() * 1000;
                    else
                        game.messageDuration = 2000; // پیش‌فرض 2 ثانیه

                    game.isShowingMessage = true;
                }
                else
                {
                    game.messageDuration = 0; // تا ابد
                }
            }
            break;
        }

        case SHOW:
            game.player.visible = true;
            break;

        case HIDE:
            game.player.visible = false;
            break;

        case CHANGE_SIZE:
            if (!b.parameters.empty())
            {
                double percent = b.parameters[0].asNumber();
                game.player.w = (int)(game.player.w * (1 + percent/100));
                game.player.h = (int)(game.player.h * (1 + percent/100));
            }
            break;

        case SET_SIZE:
            if (!b.parameters.empty())
            {
                double percent = b.parameters[0].asNumber();
                // اندازه اصلی رو باید یه جایی ذخیره کنی
                // فعلاً فرض می‌کنیم 100% همون اندازه فعلیه
                game.player.w = 100;  // اینو بعداً درستش می‌کنیم
                game.player.h = 100;
            }
            break;

            // بقیه دستورات رو بعداً اضافه می‌کنیم

        default:
            break;
    }
}

#endif