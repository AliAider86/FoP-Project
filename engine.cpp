#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "engine.h"
#include "operators.h"

using namespace std;

void saveProject(const GameState& game, const string& filename)
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cout << "Error: Could not save file!" << endl;
        return;
    }

    file << "Sprite " << game.player.x << " " << game.player.y << " "
         << game.player.w << " " << game.player.h << " "
         << game.player.visible << " " << game.player.direction << endl;

    file << "Blocks " << game.program.size() << endl;
    for (const Block& b : game.program)
    {
        file << (int)b.type << " " << b.parameters.size() << " ";
        for (const Value& v : b.parameters)
        {
            if (v.type == VALUE_NUMBER)
                file << "num " << v.asNumber() << " ";
            else if (v.type == VALUE_STRING)
                file << "str " << v.asString() << " ";
            else if (v.type == VALUE_BOOLEAN)
                file << "bool " << v.asBoolean() << " ";
        }
        file << b.repeatCount << " " << b.variableName << " "
             << b.eventName << " " << b.keyCode << endl;
    }

    file << "Variables " << game.variables.size() << endl;
    for (const auto& var : game.variables)
    {
        file << var.first << " ";
        if (var.second.type == VALUE_NUMBER)
            file << "num " << var.second.asNumber() << endl;
        else if (var.second.type == VALUE_STRING)
            file << "str " << var.second.asString() << endl;
        else if (var.second.type == VALUE_BOOLEAN)
            file << "bool " << var.second.asBoolean() << endl;
    }

    file.close();
    cout << "Project saved to " << filename << endl;
}

void loadProject(GameState& game, const string& filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Error: Could not load file!" << endl;
        return;
    }

    game.program.clear();
    game.variables.clear();
    game.scriptStartIndices.clear();
    game.scriptActive.clear();
    game.scriptCurrentBlock.clear();
    game.penX1.clear();
    game.penY1.clear();
    game.penX2.clear();
    game.penY2.clear();

    string token;
    while (file >> token)
    {
        if (token == "Sprite")
        {
            file >> game.player.x >> game.player.y
                 >> game.player.w >> game.player.h
                 >> game.player.visible >> game.player.direction;
        }
        else if (token == "Blocks")
        {
            int count;
            file >> count;
            for (int i = 0; i < count; i++)
            {
                Block b;
                int typeInt, paramCount;
                file >> typeInt >> paramCount;
                b.type = (BlockType)typeInt;

                for (int j = 0; j < paramCount; j++)
                {
                    string valType;
                    file >> valType;
                    if (valType == "num")
                    {
                        double d;
                        file >> d;
                        b.parameters.push_back(Value(d));
                    }
                    else if (valType == "str")
                    {
                        string s;
                        file >> s;
                        b.parameters.push_back(Value(s));
                    }
                    else if (valType == "bool")
                    {
                        bool bVal;
                        file >> bVal;
                        b.parameters.push_back(Value(bVal));
                    }
                }
                file >> b.repeatCount >> b.variableName >> b.eventName >> b.keyCode;
                game.program.push_back(b);
            }
        }
        else if (token == "Variables")
        {
            int count;
            file >> count;
            for (int i = 0; i < count; i++)
            {
                string varName, valType;
                file >> varName >> valType;
                if (valType == "num")
                {
                    double d;
                    file >> d;
                    game.variables[varName] = Value(d);
                }
                else if (valType == "str")
                {
                    string s;
                    file >> s;
                    game.variables[varName] = Value(s);
                }
                else if (valType == "bool")
                {
                    bool b;
                    file >> b;
                    game.variables[varName] = Value(b);
                }
            }
        }
    }

    file.close();
    cout << "Project loaded from " << filename << endl;
}

