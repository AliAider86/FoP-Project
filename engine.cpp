#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include "engine.h"
#include "operators.h"
#include "logger.h"

using namespace std;

// --- توابع کمکی برای مدیریت اسپرایت‌ها ---
Sprite* getActiveSprite(GameState& game)
{
    if (game.activeSpriteIndex >= 0 && game.activeSpriteIndex < game.sprites.size())
        return &game.sprites[game.activeSpriteIndex];
    return nullptr;
}

void setActiveSprite(GameState& game, int index)
{
    if (index < 0 || index >= game.sprites.size())
        return;

    if (game.activeSpriteIndex >= 0 && game.activeSpriteIndex < game.sprites.size())
        game.sprites[game.activeSpriteIndex].isActive = false;

    game.activeSpriteIndex = index;
    game.sprites[index].isActive = true;
    log_info(("Active sprite changed to: " + game.sprites[index].name).c_str());
}

void addSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath)
{
    Sprite newSprite;
    newSprite.x = game.screenWidth / 2 - 25;
    newSprite.y = game.screenHeight / 2 - 25;
    newSprite.w = 50;
    newSprite.h = 50;
    newSprite.visible = true;
    newSprite.direction = 0;
    newSprite.name = name;
    newSprite.message = "";
    newSprite.isThinking = false;
    newSprite.penDown = false;
    newSprite.penSize = 1;
    newSprite.penR = 0;
    newSprite.penG = 0;
    newSprite.penB = 0;
    newSprite.lastPenX = newSprite.x + newSprite.w/2;
    newSprite.lastPenY = newSprite.y + newSprite.h/2;
    newSprite.penMoved = false;
    newSprite.texture = nullptr;
    newSprite.imagePath = imagePath ? imagePath : "";
    newSprite.index = game.sprites.size();
    newSprite.isActive = false;

    // این قسمت مهمه - لود کردن texture
    if (imagePath && renderer && !newSprite.imagePath.empty())
    {
        extern bool loadSpriteTexture(Sprite* sprite, SDL_Renderer* renderer, const char* path);
        loadSpriteTexture(&newSprite, renderer, newSprite.imagePath.c_str());
    }

    game.sprites.push_back(newSprite);
    log_info(("Sprite added: " + string(name)).c_str());
}

void removeSprite(GameState& game, int index)
{
    if (index < 0 || index >= game.sprites.size())
        return;

    log_info(("Removing sprite: " + game.sprites[index].name).c_str());

    if (game.sprites[index].texture)
        SDL_DestroyTexture(game.sprites[index].texture);

    game.sprites.erase(game.sprites.begin() + index);

    if (game.activeSpriteIndex == index)
    {
        if (game.sprites.size() > 0)
            setActiveSprite(game, 0);
        else
            game.activeSpriteIndex = -1;
    }
    else if (game.activeSpriteIndex > index)
    {
        game.activeSpriteIndex--;
    }
}

// --- توابع ذخیره و بارگذاری ---
void saveProject(const GameState& game, const string& filename)
{
    log_info(("Saving project to: " + filename).c_str());

    ofstream file(filename);
    if (!file.is_open())
    {
        log_error(("Could not save file: " + filename).c_str());
        return;
    }

    // ذخیره تعداد اسپرایت‌ها
    file << "Sprites " << game.sprites.size() << endl;

    // ذخیره اطلاعات هر اسپرایت
    for (const Sprite& s : game.sprites)
    {
        file << "Sprite " << s.x << " " << s.y << " "
             << s.w << " " << s.h << " "
             << s.visible << " " << s.direction << " "
             << s.name << " " << s.imagePath << " "
             << s.penDown << " " << s.penSize << " "
             << (int)s.penR << " " << (int)s.penG << " " << (int)s.penB << endl;
    }

    // ذخیره ایندکس اسپرایت فعال
    file << "ActiveSprite " << game.activeSpriteIndex << endl;

    // ذخیره بلوک‌ها
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

    // ذخیره متغیرها
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
    log_info(("Project saved to " + filename).c_str());
}

