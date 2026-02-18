#include <iostream>
#include <algorithm>
#include "engine.h"
#include "operators.h"

using namespace std;

void update(GameState& game)
{
    if (!game.isRunningCode)
        return;

    int safetyCounter = 0;
    const int MAX_OPERATIONS_PER_FRAME = 1000;

    // ===== پردازش رویدادها و فعال کردن اسکریپت‌های مربوطه =====

    // رویداد Green Flag
    if (game.greenFlagPressed)
    {
        game.greenFlagPressed = false;
        for (int i = 0; i < game.program.size(); i++)
        {
            if (game.program[i].type == WHEN_GREEN_FLAG)
            {
                game.scriptStartIndices.push_back(i + 1);
                game.scriptActive.push_back(true);
                game.scriptCurrentBlock.push_back(i + 1);
            }
        }
    }

    // رویداد کلیک روی اسپرایت
    if (game.spriteClicked)
    {
        game.spriteClicked = false;
        for (int i = 0; i < game.program.size(); i++)
        {
            if (game.program[i].type == WHEN_SPRITE_CLICKED)
            {
                game.scriptStartIndices.push_back(i + 1);
                game.scriptActive.push_back(true);
                game.scriptCurrentBlock.push_back(i + 1);
            }
        }
    }

    // رویداد کلید فشرده - نسخه اصلاح شده
    for (int i = 0; i < game.program.size(); i++)
    {
        if (game.program[i].type == WHEN_KEY_PRESSED)
        {
            int key = game.program[i].keyCode;
            if (game.pressedKeys[key])
            {
                // اول اسکریپت‌های قبلی این کلید رو پاک کن
                for (int j = game.scriptStartIndices.size() - 1; j >= 0; j--)
                {
                    int scriptStart = game.scriptStartIndices[j];
                    if (scriptStart > 0 && scriptStart - 1 < game.program.size())
                    {
                        if (game.program[scriptStart - 1].type == WHEN_KEY_PRESSED &&
                            game.program[scriptStart - 1].keyCode == key)
                        {
                            game.scriptStartIndices.erase(game.scriptStartIndices.begin() + j);
                            game.scriptActive.erase(game.scriptActive.begin() + j);
                            game.scriptCurrentBlock.erase(game.scriptCurrentBlock.begin() + j);
                        }
                    }
                }

                // بعد اسکریپت جدید اضافه کن
                game.scriptStartIndices.push_back(i + 1);
                game.scriptActive.push_back(true);
                game.scriptCurrentBlock.push_back(i + 1);
            }
        }
    }

    // ===== اجرای اسکریپت‌های فعال =====
    for (int s = 0; s < game.scriptStartIndices.size(); s++)
    {
        if (!game.scriptActive[s])
            continue;

        int& scriptPC = game.scriptCurrentBlock[s];

        if (scriptPC >= game.program.size())
        {
            game.scriptActive[s] = false;
            continue;
        }

        Block& b = game.program[scriptPC];

        // ===== بلوک‌های کنترلی =====
        if (b.type == REPEAT)
        {
            game.repeatCountStack.push_back(b.repeatCount);
            game.repeatStartStack.push_back(scriptPC + 1);
            scriptPC++;
            safetyCounter++;
            continue;
        }

        if (b.type == END_REPEAT)
        {
            if (!game.repeatCountStack.empty())
            {
                int& lastCount = game.repeatCountStack.back();
                lastCount--;

                if (lastCount > 0)
                {
                    scriptPC = game.repeatStartStack.back();
                }
                else
                {
                    game.repeatCountStack.pop_back();
                    game.repeatStartStack.pop_back();
                    scriptPC++;
                }
            }
            safetyCounter++;
            continue;
        }

        if (b.type == FOREVER)
        {
            game.repeatStartStack.push_back(scriptPC + 1);
            scriptPC++;
            continue;
        }

        if (b.type == END_FOREVER)
        {
            if (!game.repeatStartStack.empty())
            {
                scriptPC = game.repeatStartStack.back();
            }
            continue;
        }

        if (b.type == WAIT)
        {
            if (!game.isWaiting)
            {
                game.waitStartTime = SDL_GetTicks();
                if (!b.parameters.empty())
                    game.waitDuration = b.parameters[0].asNumber() * 1000;
                else
                    game.waitDuration = 1000;
                game.isWaiting = true;
                return;
            }

            if (SDL_GetTicks() - game.waitStartTime >= game.waitDuration)
            {
                game.isWaiting = false;
                scriptPC++;
            }
            return;
        }

        // ===== بلوک‌های ظاهری =====
        if (b.type == SAY || b.type == SAY_FOR || b.type == THINK || b.type == THINK_FOR)
        {
            if (b.type == SAY_FOR || b.type == THINK_FOR)
            {
                if (!game.isShowingMessage)
                {
                    if (!b.parameters.empty())
                    {
                        game.player.message = b.parameters[0].asString();
                        game.player.isThinking = (b.type == THINK_FOR);

                        if (b.parameters.size() >= 2)
                            game.messageDuration = b.parameters[1].asNumber() * 1000;
                        else
                            game.messageDuration = 2000;

                        game.messageStartTime = SDL_GetTicks();
                        game.isShowingMessage = true;
                    }
                    return;
                }

                if (SDL_GetTicks() - game.messageStartTime >= game.messageDuration)
                {
                    game.player.message = "";
                    game.isShowingMessage = false;
                    scriptPC++;
                }
                return;
            }
            else
            {
                if (!b.parameters.empty())
                {
                    game.player.message = b.parameters[0].asString();
                    game.player.isThinking = (b.type == THINK);
                }
                scriptPC++;
                safetyCounter++;
                continue;
            }
        }

        // ===== بلوک‌های عملگر =====
        if (b.type >= OP_ADD && b.type <= OP_XOR)
        {
            Value result = evaluateOperator(b.type, b.parameters);
            scriptPC++;
            safetyCounter++;
            continue;
        }

        // ===== بلوک‌های متغیر =====
        if (b.type == SET_VARIABLE)
        {
            if (!b.parameters.empty() && !b.variableName.empty())
            {
                game.variables[b.variableName] = b.parameters[0];
            }
            scriptPC++;
            safetyCounter++;
            continue;
        }

        if (b.type == CHANGE_VARIABLE)
        {
            if (!b.parameters.empty() && !b.variableName.empty())
            {
                if (game.variables.find(b.variableName) == game.variables.end())
                {
                    game.variables[b.variableName] = Value(0.0);
                }
                Value current = game.variables[b.variableName];
                double newVal = current.asNumber() + b.parameters[0].asNumber();
                game.variables[b.variableName] = Value(newVal);
            }
            scriptPC++;
            safetyCounter++;
            continue;
        }

        // ===== بلوک‌های قلم =====
        if (b.type == PEN_DOWN)
        {
            game.player.penDown = true;
            game.player.lastPenX = game.player.x + game.player.w/2;
            game.player.lastPenY = game.player.y + game.player.h/2;
            game.player.penMoved = true;
            scriptPC++;
            continue;
        }

        if (b.type == PEN_UP)
        {
            game.player.penDown = false;
            scriptPC++;
            continue;
        }

        if (b.type == SET_PEN_COLOR)
        {
            if (b.parameters.size() >= 3)
            {
                game.player.penR = (Uint8)b.parameters[0].asNumber();
                game.player.penG = (Uint8)b.parameters[1].asNumber();
                game.player.penB = (Uint8)b.parameters[2].asNumber();
            }
            scriptPC++;
            continue;
        }

        if (b.type == SET_PEN_SIZE)
        {
            if (!b.parameters.empty())
            {
                game.player.penSize = (int)b.parameters[0].asNumber();
                if (game.player.penSize < 1) game.player.penSize = 1;
            }
            scriptPC++;
            continue;
        }

        if (b.type == PEN_CLEAR)
        {
            game.penX1.clear();
            game.penY1.clear();
            game.penX2.clear();
            game.penY2.clear();
            game.penR_.clear();
            game.penG_.clear();
            game.penB_.clear();
            game.penSize_.clear();
            scriptPC++;
            continue;
        }

        scriptPC++;
        safetyCounter++;
    }

    // رسم خط اگر قلم پایین است - این بخش همیشه اجرا میشه
    if (game.player.penDown && game.player.penMoved)
    {
        double centerX = game.player.x + game.player.w/2;
        double centerY = game.player.y + game.player.h/2;

        // اگه حداقل یه پیکسل جابجا شده باشه
        if (abs(centerX - game.player.lastPenX) > 1 || abs(centerY - game.player.lastPenY) > 1)
        {
            game.penX1.push_back((int)game.player.lastPenX);
            game.penY1.push_back((int)game.player.lastPenY);
            game.penX2.push_back((int)centerX);
            game.penY2.push_back((int)centerY);
            game.penR_.push_back(game.player.penR);
            game.penG_.push_back(game.player.penG);
            game.penB_.push_back(game.player.penB);
            game.penSize_.push_back(game.player.penSize);

            game.player.lastPenX = centerX;
            game.player.lastPenY = centerY;
        }
    }

    // پاکسازی اسکریپت‌های غیرفعال
    for (int s = game.scriptActive.size() - 1; s >= 0; s--)
    {
        if (!game.scriptActive[s])
        {
            game.scriptStartIndices.erase(game.scriptStartIndices.begin() + s);
            game.scriptActive.erase(game.scriptActive.begin() + s);
            game.scriptCurrentBlock.erase(game.scriptCurrentBlock.begin() + s);
        }
    }
}