void update(GameState& game)
{
//    cout << "Update called, isRunningCode = " << game.isRunningCode << endl;
    if (!game.isRunningCode)
        return;

    const double speed = 2.0;
    int safetyCounter = 0;
    const int MAX_OPERATIONS_PER_FRAME = 1000;

// رویداد Green Flag
    if (game.greenFlagPressed)
    {
        game.greenFlagPressed = false;

        // پاک کردن اسکریپت‌های قبلی
        game.scriptStartIndices.clear();
        game.scriptActive.clear();
        game.scriptCurrentBlock.clear();

        // ایجاد یک اسکریپت که از اولین بلوک شروع میشه
        game.scriptStartIndices.push_back(0);
        game.scriptActive.push_back(true);
        game.scriptCurrentBlock.push_back(0);
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

    // رویداد کلید فشرده
    for (int i = 0; i < game.program.size(); i++)
    {
        if (game.program[i].type == WHEN_KEY_PRESSED)
        {
            int key = game.program[i].keyCode;
            if (game.pressedThisFrame[key])
            {
                bool alreadyActive = false;
                for (int j = 0; j < game.scriptStartIndices.size(); j++)
                {
                    if (game.scriptStartIndices[j] == i + 1)
                    {
                        alreadyActive = true;
                        break;
                    }
                }

                if (!alreadyActive)
                {
                    game.scriptStartIndices.push_back(i + 1);
                    game.scriptActive.push_back(true);
                    game.scriptCurrentBlock.push_back(i + 1);
                }
            }
        }
    }

    // اجرای اسکریپت‌های فعال
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

        // ===== بلوک‌های حرکتی =====
        if (b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT ||
            b.type == TURN_RIGHT || b.type == TURN_LEFT || b.type == GOTO_XY || b.type == CHANGE_X ||
            b.type == CHANGE_Y || b.type == SET_X || b.type == SET_Y || b.type == GOTO_RANDOM)
        {
            if (b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT)
            {
                if (!game.isExecutingBlock)
                {
                    if (!b.parameters.empty())
                        game.remainingMove = b.parameters[0].asNumber();
                    else
                        game.remainingMove = 5;
                    game.isExecutingBlock = true;
                }

                double moveAmount = min(speed, game.remainingMove);

                switch (b.type)
                {
                    case MOVE_UP:    game.player.y -= moveAmount; break;
                    case MOVE_DOWN:  game.player.y += moveAmount; break;
                    case MOVE_LEFT:  game.player.x -= moveAmount; break;
                    case MOVE_RIGHT: game.player.x += moveAmount; break;
                    default: break;
                }

                game.remainingMove -= moveAmount;

                if (game.remainingMove <= 0)
                {
                    game.isExecutingBlock = false;
                    scriptPC++;  // <--- این خط رو اضافه کن
                }
                else
                {
                    // اگه هنوز حرکت تموم نشده، همینجا بمون و بعداً ادامه بده
                    break;
                }
            }
            else
            {
                // بلوک‌های حرکتی غیر از move (مثل turn, goto, ...)
                if (b.type == TURN_RIGHT)
                {
                    if (!b.parameters.empty())
                        game.player.direction += b.parameters[0].asNumber();
                }
                else if (b.type == TURN_LEFT)
                {
                    if (!b.parameters.empty())
                        game.player.direction -= b.parameters[0].asNumber();
                }
                else if (b.type == GOTO_XY)
                {
                    if (b.parameters.size() >= 2)
                    {
                        game.player.x = b.parameters[0].asNumber();
                        game.player.y = b.parameters[1].asNumber();
                    }
                }
                else if (b.type == CHANGE_X)
                {
                    if (!b.parameters.empty())
                        game.player.x += b.parameters[0].asNumber();
                }
                else if (b.type == CHANGE_Y)
                {
                    if (!b.parameters.empty())
                        game.player.y += b.parameters[0].asNumber();
                }
                else if (b.type == SET_X)
                {
                    if (!b.parameters.empty())
                        game.player.x = b.parameters[0].asNumber();
                }
                else if (b.type == SET_Y)
                {
                    if (!b.parameters.empty())
                        game.player.y = b.parameters[0].asNumber();
                }
                else if (b.type == GOTO_RANDOM)
                {
                    game.player.x = rand() % (game.screenWidth - game.player.w);
                    game.player.y = rand() % (game.screenHeight - game.player.h);
                }

                // محدودیت مرزها
                if (game.player.x < 0) game.player.x = 0;
                if (game.player.y < 0) game.player.y = 0;
                if (game.player.x + game.player.w > game.screenWidth)
                    game.player.x = game.screenWidth - game.player.w;
                if (game.player.y + game.player.h > game.screenHeight)
                    game.player.y = game.screenHeight - game.player.h;

                scriptPC++;  // <--- این خط رو اضافه کن
            }

            safetyCounter++;
            continue;
        }

        scriptPC++;
        safetyCounter++;
    }

    // رسم خط اگر قلم پایین است
    if (game.player.penDown && game.player.penMoved)
    {
        double centerX = game.player.x + game.player.w/2;
        double centerY = game.player.y + game.player.h/2;

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