void loadProject(GameState& game, const string& filename)
{
    log_info(("Loading project from: " + filename).c_str());

    ifstream file(filename);
    if (!file.is_open())
    {
        log_error(("Could not load file: " + filename).c_str());
        return;
    }

    // پاک کردن وضعیت قبلی
    for (auto& s : game.sprites)
    {
        if (s.texture)
            SDL_DestroyTexture(s.texture);
    }

    game.sprites.clear();
    game.program.clear();
    game.variables.clear();
    game.scriptStartIndices.clear();
    game.scriptActive.clear();
    game.scriptCurrentBlock.clear();
    game.penX1.clear();
    game.penY1.clear();
    game.penX2.clear();
    game.penY2.clear();
    game.penR_.clear();
    game.penG_.clear();
    game.penB_.clear();
    game.penSize_.clear();

    string token;
    while (file >> token)
    {
        if (token == "Sprites")
        {
            int count;
            file >> count;
        }
        else if (token == "Sprite")
        {
            Sprite s;
            string name, imagePath;
            int r, g, b;

            file >> s.x >> s.y >> s.w >> s.h
                 >> s.visible >> s.direction
                 >> name >> imagePath
                 >> s.penDown >> s.penSize
                 >> r >> g >> b;

            s.name = name;
            s.imagePath = imagePath;
            s.penR = (Uint8)r;
            s.penG = (Uint8)g;
            s.penB = (Uint8)b;
            s.message = "";
            s.isThinking = false;
            s.lastPenX = s.x + s.w/2;
            s.lastPenY = s.y + s.h/2;
            s.penMoved = false;
            s.texture = nullptr;
            s.index = game.sprites.size();
            s.isActive = false;

            game.sprites.push_back(s);
        }
        else if (token == "ActiveSprite")
        {
            file >> game.activeSpriteIndex;
            if (game.activeSpriteIndex >= 0 && game.activeSpriteIndex < game.sprites.size())
                game.sprites[game.activeSpriteIndex].isActive = true;
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
    log_info(("Project loaded from " + filename).c_str());
}

void update(GameState& game)
{
    if (!game.isRunningCode)
        return;

    Sprite* activeSprite = getActiveSprite(game);
    if (!activeSprite && game.sprites.size() > 0)
    {
        setActiveSprite(game, 0);
        activeSprite = getActiveSprite(game);
    }

    const double speed = 2.0;
    int safetyCounter = 0;
    const int MAX_OPERATIONS_PER_FRAME = 1000;

    // رویداد Green Flag
    if (game.greenFlagPressed)
    {
        log_info("Green flag pressed - starting program");
        game.greenFlagPressed = false;

        game.scriptStartIndices.clear();
        game.scriptActive.clear();
        game.scriptCurrentBlock.clear();

        game.scriptStartIndices.push_back(0);
        game.scriptActive.push_back(true);
        game.scriptCurrentBlock.push_back(0);
    }

    // رویداد کلیک روی اسپرایت
    if (game.spriteClicked && game.clickedSpriteIndex >= 0)
    {
        log_info(("Sprite clicked: " + game.sprites[game.clickedSpriteIndex].name).c_str());
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
        if (!activeSprite) continue;

        if (b.type == SAY || b.type == SAY_FOR || b.type == THINK || b.type == THINK_FOR)
        {
            if (b.type == SAY_FOR || b.type == THINK_FOR)
            {
                if (!game.isShowingMessage)
                {
                    if (!b.parameters.empty())
                    {
                        activeSprite->message = b.parameters[0].asString();
                        activeSprite->isThinking = (b.type == THINK_FOR);

                        if (b.parameters.size() >= 2)
                            game.messageDuration = b.parameters[1].asNumber() * 1000;
                        else
                            game.messageDuration = 2000;

                        game.messageStartTime = SDL_GetTicks();
                        game.isShowingMessage = true;

                        string msgType = activeSprite->isThinking ? "THINK" : "SAY";
                        log_info((msgType + ": " + activeSprite->message).c_str());
                    }
                    return;
                }

                if (SDL_GetTicks() - game.messageStartTime >= game.messageDuration)
                {
                    activeSprite->message = "";
                    game.isShowingMessage = false;
                    scriptPC++;
                }
                return;
            }
            else
            {
                if (!b.parameters.empty())
                {
                    activeSprite->message = b.parameters[0].asString();
                    activeSprite->isThinking = (b.type == THINK);

                    string msgType = activeSprite->isThinking ? "THINK" : "SAY";
                    log_info((msgType + ": " + activeSprite->message).c_str());
                }
                scriptPC++;
                safetyCounter++;
                continue;
            }
        }

        if (b.type == SHOW)
        {
            if (!activeSprite->visible)
            {
                activeSprite->visible = true;
                log_info("Sprite shown");
            }
            scriptPC++;
            continue;
        }

        if (b.type == HIDE)
        {
            if (activeSprite->visible)
            {
                activeSprite->visible = false;
                log_info("Sprite hidden");
            }
            scriptPC++;
            continue;
        }

        if (b.type == CHANGE_SIZE)
        {
            if (!b.parameters.empty())
            {
                double percent = b.parameters[0].asNumber();
                activeSprite->w = (int)(activeSprite->w * (1 + percent/100));
                activeSprite->h = (int)(activeSprite->h * (1 + percent/100));

                if (activeSprite->w < 5) activeSprite->w = 5;
                if (activeSprite->h < 5) activeSprite->h = 5;
            }
            scriptPC++;
            continue;
        }

        if (b.type == SET_SIZE)
        {
            if (!b.parameters.empty())
            {
                double percent = b.parameters[0].asNumber();
                int newSize = (int)(percent);
                if (newSize < 5) newSize = 5;
                activeSprite->w = newSize;
                activeSprite->h = newSize;
            }
            scriptPC++;
            continue;
        }

        // ===== بلوک‌های صدا =====
        if (b.type == PLAY_SOUND || b.type == PLAY_SOUND_UNTIL_DONE)
        {
            if (b.type == PLAY_SOUND)
            {
                if (game.soundEffect)
                {
                    int volumeLevel = (game.volume * MIX_MAX_VOLUME) / 100;
                    Mix_VolumeChunk(game.soundEffect, volumeLevel);
                    Mix_PlayChannel(-1, game.soundEffect, 0);
                    log_info("Playing sound");
                }
                else
                {
                    log_error("No sound loaded to play");
                }
                scriptPC++;
            }
            else
            {
                if (!game.isWaiting)
                {
                    if (game.soundEffect)
                    {
                        int volumeLevel = (game.volume * MIX_MAX_VOLUME) / 100;
                        Mix_VolumeChunk(game.soundEffect, volumeLevel);
                        game.soundChannel = Mix_PlayChannel(-1, game.soundEffect, 0);
                        log_info("Playing sound until done");

                        if (game.soundChannel == -1)
                        {
                            log_error(("Error playing sound: " + string(Mix_GetError())).c_str());
                        }
                    }

                    game.waitStartTime = SDL_GetTicks();
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
            continue;
        }

        if (b.type == STOP_ALL_SOUNDS)
        {
            Mix_HaltChannel(-1);
            game.isPlayingSound = false;
            log_info("Stopping all sounds");
            scriptPC++;
            continue;
        }

        if (b.type == CHANGE_VOLUME)
        {
            if (!b.parameters.empty())
            {
                int change = (int)b.parameters[0].asNumber();
                game.volume += change;
                if (game.volume < 0) game.volume = 0;
                if (game.volume > 100) game.volume = 100;
                log_info(("Volume changed to: " + to_string(game.volume) + "%").c_str());
            }
            scriptPC++;
            continue;
        }

        if (b.type == SET_VOLUME)
        {
            if (!b.parameters.empty())
            {
                int newVolume = (int)b.parameters[0].asNumber();
                if (newVolume < 0) newVolume = 0;
                if (newVolume > 100) newVolume = 100;
                game.volume = newVolume;
                log_info(("Volume set to: " + to_string(game.volume) + "%").c_str());
            }
            scriptPC++;
            continue;
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
                log_info(("Variable set: " + b.variableName).c_str());
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
                log_info(("Variable changed: " + b.variableName).c_str());
            }
            scriptPC++;
            safetyCounter++;
            continue;
        }

        // ===== بلوک‌های قلم =====
        if (b.type == PEN_DOWN)
        {
            activeSprite->penDown = true;
            activeSprite->lastPenX = activeSprite->x + activeSprite->w/2;
            activeSprite->lastPenY = activeSprite->y + activeSprite->h/2;
            activeSprite->penMoved = true;
            log_info("Pen down");
            scriptPC++;
            continue;
        }

        if (b.type == PEN_UP)
        {
            activeSprite->penDown = false;
            log_info("Pen up");
            scriptPC++;
            continue;
        }

        if (b.type == SET_PEN_COLOR)
        {
            if (b.parameters.size() >= 3)
            {
                activeSprite->penR = (Uint8)b.parameters[0].asNumber();
                activeSprite->penG = (Uint8)b.parameters[1].asNumber();
                activeSprite->penB = (Uint8)b.parameters[2].asNumber();
                log_info("Pen color changed");
            }
            scriptPC++;
            continue;
        }

        if (b.type == SET_PEN_SIZE)
        {
            if (!b.parameters.empty())
            {
                activeSprite->penSize = (int)b.parameters[0].asNumber();
                if (activeSprite->penSize < 1) activeSprite->penSize = 1;
                log_info(("Pen size set to: " + to_string(activeSprite->penSize)).c_str());
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
            log_info("Pen cleared");
            scriptPC++;
            continue;
        }

        // ===== بلوک‌های حرکتی =====
        if (b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT ||
            b.type == TURN_RIGHT || b.type == TURN_LEFT || b.type == GOTO_XY || b.type == CHANGE_X ||
            b.type == CHANGE_Y || b.type == SET_X || b.type == SET_Y || b.type == GOTO_RANDOM || b.type == GOTO_MOUSE)
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
                    case MOVE_UP:    activeSprite->y -= moveAmount; break;
                    case MOVE_DOWN:  activeSprite->y += moveAmount; break;
                    case MOVE_LEFT:  activeSprite->x -= moveAmount; break;
                    case MOVE_RIGHT: activeSprite->x += moveAmount; break;
                    default: break;
                }

                game.remainingMove -= moveAmount;

                if (game.remainingMove <= 0)
                {
                    game.isExecutingBlock = false;
                    scriptPC++;
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (b.type == TURN_RIGHT)
                {
                    if (!b.parameters.empty())
                        activeSprite->direction += b.parameters[0].asNumber();
                }
                else if (b.type == TURN_LEFT)
                {
                    if (!b.parameters.empty())
                        activeSprite->direction -= b.parameters[0].asNumber();
                }
                else if (b.type == GOTO_XY)
                {
                    if (b.parameters.size() >= 2)
                    {
                        activeSprite->x = b.parameters[0].asNumber();
                        activeSprite->y = b.parameters[1].asNumber();
                        log_info(("Goto: x=" + to_string(activeSprite->x) + " y=" + to_string(activeSprite->y)).c_str());
                    }
                }
                else if (b.type == CHANGE_X)
                {
                    if (!b.parameters.empty())
                        activeSprite->x += b.parameters[0].asNumber();
                }
                else if (b.type == CHANGE_Y)
                {
                    if (!b.parameters.empty())
                        activeSprite->y += b.parameters[0].asNumber();
                }
                else if (b.type == SET_X)
                {
                    if (!b.parameters.empty())
                        activeSprite->x = b.parameters[0].asNumber();
                }
                else if (b.type == SET_Y)
                {
                    if (!b.parameters.empty())
                        activeSprite->y = b.parameters[0].asNumber();
                }
                else if (b.type == GOTO_RANDOM)
                {
                    activeSprite->x = rand() % (game.screenWidth - activeSprite->w);
                    activeSprite->y = rand() % (game.screenHeight - activeSprite->h);
                    log_info("Goto random position");
                }
                else if (b.type == GOTO_MOUSE)
                {
                    int categoriesPanelWidth = 180;
                    int examplesPanelX = categoriesPanelWidth + 30;
                    int examplesPanelWidth = 250;
                    int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
                    int codeAreaWidth = 600;
                    int stagePanelX = codeAreaX + codeAreaWidth + 10;
                    int stagePanelWidth = game.screenWidth - stagePanelX - 5;
                    if (stagePanelWidth > 500) stagePanelWidth = 500;

                    int stageX = stagePanelX + 5;
                    int stageY = 70;
                    int stageWidth = stagePanelWidth - 20;
                    int stageHeight = 300;

                    if (game.mouseX >= stageX && game.mouseX <= stageX + stageWidth &&
                        game.mouseY >= stageY && game.mouseY <= stageY + stageHeight)
                    {
                        activeSprite->x = (double)(game.mouseX - stageX) * game.screenWidth / stageWidth;
                        activeSprite->y = (double)(game.mouseY - stageY) * game.screenHeight / stageHeight;
                    }
                    else
                    {
                        activeSprite->x = game.screenWidth / 2 - activeSprite->w / 2;
                        activeSprite->y = game.screenHeight / 2 - activeSprite->h / 2;
                    }

                    log_info("Goto mouse position");
                }

                // محدودیت مرزها
                if (activeSprite->x < 0) activeSprite->x = 0;
                if (activeSprite->y < 0) activeSprite->y = 0;
                if (activeSprite->x + activeSprite->w > game.screenWidth)
                    activeSprite->x = game.screenWidth - activeSprite->w;
                if (activeSprite->y + activeSprite->h > game.screenHeight)
                    activeSprite->y = game.screenHeight - activeSprite->h;

                scriptPC++;
            }

            safetyCounter++;
            continue;
        }

        scriptPC++;
        safetyCounter++;
    }

    // رسم خط اگر قلم پایین است (برای همه اسپرایت‌ها)
    for (auto& sprite : game.sprites)
    {
        if (sprite.penDown && sprite.penMoved)
        {
            double centerX = sprite.x + sprite.w/2;
            double centerY = sprite.y + sprite.h/2;

            if (abs(centerX - sprite.lastPenX) > 1 || abs(centerY - sprite.lastPenY) > 1)
            {
                game.penX1.push_back((int)sprite.lastPenX);
                game.penY1.push_back((int)sprite.lastPenY);
                game.penX2.push_back((int)centerX);
                game.penY2.push_back((int)centerY);
                game.penR_.push_back(sprite.penR);
                game.penG_.push_back(sprite.penG);
                game.penB_.push_back(sprite.penB);
                game.penSize_.push_back(sprite.penSize);

                sprite.lastPenX = centerX;
                sprite.lastPenY = centerY;
            }
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