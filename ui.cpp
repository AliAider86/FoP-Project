#include "ui.h"
#include "engine.h"
#include "logger.h"
#include <string>
#include "filedialog.h"
#include <iostream>
#include <cmath>
#include <SDL2/SDL2_gfx.h>
#include <algorithm>
#include <cctype>

using namespace std;

// اعلان توابع خارجی
extern bool loadSpriteTexture(Sprite* sprite, SDL_Renderer* renderer, const char* path);
extern void renderText(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color);
extern void addDefaultSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath);

void resetToNewProject(GameState& game, SDL_Renderer* renderer)
{
    // حذف همه اسپرایت‌ها
    for (auto& s : game.sprites)
        if (s.texture) SDL_DestroyTexture(s.texture);
    game.sprites.clear();

    // پاک کردن بلوک‌ها و متغیرها
    game.program.clear();
    game.variables.clear();
    game.scriptStartIndices.clear();
    game.scriptActive.clear();
    game.scriptCurrentBlock.clear();
    game.repeatCountStack.clear();
    game.repeatStartStack.clear();
    while (!game.ifStack.empty()) game.ifStack.pop();
    game.messageHandlers.clear();

    // ریست وضعیت‌ها
    game.waitingForBroadcast = false;
    game.waitingScriptIndex = -1;
    game.waitingForAnswer = false;
    game.answer = "";
    game.currentQuestion = "";
    game.isRunningCode = false;
    game.isPaused = false;
    game.stepMode = false;
    game.currentBlockIndex = 0;
    game.isExecutingBlock = false;
    game.remainingMove = 0;
    game.isWaiting = false;
    game.isShowingMessage = false;

    // افزودن اسپرایت پیش‌فرض
    addDefaultSprite(game, renderer, "Sprite1", "cat.png");
    if (game.sprites.size() > 0)
    {
        game.activeSpriteIndex = 0;
        game.sprites[0].isActive = true;
    }

    // ریست پس‌زمینه به اولین
    game.currentBackdrop = 0;

    log_info("New project created");
}

// ==================== مدیریت رویدادها ====================
void handleEvents(bool &running, GameState& game, SDL_Renderer* renderer)
{
    // reset pressedThisFrame
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        game.pressedThisFrame[i] = 0;

    // محاسبه مختصات Code Area
    int categoriesPanelWidth = 180;
    int examplesPanelX = categoriesPanelWidth + 30;
    int examplesPanelWidth = 250;
    int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
    int codeAreaWidth = 600;
    int codeAreaY = 70;
    int codeAreaHeight = game.screenHeight - 150;

    game.codeAreaX = codeAreaX;
    game.codeAreaY = codeAreaY;
    game.codeAreaWidth = codeAreaWidth;
    game.codeAreaHeight = codeAreaHeight;

    // تعیین موقعیت منطقه حذف (گوشه پایین راست Code Area)
    game.trashX = codeAreaX + codeAreaWidth - game.trashW - 10;
    game.trashY = codeAreaY + codeAreaHeight - game.trashH - 10;

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            running = false;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            running = false;

        // تشخیص کلیدهای فشرده
        if (e.type == SDL_KEYDOWN && !e.key.repeat)
        {
            game.pressedKeys[e.key.keysym.scancode] = 1;
            game.pressedThisFrame[e.key.keysym.scancode] = 1;

            // ===== ویرایش با صفحه‌کلید =====
            bool keepEditing = false;
            bool blockEditing = false;

            for (int i = 0; i < game.program.size(); i++)
            {
                if (game.program[i].editingMode)
                {
                    blockEditing = true;
                    Block& b = game.program[i];

                    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                    {
                        if (!b.editingBuffer.empty())
                        {
                            if (b.type == SET_COMPARISON)
                            {
                                switch (b.editingField) {
                                    case 0:
                                    {
                                        string op = b.editingBuffer;
                                        if (op == "=") b.compareOp = CMP_EQUAL;
                                        else if (op == "≠" || op == "!=") b.compareOp = CMP_NOT_EQUAL;
                                        else if (op == "<") b.compareOp = CMP_LESS;
                                        else if (op == "<=") b.compareOp = CMP_LESS_OR_EQUAL;
                                        else if (op == ">") b.compareOp = CMP_GREATER;
                                        else if (op == ">=") b.compareOp = CMP_GREATER_OR_EQUAL;
                                        break;
                                    }
                                    case 1:
                                        if (!b.editingBuffer.empty()) b.variableName = b.editingBuffer;
                                        break;
                                    case 2:
                                        if (isalpha(b.editingBuffer[0])) {
                                            b.leftVar = b.editingBuffer;
                                        } else {
                                            b.leftVar = "";
                                            double val = stod(b.editingBuffer);
                                            if (b.parameters.empty()) b.parameters.push_back(Value(val));
                                            else b.parameters[0] = Value(val);
                                        }
                                        break;
                                    case 3:
                                        if (isalpha(b.editingBuffer[0])) {
                                            b.rightVar = b.editingBuffer;
                                        } else {
                                            b.rightVar = "";
                                            double val = stod(b.editingBuffer);
                                            if (b.parameters.size() < 2) b.parameters.resize(2);
                                            b.parameters[1] = Value(val);
                                        }
                                        break;
                                }

                                const char* opStr = "";
                                switch (b.compareOp) {
                                    case CMP_EQUAL: opStr = "="; break;
                                    case CMP_NOT_EQUAL: opStr = "≠"; break;
                                    case CMP_LESS: opStr = "<"; break;
                                    case CMP_LESS_OR_EQUAL: opStr = "≤"; break;
                                    case CMP_GREATER: opStr = ">"; break;
                                    case CMP_GREATER_OR_EQUAL: opStr = "≥"; break;
                                }
                                string left = b.leftVar.empty() ? (b.parameters.empty() ? "0" : to_string((int)b.parameters[0].asNumber())) : b.leftVar;
                                string right = b.rightVar.empty() ? (b.parameters.size() > 1 ? to_string((int)b.parameters[1].asNumber()) : "0") : b.rightVar;
                                b.eventName = "set " + b.variableName + " to compare (" + left + " " + opStr + " " + right + ")";

                                int nextField = b.editingField + 1;
                                if (nextField <= 3) {
                                    b.editingField = nextField;
                                    switch (nextField) {
                                        case 1: b.editingBuffer = b.variableName.empty() ? "result" : b.variableName; break;
                                        case 2: b.editingBuffer = !b.leftVar.empty() ? b.leftVar : (b.parameters.empty() ? "0" : to_string((int)b.parameters[0].asNumber())); break;
                                        case 3: b.editingBuffer = !b.rightVar.empty() ? b.rightVar : (b.parameters.size() > 1 ? to_string((int)b.parameters[1].asNumber()) : "0"); break;
                                    }
                                    keepEditing = true;
                                }
                            }

                            // ===== بلوک‌های Events (ویرایش messageName) =====
                            if (b.type == WHEN_I_RECEIVE || b.type == BROADCAST || b.type == BROADCAST_AND_WAIT)
                            {
                                string oldName = b.messageName;
                                b.messageName = b.editingBuffer;

                                // اگر بلوک از نوع WHEN_I_RECEIVE است، هندلرها را به‌روز کن
                                if (b.type == WHEN_I_RECEIVE)
                                {
                                    // حذف از هندلر قدیم
                                    if (game.messageHandlers.find(oldName) != game.messageHandlers.end())
                                    {
                                        auto& vec = game.messageHandlers[oldName];
                                        vec.erase(remove(vec.begin(), vec.end(), i), vec.end());
                                        if (vec.empty()) game.messageHandlers.erase(oldName);
                                    }
                                    // اضافه به هندلر جدید
                                    game.messageHandlers[b.messageName].push_back(i);
                                }

                                // بازسازی eventName
                                if (b.type == WHEN_I_RECEIVE)
                                    b.eventName = "when I receive " + b.messageName;
                                else if (b.type == BROADCAST)
                                    b.eventName = "broadcast " + b.messageName;
                                else if (b.type == BROADCAST_AND_WAIT)
                                    b.eventName = "broadcast " + b.messageName + " and wait";

                                log_info(("Message name changed to: " + b.messageName).c_str());
                            }
                                // ===== بلوک‌های SAY و THINK (دو پارامتری) =====
                            else if (b.type == SAY_FOR || b.type == THINK_FOR)
                            {
                                if (b.editingField == 0)
                                {
                                    b.parameters[0] = Value(b.editingBuffer);
                                }
                                else if (b.editingField == 1)
                                {
                                    double newVal = stod(b.editingBuffer);
                                    b.parameters[1] = Value(newVal);
                                }
                                // بازسازی eventName
                                string msg = b.parameters[0].asString();
                                string sec = (b.parameters.size() > 1) ? to_string((int)b.parameters[1].asNumber()) : "2";
                                b.eventName = (b.type == SAY_FOR ? "say " : "think ") + msg + " for " + sec + " secs";
                            }
                                // ===== بلوک‌های SAY و THINK ساده =====
                            else if (b.type == SAY || b.type == THINK)
                            {
                                b.parameters[0] = Value(b.editingBuffer);
                                b.eventName = (b.type == SAY ? "say " : "think ") + b.parameters[0].asString();
                            }
                            else if ((b.type == GO_FORWARD_LAYERS || b.type == GO_BACKWARD_LAYERS) && !b.parameters.empty())
                            {
                                int newVal = stoi(b.editingBuffer);
                                if (newVal < 0) newVal = 0;
                                b.parameters[0] = Value((double)newVal);
                                if (b.type == GO_FORWARD_LAYERS)
                                    b.eventName = "go forward " + to_string(newVal) + " layers";
                                else
                                    b.eventName = "go backward " + to_string(newVal) + " layers";
                            }
                                // ===== بلوک GOTO_XY =====
                            else if (b.type == GOTO_XY && b.parameters.size() >= 2)
                            {
                                double newVal = stod(b.editingBuffer);
                                if (b.editingField < b.parameters.size())
                                {
                                    b.parameters[b.editingField] = Value(newVal);
                                }
                                // بازسازی
                                string x = to_string((int)b.parameters[0].asNumber());
                                string y = to_string((int)b.parameters[1].asNumber());
                                b.eventName = "go to x: " + x + " y: " + y;
                            }

                            else if (b.type == COSTUME_NUMBER || b.type == SPRITE_SIZE || b.type == BACKDROP_NUMBER)
                            {
                                if (!b.editingBuffer.empty())
                                    b.variableName = b.editingBuffer;
                                // بازسازی eventName
                                if (b.type == COSTUME_NUMBER)
                                    b.eventName = "set " + b.variableName + " to costume number";
                                else if (b.type == SPRITE_SIZE)
                                    b.eventName = "set " + b.variableName + " to size";
                                else if (b.type == BACKDROP_NUMBER)
                                    b.eventName = "set " + b.variableName + " to backdrop number";
                            }
                                // ===== بلوک‌های حرکتی و کنترلی ساده =====
                            else if ((b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT ||
                                      b.type == TURN_RIGHT || b.type == TURN_LEFT ||
                                      b.type == CHANGE_X || b.type == CHANGE_Y ||
                                      b.type == SET_X || b.type == SET_Y ||
                                      b.type == CHANGE_SIZE || b.type == WAIT) &&
                                     !b.parameters.empty())
                            {
                                double newVal = stod(b.editingBuffer);
                                b.parameters[0] = Value(newVal);
                                // بازسازی بر اساس نوع
                                switch (b.type) {
                                    case MOVE_UP:    b.eventName = "move up " + to_string((int)newVal) + " steps"; break;
                                    case MOVE_DOWN:  b.eventName = "move down " + to_string((int)newVal) + " steps"; break;
                                    case MOVE_LEFT:  b.eventName = "move left " + to_string((int)newVal) + " steps"; break;
                                    case MOVE_RIGHT: b.eventName = "move right " + to_string((int)newVal) + " steps"; break;
                                    case TURN_RIGHT: b.eventName = "turn right " + to_string((int)newVal) + " degrees"; break;
                                    case TURN_LEFT:  b.eventName = "turn left " + to_string((int)newVal) + " degrees"; break;
                                    case CHANGE_X:   b.eventName = "change x by " + to_string((int)newVal); break;
                                    case CHANGE_Y:   b.eventName = "change y by " + to_string((int)newVal); break;
                                    case SET_X:      b.eventName = "set x to " + to_string((int)newVal); break;
                                    case SET_Y:      b.eventName = "set y to " + to_string((int)newVal); break;
                                    case CHANGE_SIZE: b.eventName = "change size by " + to_string((int)newVal); break;
                                    case WAIT:        b.eventName = "wait " + to_string((int)newVal) + " seconds"; break;
                                    default: break;
                                }
                            }
                                // ===== بلوک SET_SIZE =====
                            else if (b.type == SET_SIZE && !b.parameters.empty())
                            {
                                int newVal = stoi(b.editingBuffer);
                                if (newVal < 1) newVal = 1;
                                if (newVal > 500) {
                                    newVal = 500; // محدودیت بالا
                                }
                                b.parameters[0] = Value((double)newVal);
                                b.eventName = "set size to " + to_string(newVal) + " %";
                            }
                                // ===== بلوک‌های صدا =====
                            else if (b.type == CHANGE_VOLUME || b.type == SET_VOLUME)
                            {
                                int newVal = stoi(b.editingBuffer);
                                if (newVal < 0) newVal = 0;
                                if (newVal > 100) newVal = 100;
                                b.parameters[0] = Value((double)newVal);
                                if (b.type == CHANGE_VOLUME)
                                    b.eventName = "change volume by " + to_string(newVal);
                                else
                                    b.eventName = "set volume to " + to_string(newVal) + " %";
                            }
                                // ===== بلوک‌های Pen =====
                            else if (b.type == PEN_SET_COLOR_PARAM || b.type == PEN_CHANGE_COLOR_PARAM ||
                                     b.type == PEN_SET_SIZE || b.type == PEN_CHANGE_SIZE)
                            {
                                double newVal = stod(b.editingBuffer);
                                b.parameters[0] = Value(newVal);

                                // بازسازی eventName بر اساس نوع بلوک Pen
                                switch (b.type) {
                                    case PEN_SET_COLOR_PARAM:
                                        if (b.penParam == PEN_PARAM_COLOR)
                                            b.eventName = "set pen color to " + to_string((int)newVal);
                                        else if (b.penParam == PEN_PARAM_SATURATION)
                                            b.eventName = "set pen saturation to " + to_string((int)newVal);
                                        else if (b.penParam == PEN_PARAM_BRIGHTNESS)
                                            b.eventName = "set pen brightness to " + to_string((int)newVal);
                                        break;
                                    case PEN_CHANGE_COLOR_PARAM:
                                        if (b.penParam == PEN_PARAM_COLOR)
                                            b.eventName = "change pen color by " + to_string((int)newVal);
                                        else if (b.penParam == PEN_PARAM_SATURATION)
                                            b.eventName = "change pen saturation by " + to_string((int)newVal);
                                        else if (b.penParam == PEN_PARAM_BRIGHTNESS)
                                            b.eventName = "change pen brightness by " + to_string((int)newVal);
                                        break;
                                    case PEN_SET_SIZE:
                                        b.eventName = "set pen size to " + to_string((int)newVal);
                                        break;
                                    case PEN_CHANGE_SIZE:
                                        b.eventName = "change pen size by " + to_string((int)newVal);
                                        break;
                                    default:
                                        break;
                                }
                            }
                                // ===== بلوک‌های کنترلی جدید =====
                            else if (b.type == REPEAT)
                            {
                                int newVal = stoi(b.editingBuffer);
                                if (newVal < 0) newVal = 0;
                                b.repeatCount = newVal;
                                b.eventName = "repeat " + to_string(newVal);
                            }
                            else if (b.type == IF_THEN || b.type == IF_THEN_ELSE || b.type == WAIT_UNTIL || b.type == REPEAT_UNTIL)
                            {
                                // ذخیره شرط (به صورت بولی)
                                bool cond = (b.editingBuffer == "true" || b.editingBuffer == "1");
                                if (b.parameters.empty())
                                    b.parameters.push_back(Value(cond));
                                else
                                    b.parameters[0] = Value(cond);

                                // بازسازی eventName
                                if (b.type == IF_THEN)
                                    b.eventName = "if " + b.editingBuffer + " then";
                                else if (b.type == IF_THEN_ELSE)
                                    b.eventName = "if " + b.editingBuffer + " then else";
                                else if (b.type == WAIT_UNTIL)
                                    b.eventName = "wait until " + b.editingBuffer;
                                else if (b.type == REPEAT_UNTIL)
                                    b.eventName = "repeat until " + b.editingBuffer;
                            }
                            else if (b.type == STOP_ALL)
                            {
                                b.eventName = "stop all";
                            }
                            else if (b.type == STOP_THIS_SCRIPT)
                            {
                                b.eventName = "stop this script";
                            }
                        }
                        b.editingMode = false;
                        b.editingField = -1;
                        b.editingBuffer = "";

                        if (!keepEditing) {
                            b.editingMode = false;
                            b.editingField = -1;
                            b.editingBuffer = "";
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        b.editingMode = false;
                        b.editingField = -1;
                        b.editingBuffer = "";
                    }
                    else if (e.key.keysym.sym == SDLK_BACKSPACE && !b.editingBuffer.empty())
                    {
                        b.editingBuffer.pop_back();
                    }
                    break;
                }
            }

            if (!blockEditing && game.editingMode)
            {
                Sprite* active = getActiveSprite(game);
                if (!active) continue;

                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                {
                    if (game.editingField == 0)
                    {
                        if (!game.editingBuffer.empty())
                            active->name = game.editingBuffer;
                    }
                    else if (game.editingField == 1)
                    {
                        active->x = stod(game.editingBuffer);
                    }
                    else if (game.editingField == 2)
                    {
                        active->y = stod(game.editingBuffer);
                    }
                    else if (game.editingField == 3)
                    {
                        int newSize = stoi(game.editingBuffer);
                        if (newSize < 5) newSize = 5;
                        active->w = newSize;
                        active->h = newSize;
                    }
                    else if (game.editingField == 4)
                    {
                        active->direction = stod(game.editingBuffer);
                    }
                    else if (game.editingField == 5)
                    {
                        int newVolume = stoi(game.editingBuffer);
                        if (newVolume < 0) newVolume = 0;
                        if (newVolume > 100) newVolume = 100;
                        game.volume = newVolume;
                    }

                    game.editingMode = false;
                    game.editingField = -1;
                    game.editingBuffer = "";
                }
                else if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    game.editingMode = false;
                    game.editingField = -1;
                    game.editingBuffer = "";
                }
                else if (e.key.keysym.sym == SDLK_BACKSPACE && !game.editingBuffer.empty())
                {
                    game.editingBuffer.pop_back();
                }
            }
        }

        if (e.type == SDL_KEYUP)
        {
            game.pressedKeys[e.key.keysym.scancode] = 0;
        }

        if (e.type == SDL_MOUSEMOTION)
        {
            game.mouseX = e.motion.x;
            game.mouseY = e.motion.y;

            // ===== حرکت اسپرایت هنگام Drag =====
            if (game.isDragging && game.activeSpriteIndex >= 0)
            {
                Sprite* active = getActiveSprite(game);
                if (!active) continue;

                int stagePanelX = game.codeAreaX + game.codeAreaWidth + 10;
                int stagePanelWidth = game.screenWidth - stagePanelX - 5;
                if (stagePanelWidth > 500) stagePanelWidth = 500;

                int stageX = stagePanelX + 5;
                int stageY = 70;
                int stageWidth = stagePanelWidth - 20;
                int stageHeight = 300;

                double newScreenX = e.motion.x - game.dragOffsetX;
                double newScreenY = e.motion.y - game.dragOffsetY;

                if (newScreenX < stageX) newScreenX = stageX;
                if (newScreenY < stageY) newScreenY = stageY;
                if (newScreenX + (active->w * stageWidth / game.screenWidth) > stageX + stageWidth)
                    newScreenX = stageX + stageWidth - (active->w * stageWidth / game.screenWidth);
                if (newScreenY + (active->h * stageHeight / game.screenHeight) > stageY + stageHeight)
                    newScreenY = stageY + stageHeight - (active->h * stageHeight / game.screenHeight);

                active->x = (newScreenX - stageX) * game.screenWidth / stageWidth;
                active->y = (newScreenY - stageY) * game.screenHeight / stageHeight;

                if (active->x < 0) active->x = 0;
                if (active->y < 0) active->y = 0;
                if (active->x + active->w > game.screenWidth)
                    active->x = game.screenWidth - active->w;
                if (active->y + active->h > game.screenHeight)
                    active->y = game.screenHeight - active->h;
            }

            // ===== حرکت بلوک در حال درگ =====
            if (game.isDraggingBlock && game.draggedBlock)
            {
                game.draggedBlock->x = e.motion.x - game.dragBlockOffsetX;
                game.draggedBlock->y = e.motion.y - game.dragBlockOffsetY;

                // بررسی برای چسبیدن به بلوک‌های دیگه
                game.hoverBlock = nullptr;
                for (auto& block : game.program)
                {
                    if (&block == game.draggedBlock) continue;

                    if (block.width > 0 && block.height > 0 &&
                        abs(game.draggedBlock->y - (block.y + block.height)) < 15 &&
                        abs(game.draggedBlock->x - block.x) < 50)
                    {
                        game.hoverBlock = &block;
                        break;
                    }
                }
            }
        }

        // ===== دریافت متن از کاربر =====
        if (e.type == SDL_TEXTINPUT && !game.isDragging && !game.isDraggingBlock)
        {
            bool blockEditing = false;

            for (int i = 0; i < game.program.size(); i++)
            {
                if (game.program[i].editingMode)
                {
                    blockEditing = true;
                    Block& b = game.program[i];

                    if (b.type == SET_COMPARISON)
                    {
                        b.editingBuffer += e.text.text;
                        break;
                    }

                    else if (b.type == COSTUME_NUMBER || b.type == SPRITE_SIZE || b.type == BACKDROP_NUMBER)
                    {
                        b.editingBuffer += e.text.text;
                    }

                    // تشخیص نوع بلوک برای ویرایش
                    if (b.type == WHEN_I_RECEIVE || b.type == BROADCAST || b.type == BROADCAST_AND_WAIT)
                    {
                        // ویرایش messageName (فیلد متنی)
                        b.editingBuffer += e.text.text;
                    }
                    else if (b.type == GO_FORWARD_LAYERS || b.type == GO_BACKWARD_LAYERS)
                    {
                        for (char c : string(e.text.text))
                        {
                            if (c >= '0' && c <= '9')
                                b.editingBuffer += c;
                        }
                    }
                    else if (b.type == REPEAT || b.type == IF_THEN || b.type == IF_THEN_ELSE || b.type == WAIT_UNTIL || b.type == REPEAT_UNTIL)
                    {
                        // این بلوک‌ها عددی/بولی هستند
                        for (char c : string(e.text.text))
                        {
                            if (c >= '0' && c <= '9')
                                b.editingBuffer += c;
                            else if (c == '-' && b.editingBuffer.empty())
                                b.editingBuffer += '-';
                                // اجازه ورود حروف برای true/false هم می‌دهیم
                            else if (b.type != REPEAT && (c == 't' || c == 'r' || c == 'u' || c == 'e' || c == 'f' || c == 'a' || c == 'l' || c == 's'))
                                b.editingBuffer += c;
                        }
                    }
                    else
                    {
                        // تشخیص فیلد متنی برای SAY و THINK
                        bool isTextField = (b.type == SAY || b.type == THINK ||
                                            ((b.type == SAY_FOR || b.type == THINK_FOR) && b.editingField == 0));

                        if (isTextField)
                        {
                            // فیلد متنی: هر کاراکتری مجاز
                            b.editingBuffer += e.text.text;
                        }
                        else
                        {
                            // فیلد عددی: فقط اعداد، نقطه و منفی
                            for (char c : string(e.text.text))
                            {
                                if (c >= '0' && c <= '9')
                                    b.editingBuffer += c;
                                else if (c == '.' && b.editingBuffer.find('.') == string::npos)
                                    b.editingBuffer += '.';
                                else if (c == '-' && b.editingBuffer.empty())
                                    b.editingBuffer += '-';
                            }
                        }
                    }
                    break;
                }
            }

            // اگر هیچ بلوکی در حال ویرایش نیست، شاید در حال ویرایش مشخصات اسپرایت باشیم
            if (!blockEditing && game.editingMode)
            {
                if (game.editingField == 0) // اسم اسپرایت (فیلد متنی)
                {
                    game.editingBuffer += e.text.text;
                }
                else // سایر فیلدهای عددی (x, y, size, direction, volume)
                {
                    for (char c : string(e.text.text))
                    {
                        if (c >= '0' && c <= '9')
                            game.editingBuffer += c;
                        else if (c == '.' && game.editingBuffer.find('.') == string::npos)
                            game.editingBuffer += '.';
                        else if (c == '-' && game.editingBuffer.empty())
                            game.editingBuffer += '-';
                    }
                }
            }
        }

        // ===== کلیک موس =====
        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            game.mousePressed = true;
            int mx = e.button.x;
            int my = e.button.y;

            // ===== محاسبه مختصات استیج =====
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

            // ذخیره مختصات پنل‌ها
            game.categoriesPanelX = 10;
            game.categoriesPanelY = 70;
            game.categoriesPanelWidth = categoriesPanelWidth;
            game.categoriesPanelHeight = game.screenHeight - 150;

            game.examplesPanelX = examplesPanelX;
            game.examplesPanelY = 70;
            game.examplesPanelWidth = examplesPanelWidth;
            game.examplesPanelHeight = game.screenHeight - 150;

            // ===== تشخیص کلیک روی بلوک برای Drag (چپ کلیک) =====
            if (e.button.button == SDL_BUTTON_LEFT)
            {
                Block* clickedBlock = nullptr;

                // بررسی کن که آیا کلیک روی پنل دسته‌بندی هست یا نه
                bool isOnCategoryPanel = (mx >= game.categoriesPanelX && mx <= game.categoriesPanelX + game.categoriesPanelWidth &&
                                          my >= game.categoriesPanelY && my <= game.categoriesPanelY + game.categoriesPanelHeight);

                // بررسی کن که آیا کلیک روی پنل بلوک‌ها (پالت) هست یا نه
                bool isOnExamplesPanel = (mx >= game.examplesPanelX && mx <= game.examplesPanelX + game.examplesPanelWidth &&
                                          my >= game.examplesPanelY && my <= game.examplesPanelY + game.examplesPanelHeight);

                // فقط اگه روی پنل دسته‌بندی نبود، بلوک‌های برنامه رو بررسی کن
                if (!isOnCategoryPanel)
                {
                    for (int i = game.program.size() - 1; i >= 0; i--)
                    {
                        Block& block = game.program[i];
                        if (block.width > 0 && block.height > 0 &&
                            mx >= block.x && mx <= block.x + block.width &&
                            my >= block.y && my <= block.y + block.height)
                        {
                            clickedBlock = &game.program[i];
                            break;
                        }
                    }
                }

                // بلوک‌های پالت رو بررسی کن (فقط وقتی روی پنل بلوک‌ها کلیک شده)
                if (!clickedBlock && isOnExamplesPanel)
                {
                    for (int i = game.paletteBlocks.size() - 1; i >= 0; i--)
                    {
                        Block& block = game.paletteBlocks[i];
                        // فقط بلوک‌هایی که کتگوری‌شون با کتگوری فعلی مطابقت داره
                        if (block.category == game.currentCategory)
                        {
                            if (block.width > 0 && block.height > 0 &&
                                mx >= block.x && mx <= block.x + block.width &&
                                my >= block.y && my <= block.y + block.height)
                            {
                                clickedBlock = &game.paletteBlocks[i];
                                break;
                            }
                        }
                    }
                }

                if (clickedBlock)
                {
                    game.isDraggingBlock = true;
                    game.draggedBlock = clickedBlock;
                    game.dragBlockOffsetX = mx - clickedBlock->x;
                    game.dragBlockOffsetY = my - clickedBlock->y;
                    game.dragBlockStartX = clickedBlock->x;
                    game.dragBlockStartY = clickedBlock->y;

                    // ذخیره اطلاعات والد اصلی برای بلوک‌های داخل Code Area
                    if (clickedBlock->parent)
                    {
                        game.draggedBlockOriginalParent = clickedBlock->parent;
                        // پیدا کردن ایندکس در children
                        auto it = find(clickedBlock->parent->children.begin(), clickedBlock->parent->children.end(), clickedBlock);
                        if (it != clickedBlock->parent->children.end())
                            game.draggedBlockOriginalIndex = it - clickedBlock->parent->children.begin();
                        else
                            game.draggedBlockOriginalIndex = -1;
                    }
                    else
                    {
                        game.draggedBlockOriginalParent = nullptr;
                        game.draggedBlockOriginalIndex = -1;
                    }

                    // اگه بلوک از پالت بود، یه کپی بساز
                    bool isFromPalette = false;
                    for (auto& block : game.paletteBlocks)
                    {
                        if (&block == clickedBlock)
                        {
                            isFromPalette = true;
                            break;
                        }
                    }

                    if (isFromPalette)
                    {
                        // کپی کردن بلوک - فقط از روی پالت
                        Block newBlock = *clickedBlock;
                        newBlock.isDragging = true;
                        newBlock.parent = nullptr;
                        newBlock.children.clear();
                        newBlock.inCodeArea = false;
                        newBlock.x = mx - game.dragBlockOffsetX;
                        newBlock.y = my - game.dragBlockOffsetY;
                        game.program.push_back(newBlock);
                        game.draggedBlock = &game.program.back();

                        // ثبت هندلر برای بلوک WHEN_I_RECEIVE جدید
                        if (newBlock.type == WHEN_I_RECEIVE && !newBlock.messageName.empty())
                        {
                            game.messageHandlers[newBlock.messageName].push_back(game.program.size() - 1);
                            log_info(("Message handler registered for new block: " + newBlock.messageName).c_str());
                        }

                        log_info(("Block dragged from palette: " + newBlock.eventName + " (category: " + to_string(newBlock.category) + ")").c_str());
                    }
                    else
                    {
                        // جدا کردن از والد اگر چسبیده بود
                        if (game.draggedBlock->parent)
                        {
                            auto& siblings = game.draggedBlock->parent->children;
                            siblings.erase(remove(siblings.begin(), siblings.end(), game.draggedBlock), siblings.end());
                            game.draggedBlock->parent = nullptr;
                        }
                        game.draggedBlock->isDragging = true;
                    }
                }
            }
                // ===== راست کلیک برای ویرایش بلوک =====
            else if (e.button.button == SDL_BUTTON_RIGHT)
            {
                // پیدا کردن بلوکی که روی آن کلیک راست شده
                for (int i = game.program.size() - 1; i >= 0; i--)
                {
                    Block& block = game.program[i];
                    if (block.width > 0 && block.height > 0 &&
                        mx >= block.x && mx <= block.x + block.width &&
                        my >= block.y && my <= block.y + block.height)
                    {
                        // فعال کردن حالت ویرایش
                        block.editingMode = true;

                        // تشخیص نوع بلوک برای ویرایش
                        if (block.type == WHEN_I_RECEIVE || block.type == BROADCAST || block.type == BROADCAST_AND_WAIT)
                        {
                            // ویرایش messageName
                            block.editingField = 0;
                            block.editingBuffer = block.messageName;
                            log_info(("Editing message name: " + block.messageName).c_str());
                        }
                        else if (block.type == REPEAT)
                        {
                            // ویرایش تعداد تکرار
                            block.editingField = 0;
                            block.editingBuffer = to_string(block.repeatCount);
                            log_info(("Editing repeat count: " + block.editingBuffer).c_str());
                        }
                        else if (block.type == IF_THEN || block.type == IF_THEN_ELSE || block.type == WAIT_UNTIL || block.type == REPEAT_UNTIL)
                        {
                            // ویرایش شرط (پارامتر اول)
                            block.editingField = 0;
                            if (!block.parameters.empty())
                                block.editingBuffer = block.parameters[0].asString();
                            else
                                block.editingBuffer = "true";
                            log_info(("Editing condition: " + block.editingBuffer).c_str());
                        }
                        else if (block.type == GO_FORWARD_LAYERS || block.type == GO_BACKWARD_LAYERS)
                        {
                            block.editingMode = true;
                            block.editingField = 0;
                            if (!block.parameters.empty())
                                block.editingBuffer = to_string((int)block.parameters[0].asNumber());
                            else
                                block.editingBuffer = "1";
                            log_info(("Editing layers count: " + block.editingBuffer).c_str());
                        }
                        else if (block.type == COSTUME_NUMBER || block.type == SPRITE_SIZE || block.type == BACKDROP_NUMBER)
                        {
                            block.editingMode = true;
                            block.editingField = 0;
                            block.editingBuffer = block.variableName.empty() ? "myVar" : block.variableName;
                            log_info(("Editing variable name for looks reporter: " + block.editingBuffer).c_str());
                        }
                        else if (!block.parameters.empty())
                        {
                            // ویرایش پارامترهای عددی/متنی (بقیه بلوک‌ها)
                            block.editingField = 0;
                            if (block.type == SAY || block.type == THINK ||
                                block.type == SAY_FOR || block.type == THINK_FOR)
                            {
                                block.editingBuffer = block.parameters[0].asString();
                            }
                            else
                            {
                                block.editingBuffer = to_string((int)block.parameters[0].asNumber());
                            }
                            log_info(("Editing block: " + block.eventName).c_str());
                        }
                        else if (block.type == SET_COMPARISON)
                        {
                            // تقسیم بلوک به ۴ قسمت مساوی
                            int partWidth = block.width / 4;
                            int partIndex = (mx - block.x) / partWidth;
                            if (partIndex < 0) partIndex = 0;
                            if (partIndex > 3) partIndex = 3;
                            block.editingField = partIndex;

                            switch (partIndex) {
                                case 0: // عملگر
                                {
                                    const char* opNames[] = {"=", "≠", "<", "≤", ">", "≥"};
                                    block.editingBuffer = opNames[block.compareOp];
                                    break;
                                }
                                case 1: // متغیر خروجی
                                    block.editingBuffer = block.variableName.empty() ? "result" : block.variableName;
                                    break;
                                case 2: // سمت چپ
                                    if (!block.leftVar.empty())
                                        block.editingBuffer = block.leftVar;
                                    else if (!block.parameters.empty())
                                        block.editingBuffer = to_string((int)block.parameters[0].asNumber());
                                    else
                                        block.editingBuffer = "0";
                                    break;
                                case 3: // سمت راست
                                    if (!block.rightVar.empty())
                                        block.editingBuffer = block.rightVar;
                                    else if (block.parameters.size() > 1)
                                        block.editingBuffer = to_string((int)block.parameters[1].asNumber());
                                    else
                                        block.editingBuffer = "0";
                                    break;
                            }
                        }
                        break;
                    }
                }
            }

            // ===== تشخیص کلیک روی مشخصات اسپرایت برای ویرایش =====
            SDL_Rect stage = {stageX, stageY, stageWidth, stageHeight};

            int spritePanelY = stage.y + stage.h + 10;
            SDL_Rect spritePanel = {stagePanelX + 5, spritePanelY, stageWidth, 150};

            int textX = spritePanel.x + 70;
            int textY = spritePanel.y + 25;
            int lineHeight = 20;

            Sprite* active = getActiveSprite(game);
            if (active)
            {
                // کلیک روی اسم
                SDL_Rect nameRect = {textX, textY, 150, 18};
                if (mx >= nameRect.x && mx <= nameRect.x + nameRect.w &&
                    my >= nameRect.y && my <= nameRect.y + nameRect.h)
                {
                    game.editingMode = true;
                    game.editingField = 0;
                    game.editingBuffer = active->name;
                    log_info("Editing sprite name");
                }

                // کلیک روی X
                SDL_Rect xRect = {textX, textY + lineHeight, 100, 18};
                if (mx >= xRect.x && mx <= xRect.x + xRect.w &&
                    my >= xRect.y && my <= xRect.y + xRect.h)
                {
                    game.editingMode = true;
                    game.editingField = 1;
                    char buffer[50];
                    sprintf(buffer, "%.1f", active->x);
                    game.editingBuffer = buffer;
                    log_info("Editing sprite X");
                }

                // کلیک روی Y
                SDL_Rect yRect = {textX + 120, textY + lineHeight, 100, 18};
                if (mx >= yRect.x && mx <= yRect.x + yRect.w &&
                    my >= yRect.y && my <= yRect.y + yRect.h)
                {
                    game.editingMode = true;
                    game.editingField = 2;
                    char buffer[50];
                    sprintf(buffer, "%.1f", active->y);
                    game.editingBuffer = buffer;
                    log_info("Editing sprite Y");
                }

                // کلیک روی Size
                SDL_Rect sizeRect = {textX, textY + lineHeight * 2, 100, 18};
                if (mx >= sizeRect.x && mx <= sizeRect.x + sizeRect.w &&
                    my >= sizeRect.y && my <= sizeRect.y + sizeRect.h)
                {
                    game.editingMode = true;
                    game.editingField = 3;
                    char buffer[50];
                    sprintf(buffer, "%d", active->w);
                    game.editingBuffer = buffer;
                    log_info("Editing sprite size");
                }

                // کلیک روی Direction
                SDL_Rect dirRect = {textX, textY + lineHeight * 3, 100, 18};
                if (mx >= dirRect.x && mx <= dirRect.x + dirRect.w &&
                    my >= dirRect.y && my <= dirRect.y + dirRect.h)
                {
                    game.editingMode = true;
                    game.editingField = 4;
                    char buffer[50];
                    sprintf(buffer, "%.1f", active->direction);
                    game.editingBuffer = buffer;
                    log_info("Editing sprite direction");
                }

                // کلیک روی Visible toggle
                SDL_Rect visibleRect = {textX, textY + lineHeight * 4, 80, 18};
                if (mx >= visibleRect.x && mx <= visibleRect.x + visibleRect.w &&
                    my >= visibleRect.y && my <= visibleRect.y + visibleRect.h)
                {
                    active->visible = !active->visible;
                    log_info(("Visible toggled: " + string(active->visible ? "Yes" : "No")).c_str());
                }

                // کلیک روی Volume
                SDL_Rect volumeRect = {textX, textY + lineHeight * 5, 100, 18};
                if (mx >= volumeRect.x && mx <= volumeRect.x + volumeRect.w &&
                    my >= volumeRect.y && my <= volumeRect.y + volumeRect.h)
                {
                    game.editingMode = true;
                    game.editingField = 5;
                    char buffer[50];
                    sprintf(buffer, "%d", game.volume);
                    game.editingBuffer = buffer;
                    log_info("Editing volume");
                }
            }

            // ===== تشخیص کلیک روی دکمه‌های تغییر costume =====
            // ===== تشخیص کلیک روی دکمه‌های تغییر costume =====
            if (active && active->costumes.size() > 0)
            {
                int btnSize = 30;
                int spacing = 10;
                int totalWidth = (btnSize * 2) + spacing + 40;

                int costumeBtnX = spritePanel.x + stageWidth - totalWidth - 10;
                int costumeBtnY = spritePanel.y + 25;

                // دکمه قبلی
                SDL_Rect prevCostumeRect = {costumeBtnX, costumeBtnY, btnSize, btnSize};
                if (mx >= prevCostumeRect.x && mx <= prevCostumeRect.x + prevCostumeRect.w &&
                    my >= prevCostumeRect.y && my <= prevCostumeRect.y + prevCostumeRect.h)
                {
                    int newCostume = active->currentCostume - 1;
                    if (newCostume < 0) newCostume = active->costumes.size() - 1;

                    active->currentCostume = newCostume;
                    active->imagePath = active->costumes[newCostume];

                    if (active->texture)
                    {
                        SDL_DestroyTexture(active->texture);
                        active->texture = nullptr;
                    }
                    if (!active->imagePath.empty())
                    {
                        loadSpriteTexture(active, renderer, active->imagePath.c_str());
                    }
                    log_info(("Costume changed to: " + active->imagePath).c_str());
                }

                // دکمه بعدی
                SDL_Rect nextCostumeRect = {prevCostumeRect.x + btnSize + 40, costumeBtnY, btnSize, btnSize};
                if (mx >= nextCostumeRect.x && mx <= nextCostumeRect.x + nextCostumeRect.w &&
                    my >= nextCostumeRect.y && my <= nextCostumeRect.y + nextCostumeRect.h)
                {
                    int newCostume = (active->currentCostume + 1) % active->costumes.size();
                    active->currentCostume = newCostume;
                    active->imagePath = active->costumes[newCostume];

                    if (active->texture)
                    {
                        SDL_DestroyTexture(active->texture);
                        active->texture = nullptr;
                    }
                    if (!active->imagePath.empty())
                    {
                        loadSpriteTexture(active, renderer, active->imagePath.c_str());
                    }
                    log_info(("Costume changed to: " + active->imagePath).c_str());
                }
            }

            // ===== تشخیص کلیک روی اسپرایت‌ها برای Drag =====
            game.clickedSpriteIndex = -1;
            for (int i = game.sprites.size() - 1; i >= 0; i--)
            {
                Sprite& sprite = game.sprites[i];
                if (!sprite.visible) continue;

                double playerScreenX = stageX + (sprite.x * stageWidth / game.screenWidth);
                double playerScreenY = stageY + (sprite.y * stageHeight / game.screenHeight);
                double playerScreenW = sprite.w * stageWidth / game.screenWidth;
                double playerScreenH = sprite.h * stageHeight / game.screenHeight;

                if (mx >= playerScreenX && mx <= playerScreenX + playerScreenW &&
                    my >= playerScreenY && my <= playerScreenY + playerScreenH)
                {
                    game.clickedSpriteIndex = i;
                    game.spriteClicked = true;
                    game.isDragging = true;
                    game.dragOffsetX = mx - playerScreenX;
                    game.dragOffsetY = my - playerScreenY;

                    setActiveSprite(game, i);
                    log_info(("Sprite clicked: " + sprite.name).c_str());
                    break;
                }
            }

            // ریست وضعیت دکمه‌ها
            game.runButton.isPressed = false;
            game.pauseButton.isPressed = false;
            game.stepButton.isPressed = false;
            game.resetButton.isPressed = false;
            game.saveButton.isPressed = false;
            game.loadButton.isPressed = false;

            game.addSpriteBtn.isPressed = false;
            game.deleteSpriteBtn.isPressed = false;
            game.prevSpriteBtn.isPressed = false;
            game.nextSpriteBtn.isPressed = false;

            game.prevBackdropBtn.isPressed = false;
            game.nextBackdropBtn.isPressed = false;
            game.uploadBackdropBtn.isPressed = false;

            game.moveCategoryBtn.isPressed = false;
            game.looksCategoryBtn.isPressed = false;
            game.soundCategoryBtn.isPressed = false;
            game.eventsCategoryBtn.isPressed = false;
            game.controlCategoryBtn.isPressed = false;
            game.sensingCategoryBtn.isPressed = false;
            game.operatorsCategoryBtn.isPressed = false;
            game.variablesCategoryBtn.isPressed = false;
            game.penCategoryBtn.isPressed = false;

            // ===== دکمه‌های پایین صفحه =====
            int buttonY = game.screenHeight - 80;
            int buttonWidth = 100;
            int buttonHeight = 40;
            int buttonSpacing = 10;
            int startX = (game.screenWidth - (6 * (buttonWidth + buttonSpacing))) / 2;

            // دکمه Run
            if (mx >= game.runButton.x && mx <= game.runButton.x + game.runButton.w &&
                my >= game.runButton.y && my <= game.runButton.y + game.runButton.h)
            {
                game.runButton.isPressed = true;

                if (game.isPaused) {
                    // ادامه از جایی که متوقف شده
                    game.isPaused = false;
                    game.isRunningCode = true;
                } else {
                    // شروع جدید
                    game.isRunningCode = true;
                    game.greenFlagPressed = true;
                    game.currentBlockIndex = 0;
                    game.isExecutingBlock = false;
                    game.remainingMove = 0;
                    game.isWaiting = false;
                    game.isShowingMessage = false;
                    game.repeatCountStack.clear();
                    game.repeatStartStack.clear();
                }
                log_info("Run button clicked");
            }

            // ===== دکمه Pause =====
            if (mx >= game.pauseButton.x && mx <= game.pauseButton.x + game.pauseButton.w &&
                my >= game.pauseButton.y && my <= game.pauseButton.y + game.pauseButton.h)
            {
                game.pauseButton.isPressed = true;
                if (game.isRunningCode) {
                    game.isPaused = true;
                    game.isRunningCode = false;
                }
                log_info("Pause button clicked");
            }

// ===== دکمه Step =====
// ===== دکمه Step =====
            if (mx >= game.stepButton.x && mx <= game.stepButton.x + game.stepButton.w &&
                my >= game.stepButton.y && my <= game.stepButton.y + game.stepButton.h)
            {
                game.stepButton.isPressed = true;

                // اگر هیچ اسکریپتی فعال نیست، باید برنامه را راه‌اندازی کنیم
                if (game.scriptStartIndices.empty()) {
                    game.greenFlagPressed = true;
                }

                game.stepMode = true;
                game.isRunningCode = true;
                log_info("Step button clicked");
            }

            // ===== دکمه Reset =====
            if (mx >= game.resetButton.x && mx <= game.resetButton.x + game.resetButton.w &&
                my >= game.resetButton.y && my <= game.resetButton.y + game.resetButton.h)
            {
                game.resetButton.isPressed = true;

                // توقف اجرا
                game.isRunningCode = false;
                game.isPaused = false;
                game.stepMode = false;

                // ریست وضعیت اجرای بلوک‌ها
                game.currentBlockIndex = 0;
                game.isExecutingBlock = false;
                game.remainingMove = 0;
                game.isWaiting = false;
                game.isShowingMessage = false;

                // پاک کردن پشته‌ها
                game.repeatCountStack.clear();
                game.repeatStartStack.clear();

                // پاک کردن پشته شرط‌ها
                while (!game.ifStack.empty()) game.ifStack.pop();

                // پاک کردن اسکریپت‌های فعال
                game.scriptStartIndices.clear();
                game.scriptActive.clear();
                game.scriptCurrentBlock.clear();

                // پاک کردن متغیرها (مهم)
                game.variables.clear();

                // پاک کردن صف پیام‌ها
                while (!game.messageQueue.empty()) game.messageQueue.pop();

                // ریست وضعیت broadcast
                game.waitingForBroadcast = false;
                game.waitingScriptIndex = -1;

                // ریست وضعیت حسگرها
                game.waitingForAnswer = false;
                game.answer = "";
                game.currentQuestion = "";

                // ریست موقعیت اسپرایت‌ها
                for (auto& sprite : game.sprites)
                {
                    sprite.x = game.screenWidth / 2 - sprite.w / 2;
                    sprite.y = game.screenHeight / 2 - sprite.h / 2;
                    sprite.direction = 0;
                    sprite.message = "";
                    sprite.isThinking = false;
                    sprite.visible = true;
                }

                log_info("Reset button clicked");
            }

            // ===== دکمه Save =====
            if (mx >= game.saveButton.x && mx <= game.saveButton.x + game.saveButton.w &&
                my >= game.saveButton.y && my <= game.saveButton.y + game.saveButton.h)
            {
                game.saveButton.isPressed = true;

                std::string filename = showSaveFileDialog();
                if (!filename.empty())
                {
                    saveProject(game, filename);
                    log_info(("Project saved to: " + filename).c_str());
                }
                else
                {
                    log_info("Save cancelled by user");
                }
            }

            // ===== دکمه Load =====
            if (mx >= game.loadButton.x && mx <= game.loadButton.x + game.loadButton.w &&
                my >= game.loadButton.y && my <= game.loadButton.y + game.loadButton.h)
            {
                game.loadButton.isPressed = true;

                std::string filename = showOpenFileDialog();
                if (!filename.empty())
                {
                    loadProject(game, filename);
                    reloadAllTextures(game, renderer);
                    for (auto& sprite : game.sprites)
                    {
                        if (!sprite.imagePath.empty() && sprite.texture == nullptr)
                        {
                            loadSpriteTexture(&sprite, renderer, sprite.imagePath.c_str());
                        }
                    }
                    log_info(("Project loaded from: " + filename).c_str());
                }
                else
                {
                    log_info("Load cancelled by user");
                }
            }

            // ===== دکمه Toggle Variables =====
            if (mx >= game.loadButton.x + game.loadButton.w + 20 &&
                mx <= game.loadButton.x + game.loadButton.w + 20 + 60 &&
                my >= game.screenHeight - 80 &&
                my <= game.screenHeight - 80 + 40)
            {
                game.showVariables = !game.showVariables;
                log_info(("Toggle variables: " + string(game.showVariables ? "ON" : "OFF")).c_str());
            }

            int toggleBtnX = game.loadButton.x + game.loadButton.w + 20;
            int toggleBtnY = game.screenHeight - 80;
            int toggleBtnW = 60;
            int toggleBtnH = 40;

            // ===== دکمه Previous Backdrop =====
            if (mx >= game.prevBackdropBtn.x && mx <= game.prevBackdropBtn.x + game.prevBackdropBtn.w &&
                my >= game.prevBackdropBtn.y && my <= game.prevBackdropBtn.y + game.prevBackdropBtn.h)
            {
                game.prevBackdropBtn.isPressed = true;
                if (game.backdrops.size() > 0)
                {
                    game.currentBackdrop--;
                    if (game.currentBackdrop < 0)
                        game.currentBackdrop = game.backdrops.size() - 1;
                    log_info(("Backdrop changed to: " + game.backdrops[game.currentBackdrop].name).c_str());
                }
            }

            // ===== دکمه Next Backdrop =====
            if (mx >= game.nextBackdropBtn.x && mx <= game.nextBackdropBtn.x + game.nextBackdropBtn.w &&
                my >= game.nextBackdropBtn.y && my <= game.nextBackdropBtn.y + game.nextBackdropBtn.h)
            {
                game.nextBackdropBtn.isPressed = true;
                if (game.backdrops.size() > 0)
                {
                    game.currentBackdrop = (game.currentBackdrop + 1) % game.backdrops.size();
                    log_info(("Backdrop changed to: " + game.backdrops[game.currentBackdrop].name).c_str());
                }
            }

            // ===== دکمه Upload Backdrop =====
            if (mx >= game.uploadBackdropBtn.x && mx <= game.uploadBackdropBtn.x + game.uploadBackdropBtn.w &&
                my >= game.uploadBackdropBtn.y && my <= game.uploadBackdropBtn.y + game.uploadBackdropBtn.h)
            {
                game.uploadBackdropBtn.isPressed = true;

                string filename = showOpenFileDialog();
                if (!filename.empty())
                {
                    string ext = filename.substr(filename.find_last_of(".") + 1);
                    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
                    {
                        addCustomBackdrop(game, renderer, filename.c_str());
                        log_info(("Upload backdrop: " + filename).c_str());
                    }
                    else
                    {
                        log_warning("Invalid image format. Please use PNG, JPG, or BMP");
                    }
                }
                else
                {
                    log_info("Upload cancelled by user");
                }
            }

            // ===== دکمه New Project در نوار منو =====
            if (e.button.button == SDL_BUTTON_LEFT &&
                mx >= game.newProjectBtn.x && mx <= game.newProjectBtn.x + game.newProjectBtn.w &&
                my >= game.newProjectBtn.y && my <= game.newProjectBtn.y + game.newProjectBtn.h)
            {
                game.newProjectBtn.isPressed = true;

                int result = MessageBox(NULL,
                                        "Do you want to save the current project?",
                                        "New Project",
                                        MB_YESNO | MB_ICONQUESTION);

                if (result == IDYES)
                {
                    std::string filename = showSaveFileDialog();
                    if (!filename.empty())
                    {
                        saveProject(game, filename);
                        log_info(("Project saved before new: " + filename).c_str());
                    }
                }

                // فراخوانی تابع ریست (که باید در ui.cpp یا engine.cpp تعریف شود)
                resetToNewProject(game, renderer);

                log_info("New project button clicked");
            }

            // ===== دکمه‌های مدیریت اسپرایت =====
            int spriteBtnStartX = game.screenWidth - 350;

            if (mx >= game.addSpriteBtn.x && mx <= game.addSpriteBtn.x + game.addSpriteBtn.w &&
                my >= game.addSpriteBtn.y && my <= game.addSpriteBtn.y + game.addSpriteBtn.h)
            {
                game.addSpriteBtn.isPressed = true;
                addSprite(game, renderer, ("Sprite" + to_string(game.sprites.size() + 1)).c_str(), "cat.png");
                log_info("Add sprite clicked");
            }

            if (mx >= game.deleteSpriteBtn.x && mx <= game.deleteSpriteBtn.x + game.deleteSpriteBtn.w &&
                my >= game.deleteSpriteBtn.y && my <= game.deleteSpriteBtn.y + game.deleteSpriteBtn.h)
            {
                game.deleteSpriteBtn.isPressed = true;
                if (game.sprites.size() > 1)
                {
                    removeSprite(game, game.activeSpriteIndex);
                    log_info("Delete sprite clicked");
                }
                else
                {
                    log_warning("Cannot delete last sprite");
                }
            }

            if (mx >= game.prevSpriteBtn.x && mx <= game.prevSpriteBtn.x + game.prevSpriteBtn.w &&
                my >= game.prevSpriteBtn.y && my <= game.prevSpriteBtn.y + game.prevSpriteBtn.h)
            {
                game.prevSpriteBtn.isPressed = true;
                if (game.sprites.size() > 0)
                {
                    int newIndex = game.activeSpriteIndex - 1;
                    if (newIndex < 0) newIndex = game.sprites.size() - 1;
                    setActiveSprite(game, newIndex);
                    log_info("Previous sprite");
                }
            }

            if (mx >= game.nextSpriteBtn.x && mx <= game.nextSpriteBtn.x + game.nextSpriteBtn.w &&
                my >= game.nextSpriteBtn.y && my <= game.nextSpriteBtn.y + game.nextSpriteBtn.h)
            {
                game.nextSpriteBtn.isPressed = true;
                if (game.sprites.size() > 0)
                {
                    int newIndex = (game.activeSpriteIndex + 1) % game.sprites.size();
                    setActiveSprite(game, newIndex);
                    log_info("Next sprite");
                }
            }

            // ===== دکمه‌های دسته‌بندی =====
            if (mx >= game.moveCategoryBtn.x && mx <= game.moveCategoryBtn.x + game.moveCategoryBtn.w &&
                my >= game.moveCategoryBtn.y && my <= game.moveCategoryBtn.y + game.moveCategoryBtn.h)
            {
                game.moveCategoryBtn.isPressed = true;
                game.currentCategory = 0;
            }

            if (mx >= game.looksCategoryBtn.x && mx <= game.looksCategoryBtn.x + game.looksCategoryBtn.w &&
                my >= game.looksCategoryBtn.y && my <= game.looksCategoryBtn.y + game.looksCategoryBtn.h)
            {
                game.looksCategoryBtn.isPressed = true;
                game.currentCategory = 1;
            }

            if (mx >= game.soundCategoryBtn.x && mx <= game.soundCategoryBtn.x + game.soundCategoryBtn.w &&
                my >= game.soundCategoryBtn.y && my <= game.soundCategoryBtn.y + game.soundCategoryBtn.h)
            {
                game.soundCategoryBtn.isPressed = true;
                game.currentCategory = 2;
            }

            if (mx >= game.eventsCategoryBtn.x && mx <= game.eventsCategoryBtn.x + game.eventsCategoryBtn.w &&
                my >= game.eventsCategoryBtn.y && my <= game.eventsCategoryBtn.y + game.eventsCategoryBtn.h)
            {
                game.eventsCategoryBtn.isPressed = true;
                game.currentCategory = 3;
            }

            if (mx >= game.controlCategoryBtn.x && mx <= game.controlCategoryBtn.x + game.controlCategoryBtn.w &&
                my >= game.controlCategoryBtn.y && my <= game.controlCategoryBtn.y + game.controlCategoryBtn.h)
            {
                game.controlCategoryBtn.isPressed = true;
                game.currentCategory = 4;
            }

            if (mx >= game.sensingCategoryBtn.x && mx <= game.sensingCategoryBtn.x + game.sensingCategoryBtn.w &&
                my >= game.sensingCategoryBtn.y && my <= game.sensingCategoryBtn.y + game.sensingCategoryBtn.h)
            {
                game.sensingCategoryBtn.isPressed = true;
                game.currentCategory = 5;
            }

            if (mx >= game.operatorsCategoryBtn.x && mx <= game.operatorsCategoryBtn.x + game.operatorsCategoryBtn.w &&
                my >= game.operatorsCategoryBtn.y && my <= game.operatorsCategoryBtn.y + game.operatorsCategoryBtn.h)
            {
                game.operatorsCategoryBtn.isPressed = true;
                game.currentCategory = 6;
            }

            if (mx >= game.variablesCategoryBtn.x && mx <= game.variablesCategoryBtn.x + game.variablesCategoryBtn.w &&
                my >= game.variablesCategoryBtn.y && my <= game.variablesCategoryBtn.y + game.variablesCategoryBtn.h)
            {
                game.variablesCategoryBtn.isPressed = true;
                game.currentCategory = 7;
            }
            if (mx >= game.penCategoryBtn.x && mx <= game.penCategoryBtn.x + game.penCategoryBtn.w &&
                my >= game.penCategoryBtn.y && my <= game.penCategoryBtn.y + game.penCategoryBtn.h)
            {
                game.penCategoryBtn.isPressed = true;
                game.currentCategory = 8;
            }
        }

        // ===== رها کردن کلیک =====
        if (e.type == SDL_MOUSEBUTTONUP)
        {
            if (e.button.button == SDL_BUTTON_LEFT && game.isDraggingBlock && game.draggedBlock)
            {
                // بررسی کن draggedBlock هنوز وجود داره
                bool blockStillExists = false;
                for (auto& block : game.program)
                {
                    if (&block == game.draggedBlock)
                    {
                        blockStillExists = true;
                        break;
                    }
                }

                if (blockStillExists)
                {
                    int mx = e.button.x;
                    int my = e.button.y;

                    // بررسی کن که آیا در منطقه حذف (Trash) رها شده
                    bool inTrash = (mx >= game.trashX && mx <= game.trashX + game.trashW &&
                                    my >= game.trashY && my <= game.trashY + game.trashH);

                    if (inTrash)
                    {
                        // قبل از حذف، اگر بلوک از نوع WHEN_I_RECEIVE است، آن را از هندلرها حذف کن
                        if (game.draggedBlock->type == WHEN_I_RECEIVE && !game.draggedBlock->messageName.empty())
                        {
                            // پیدا کردن ایندکس بلوک در program
                            for (size_t idx = 0; idx < game.program.size(); ++idx)
                            {
                                if (&game.program[idx] == game.draggedBlock)
                                {
                                    string msgName = game.draggedBlock->messageName;
                                    auto& vec = game.messageHandlers[msgName];
                                    vec.erase(remove(vec.begin(), vec.end(), idx), vec.end());
                                    if (vec.empty()) {
                                        game.messageHandlers.erase(msgName);
                                    }
                                    break;
                                }
                            }
                        }
                        // حذف بلوک
                        for (auto it = game.program.begin(); it != game.program.end(); ++it)
                        {
                            if (&(*it) == game.draggedBlock)
                            {
                                game.program.erase(it);
                                break;
                            }
                        }
                        log_info("Block deleted by dropping on trash");
                    }
                    else
                    {
                        // بررسی کن که آیا بلوک در Code Area هست یا نه
                        bool inCodeArea = (game.draggedBlock->x >= game.codeAreaX &&
                                           game.draggedBlock->x <= game.codeAreaX + game.codeAreaWidth &&
                                           game.draggedBlock->y >= game.codeAreaY &&
                                           game.draggedBlock->y <= game.codeAreaY + game.codeAreaHeight);

                        if (inCodeArea)
                        {
                            // در Code Area: چسبیدن به والد اگر hoverBlock باشد
                            if (game.hoverBlock)
                            {
                                // جدا کردن از والد قبلی
                                if (game.draggedBlock->parent)
                                {
                                    auto& siblings = game.draggedBlock->parent->children;
                                    siblings.erase(remove(siblings.begin(), siblings.end(), game.draggedBlock), siblings.end());
                                }

                                // چسبیدن به والد جدید
                                game.draggedBlock->parent = game.hoverBlock;
                                game.hoverBlock->children.push_back(game.draggedBlock);

                                // تنظیم موقعیت
                                game.draggedBlock->x = game.hoverBlock->x;
                                game.draggedBlock->y = game.hoverBlock->y + game.hoverBlock->height;
                            }
                            game.draggedBlock->inCodeArea = true;
                        }
                        else
                        {
                            // خارج از Code Area و خارج از Trash
                            bool isFromPalette = (game.draggedBlock->paletteId >= 0);
                            if (isFromPalette)
                            {
                                // بلوک از پالت: حذف کن
                                for (auto it = game.program.begin(); it != game.program.end(); ++it)
                                {
                                    if (&(*it) == game.draggedBlock)
                                    {
                                        game.program.erase(it);
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                // بلوک از Code Area: برگردان به موقعیت اصلی
                                game.draggedBlock->x = game.dragBlockStartX;
                                game.draggedBlock->y = game.dragBlockStartY;
                                // برگرداندن به والد اصلی
                                if (game.draggedBlockOriginalParent)
                                {
                                    game.draggedBlock->parent = game.draggedBlockOriginalParent;
                                    if (game.draggedBlockOriginalIndex >= 0 &&
                                        game.draggedBlockOriginalIndex <= game.draggedBlockOriginalParent->children.size())
                                    {
                                        game.draggedBlockOriginalParent->children.insert(
                                                game.draggedBlockOriginalParent->children.begin() + game.draggedBlockOriginalIndex,
                                                game.draggedBlock);
                                    }
                                    else
                                    {
                                        game.draggedBlockOriginalParent->children.push_back(game.draggedBlock);
                                    }
                                }
                                else
                                {
                                    game.draggedBlock->parent = nullptr;
                                }
                                game.draggedBlock->inCodeArea = true; // فرض می‌کنیم اصلی در Code Area بوده
                            }
                        }
                    }

                    if (game.draggedBlock)
                        game.draggedBlock->isDragging = false;
                }

                game.isDraggingBlock = false;
                game.draggedBlock = nullptr;
                game.hoverBlock = nullptr;
                game.draggedBlockOriginalParent = nullptr;
                game.draggedBlockOriginalIndex = -1;
            }

            game.mousePressed = false;
            game.spriteClicked = false;
            game.isDragging = false;

            game.runButton.isPressed = false;
            game.pauseButton.isPressed = false;
            game.stepButton.isPressed = false;
            game.resetButton.isPressed = false;
            game.saveButton.isPressed = false;
            game.loadButton.isPressed = false;

            game.addSpriteBtn.isPressed = false;
            game.deleteSpriteBtn.isPressed = false;
            game.prevSpriteBtn.isPressed = false;
            game.nextSpriteBtn.isPressed = false;

            game.prevBackdropBtn.isPressed = false;
            game.nextBackdropBtn.isPressed = false;
            game.uploadBackdropBtn.isPressed = false;

            game.moveCategoryBtn.isPressed = false;
            game.looksCategoryBtn.isPressed = false;
            game.soundCategoryBtn.isPressed = false;
            game.eventsCategoryBtn.isPressed = false;
            game.controlCategoryBtn.isPressed = false;
            game.sensingCategoryBtn.isPressed = false;
            game.operatorsCategoryBtn.isPressed = false;
            game.variablesCategoryBtn.isPressed = false;
            game.penCategoryBtn.isPressed = false;
        }
    }

    // حرکت با کلیدهای جهت‌دار
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    double moveSpeed = 3.0;

    Sprite* active = getActiveSprite(game);
    if (active)
    {
        bool moved = false;
        double oldX = active->x;
        double oldY = active->y;

        if (keys[SDL_SCANCODE_UP]) {
            active->y -= moveSpeed;
            moved = true;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            active->y += moveSpeed;
            moved = true;
        }
        if (keys[SDL_SCANCODE_LEFT]) {
            active->x -= moveSpeed;
            moved = true;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            active->x += moveSpeed;
            moved = true;
        }

        // محدودیت مرزها
        if (active->x < 0) active->x = 0;
        if (active->y < 0) active->y = 0;
        if (active->x + active->w > game.screenWidth)
            active->x = game.screenWidth - active->w;
        if (active->y + active->h > game.screenHeight)
            active->y = game.screenHeight - active->h;

        // اگر حرکت کرده و pen پایین است، نقطه اضافه کن
        if (moved && game.penDown)
        {
            int stagePanelX = game.codeAreaX + game.codeAreaWidth + 10;
            int stagePanelWidth = game.screenWidth - stagePanelX - 5;
            if (stagePanelWidth > 500) stagePanelWidth = 500;

            int stageX = stagePanelX + 5;
            int stageY = 70;
            int stageWidth = stagePanelWidth - 20;
            int stageHeight = 300;

            int spriteStageX = stageX + (int)((active->x / game.screenWidth) * stageWidth);
            int spriteStageY = stageY + (int)((active->y / game.screenHeight) * stageHeight);

            PenPoint p;
            p.x = spriteStageX;
            p.y = spriteStageY;
            p.r = game.penR;
            p.g = game.penG;
            p.b = game.penB;
            p.a = 255;
            p.size = game.penSize;
            game.penPoints.push_back(p);
        }
    }
}



// ==================== تابع رندر ====================
void render(SDL_Renderer* renderer, GameState& game)
{
    // پس‌زمینه اصلی
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderClear(renderer);

    // نوار منوی بالا
    SDL_Rect menuBar = {0, 0, game.screenWidth, 60};
    SDL_SetRenderDrawColor(renderer, 70, 150, 70, 255);
    SDL_RenderFillRect(renderer, &menuBar);
    SDL_Color white = {255,255,255,255};

    if (game.logoTexture)
    {
        SDL_Rect logoRect = {20, 10, 40, 40};
        SDL_RenderCopy(renderer, game.logoTexture, NULL, &logoRect);
    }
    else
    {
        filledCircleRGBA(renderer, 40, 30, 20, 255, 255, 255, 255);
    }
    renderText(renderer, "Sharif University of Technology", 70, 20, white);
    int textWidth = 200; // تخمینی
    int btnX = 70 + textWidth + 20; // 70 موقعیت متن + عرض متن + فاصله
    int btnY = 15;
    int btnW = 60;
    int btnH = 30;

// ذخیره مختصات در GameState برای تشخیص کلیک
    game.newProjectBtn.x = btnX;
    game.newProjectBtn.y = btnY;
    game.newProjectBtn.w = btnW;
    game.newProjectBtn.h = btnH;

// رسم دکمه
    if (game.newProjectBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 100, 200, 255);
    else
        SDL_SetRenderDrawColor(renderer, 150, 150, 255, 255);
    SDL_Rect newRect = {btnX, btnY, btnW, btnH};
    SDL_RenderFillRect(renderer, &newRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &newRect);
    renderText(renderer, "New", btnX + 15, btnY + 8, white);

    if (game.isRunningCode)
        filledCircleRGBA(renderer, game.screenWidth - 100, 30, 15, 0, 255, 0, 255);
    else
        filledCircleRGBA(renderer, game.screenWidth - 100, 30, 15, 255, 0, 0, 255);

    // ========== ۱. پنل دسته‌بندی (سمت چپ) ==========
    int categoriesPanelWidth = 180;
    SDL_Rect categoriesPanel = {10, 70, categoriesPanelWidth, game.screenHeight - 150};
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderFillRect(renderer, &categoriesPanel);
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderDrawRect(renderer, &categoriesPanel);

    SDL_Color black = {0,0,0,255};
    renderText(renderer, "Categories", 15, 75, black);

    Uint8 catColors[9][3] = {
            {70, 120, 255},   // Motion
            {100, 150, 255},  // Looks
            {200, 50, 200},   // Sound
            {255, 200, 50},   // Events
            {255, 140, 0},    // Control
            {0, 200, 200},    // Sensing
            {70, 200, 70},    // Operators
            {255, 100, 50},   // Variables
            {0, 200, 0}       // Pen (سبز پررنگ)
    };

    const char* catNames[] = {
            "Motion", "Looks", "Sound", "Events",
            "Control", "Sensing", "Operators", "Variables", "Pen"
    };

    Button* catButtons[] = {
            &game.moveCategoryBtn, &game.looksCategoryBtn, &game.soundCategoryBtn,
            &game.eventsCategoryBtn, &game.controlCategoryBtn, &game.sensingCategoryBtn,
            &game.operatorsCategoryBtn, &game.variablesCategoryBtn, &game.penCategoryBtn
    };

    for (int i = 0; i < 9; i++)
    {
        SDL_Rect catRect = {catButtons[i]->x, catButtons[i]->y, catButtons[i]->w, catButtons[i]->h};

        if (catButtons[i]->isPressed || game.currentCategory == i)
        {
            SDL_SetRenderDrawColor(renderer, catColors[i][0]/2, catColors[i][1]/2, catColors[i][2]/2, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, catColors[i][0], catColors[i][1], catColors[i][2], 255);
        }

        SDL_RenderFillRect(renderer, &catRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &catRect);

        renderText(renderer, catNames[i], catRect.x + 5, catRect.y + 10, white);
    }

    // ========== ۲. پنل بلوک‌های مثال (پالت) ==========
    int examplesPanelX = categoriesPanelWidth + 30;
    int examplesPanelWidth = 250;
    SDL_Rect examplesPanel = {examplesPanelX, 70, examplesPanelWidth, game.screenHeight - 150};
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderFillRect(renderer, &examplesPanel);
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderDrawRect(renderer, &examplesPanel);

    renderText(renderer, "Blocks", examplesPanelX + 10, 75, black);

    // رسم بلوک‌های پالت (با در نظر گرفتن CAT_PEN)
    int blockY = 110;
    for (auto& block : game.paletteBlocks)
    {
        if (block.category == game.currentCategory)
        {
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            const_cast<Block&>(block).x = blockRect.x;
            const_cast<Block&>(block).y = blockRect.y;
            const_cast<Block&>(block).width = blockRect.w;
            const_cast<Block&>(block).height = blockRect.h;

            // رنگ‌بندی براساس کتگوری
            switch(block.category)
            {
                case CAT_MOTION:
                    SDL_SetRenderDrawColor(renderer, 70, 120, 255, 255); break;
                case CAT_LOOKS:
                    SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255); break;
                case CAT_SOUND:
                    SDL_SetRenderDrawColor(renderer, 200, 50, 200, 255); break;
                case CAT_EVENTS:
                    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255); break;
                case CAT_CONTROL:
                    SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); break;
                case CAT_SENSING:
                    SDL_SetRenderDrawColor(renderer, 0, 200, 200, 255); break;
                case CAT_OPERATORS:
                    SDL_SetRenderDrawColor(renderer, 70, 200, 70, 255); break;
                case CAT_VARIABLES:
                    SDL_SetRenderDrawColor(renderer, 255, 100, 50, 255); break;
                case CAT_PEN:
                    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255); break; // سبز پررنگ
                default:
                    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
            }

            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, block.eventName.c_str(), blockRect.x + 5, blockRect.y + 10, black);

            blockY += 45;
        }
    }

    // ========== ۳. فضای کدنویسی (Code Area) ==========
    int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
    int codeAreaWidth = 600;
    SDL_Rect codeArea = {codeAreaX, 70, codeAreaWidth, game.screenHeight - 150};

    game.codeAreaX = codeAreaX;
    game.codeAreaY = 70;
    game.codeAreaWidth = codeAreaWidth;
    game.codeAreaHeight = game.screenHeight - 150;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &codeArea);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &codeArea);

    renderText(renderer, "Code Area", codeArea.x + 10, codeArea.y + 5, black);

    // رسم بلوک‌های برنامه (مشابه پالت با همان رنگ‌ها)
    // رسم بلوک‌های برنامه
    for (size_t i = 0; i < game.program.size(); i++)
    {
        Block& block = game.program[i];
        if (&block != game.draggedBlock || !game.isDraggingBlock)
        {
            if (block.width > 0 && block.height > 0)
            {
                SDL_Rect blockRect = {block.x, block.y, block.width, block.height};

                // رنگ‌بندی براساس کتگوری
                switch(block.category)
                {
                    case CAT_MOTION: SDL_SetRenderDrawColor(renderer, 70, 120, 255, 255); break;
                    case CAT_LOOKS: SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255); break;
                    case CAT_SOUND: SDL_SetRenderDrawColor(renderer, 200, 50, 200, 255); break;
                    case CAT_EVENTS: SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255); break;
                    case CAT_CONTROL: SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); break;
                    case CAT_SENSING: SDL_SetRenderDrawColor(renderer, 0, 200, 200, 255); break;
                    case CAT_OPERATORS: SDL_SetRenderDrawColor(renderer, 70, 200, 70, 255); break;
                    case CAT_VARIABLES: SDL_SetRenderDrawColor(renderer, 255, 100, 50, 255); break;
                    case CAT_PEN: SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255); break;
                    default: SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
                }

                SDL_RenderFillRect(renderer, &blockRect);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &blockRect);

                if (block.editingMode)
                {
                    string displayText = block.editingBuffer + "_";
                    renderText(renderer, displayText.c_str(), block.x + 5, block.y + 10, black);
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_Rect editRect = {block.x - 2, block.y - 2, block.width + 4, block.height + 4};
                    SDL_RenderDrawRect(renderer, &editRect);
                }
                else
                {
                    renderText(renderer, block.eventName.c_str(), block.x + 5, block.y + 10, black);
                }

                // ===== اینجا: حاشیه برای بلوک در حال اجرا =====
                if (i == game.lastExecutedBlock && game.isRunningCode)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_Rect execRect = {block.x - 3, block.y - 3, block.width + 6, block.height + 6};
                    SDL_RenderDrawRect(renderer, &execRect);
                    // یه بار دیگه بکش تا پررنگ‌تر بشه
                    SDL_RenderDrawRect(renderer, &execRect);
                }

                if (game.hoverBlock == &block)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_Rect highlightRect = {block.x - 2, block.y - 2, block.width + 4, block.height + 4};
                    SDL_RenderDrawRect(renderer, &highlightRect);
                }
            }
        }
    }

    // رسم بلوک در حال درگ
    if (game.isDraggingBlock && game.draggedBlock && game.draggedBlock->width > 0 && game.draggedBlock->height > 0)
    {
        SDL_Rect dragRect = {game.draggedBlock->x, game.draggedBlock->y,
                             game.draggedBlock->width, game.draggedBlock->height};

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        switch(game.draggedBlock->category)
        {
            case CAT_MOTION:
                SDL_SetRenderDrawColor(renderer, 70, 120, 255, 128); break;
            case CAT_LOOKS:
                SDL_SetRenderDrawColor(renderer, 100, 150, 255, 128); break;
            case CAT_SOUND:
                SDL_SetRenderDrawColor(renderer, 200, 50, 200, 128); break;
            case CAT_EVENTS:
                SDL_SetRenderDrawColor(renderer, 255, 200, 50, 128); break;
            case CAT_CONTROL:
                SDL_SetRenderDrawColor(renderer, 255, 140, 0, 128); break;
            case CAT_SENSING:
                SDL_SetRenderDrawColor(renderer, 0, 200, 200, 128); break;
            case CAT_OPERATORS:
                SDL_SetRenderDrawColor(renderer, 70, 200, 70, 128); break;
            case CAT_VARIABLES:
                SDL_SetRenderDrawColor(renderer, 255, 100, 50, 128); break;
            case CAT_PEN:
                SDL_SetRenderDrawColor(renderer, 0, 200, 0, 128); break;
            default:
                SDL_SetRenderDrawColor(renderer, 150, 150, 150, 128);
        }

        SDL_RenderFillRect(renderer, &dragRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &dragRect);
        renderText(renderer, game.draggedBlock->eventName.c_str(),
                   game.draggedBlock->x + 5, game.draggedBlock->y + 10, black);

        if (game.hoverBlock)
        {
            int snapY = game.hoverBlock->y + game.hoverBlock->height;
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawLine(renderer, game.codeAreaX, snapY,
                               game.codeAreaX + game.codeAreaWidth, snapY);
        }
    }

    // رسم منطقه حذف (Trash)
    SDL_Rect trashRect = {game.trashX, game.trashY, game.trashW, game.trashH};
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 200);
    SDL_RenderFillRect(renderer, &trashRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &trashRect);
    renderText(renderer, "X", trashRect.x + 25, trashRect.y + 20, white);

    // ========== ۴. پنل استیج و اسپرایت ==========
    int stagePanelX = codeAreaX + codeAreaWidth + 10;
    int stagePanelWidth = game.screenWidth - stagePanelX - 5;
    if (stagePanelWidth > 500) stagePanelWidth = 500;

    int stageWidth = stagePanelWidth - 20;
    int stageHeight = 300;
    SDL_Rect stage = {stagePanelX + 5, 70, stageWidth, stageHeight};

    if (game.backdrops.size() > 0 && game.currentBackdrop >= 0 && game.currentBackdrop < game.backdrops.size())
    {
        Backdrop& current = game.backdrops[game.currentBackdrop];
        if (current.texture)
        {
            SDL_Rect stageRect = {stage.x, stage.y, stage.w, stage.h};
            SDL_RenderCopy(renderer, current.texture, NULL, &stageRect);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
            SDL_RenderFillRect(renderer, &stage);
        }
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
        SDL_RenderFillRect(renderer, &stage);
    }

    game.stageX = stage.x;
    game.stageY = stage.y;
    game.stageW = stage.w;
    game.stageH = stage.h;

// رسم خطوط Pen (با دایره‌های کوچک به جای خط)
    if (game.penPoints.size() >= 1)
    {
        for (size_t i = 0; i < game.penPoints.size(); i++)
        {
            const PenPoint& p = game.penPoints[i];

            // رسم دایره با مختصات stage (نه p.x, p.y)
            filledCircleRGBA(renderer, p.x, p.y,
                             p.size,
                             p.r, p.g, p.b, p.a);
        }
    }
    else if (game.penPoints.size() == 1)
    {
        const PenPoint& p = game.penPoints[0];
        int stageX = stage.x + (int)((p.x / game.screenWidth) * stage.w);
        int stageY = stage.y + (int)((p.y / game.screenHeight) * stage.h);

        filledCircleRGBA(renderer, stageX, stageY, p.size, p.r, p.g, p.b, p.a);
    }

    vector<int> spriteIndices(game.sprites.size());
    for (int i = 0; i < game.sprites.size(); i++) spriteIndices[i] = i;

// مرتب‌سازی بر اساس layer (صعودی)
    sort(spriteIndices.begin(), spriteIndices.end(),
         [&](int a, int b) { return game.sprites[a].layer < game.sprites[b].layer; });

    // رسم اسپرایت‌ها
    for (int idx : spriteIndices)
    {
        Sprite& sprite = game.sprites[idx];
        if (!sprite.visible) continue;

        int spriteStageX = stage.x + (int)((sprite.x / game.screenWidth) * stage.w);
        int spriteStageY = stage.y + (int)((sprite.y / game.screenHeight) * stage.h);

        if (spriteStageX < stage.x) spriteStageX = stage.x;
        if (spriteStageY < stage.y) spriteStageY = stage.y;
        if (spriteStageX + sprite.w/2 > stage.x + stage.w)
            spriteStageX = stage.x + stage.w - sprite.w/2;
        if (spriteStageY + sprite.h/2 > stage.y + stage.h)
            spriteStageY = stage.y + stage.h - sprite.h/2;

        SDL_Rect spriteRect = {spriteStageX, spriteStageY, sprite.w/2, sprite.h/2};

        if (sprite.texture)
        {
            SDL_Point center = {spriteRect.w/2, spriteRect.h/2};
            double angle = sprite.direction;
            SDL_RenderCopyEx(renderer, sprite.texture, NULL, &spriteRect, angle, &center, SDL_FLIP_NONE);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
            SDL_RenderFillRect(renderer, &spriteRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &spriteRect);
        }

        string spriteName = sprite.name;
        int nameX = spriteStageX + (sprite.w/4) - (spriteName.length() * 4);
        int nameY = spriteStageY - 15;

        if (game.activeSpriteIndex >= 0 && &sprite == &game.sprites[game.activeSpriteIndex])
        {
            SDL_Color activeColor = {0, 100, 255, 255};
            renderText(renderer, spriteName.c_str(), nameX, nameY, activeColor);
        }
        else
        {
            renderText(renderer, spriteName.c_str(), nameX, nameY, black);
        }

        if (!sprite.message.empty())
        {
            int bubbleWidth = 150;
            int bubbleHeight = 50;
            int bubbleX = spriteStageX + (sprite.w/4) - bubbleWidth/2;
            int bubbleY = nameY - bubbleHeight - 20;

            if (bubbleX < stage.x) bubbleX = stage.x;
            if (bubbleX + bubbleWidth > stage.x + stage.w)
                bubbleX = stage.x + stage.w - bubbleWidth;
            if (bubbleY < stage.y) bubbleY = stage.y + 5;

            SDL_Rect bubbleRect = {bubbleX, bubbleY, bubbleWidth, bubbleHeight};

            if (sprite.isThinking)
            {
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderFillRect(renderer, &bubbleRect);
                SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);

                for (int i = 0; i < bubbleRect.w; i += 5)
                {
                    SDL_RenderDrawPoint(renderer, bubbleRect.x + i, bubbleRect.y);
                    SDL_RenderDrawPoint(renderer, bubbleRect.x + i, bubbleRect.y + bubbleRect.h);
                }
                for (int i = 0; i < bubbleRect.h; i += 5)
                {
                    SDL_RenderDrawPoint(renderer, bubbleRect.x, bubbleRect.y + i);
                    SDL_RenderDrawPoint(renderer, bubbleRect.x + bubbleRect.w, bubbleRect.y + i);
                }
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &bubbleRect);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &bubbleRect);
            }

            string message = sprite.message;
            if (message.length() > 20)
                message = message.substr(0, 17) + "...";

            int textX = bubbleX + (bubbleWidth - message.length() * 8) / 2;
            int textY = bubbleY + (bubbleHeight - 16) / 2;
            renderText(renderer, message.c_str(), textX, textY, black);

            if (sprite.isThinking)
            {
                int circleX = spriteStageX + sprite.w/4;
                int circleY = bubbleY + bubbleHeight + 5;

                filledCircleRGBA(renderer, circleX - 15, circleY, 5, 220, 220, 220, 255);
                filledCircleRGBA(renderer, circleX, circleY + 5, 5, 220, 220, 220, 255);
                filledCircleRGBA(renderer, circleX + 15, circleY + 10, 5, 220, 220, 220, 255);
            }
            else
            {
                int x1 = spriteStageX + sprite.w/4;
                int y1 = bubbleY + bubbleHeight;
                int x2 = x1 - 10;
                int y2 = y1 + 10;
                int x3 = x1 + 10;
                int y3 = y1 + 10;

                filledTrigonRGBA(renderer, x1, y1, x2, y2, x3, y3, 255, 255, 255, 255);
                aatrigonRGBA(renderer, x1, y1, x2, y2, x3, y3, 0, 0, 0, 255);
            }
        }
    }

    // پنل اسپرایت
    int spritePanelY = stage.y + stage.h + 10;
    SDL_Rect spritePanel = {stagePanelX + 5, spritePanelY, stageWidth, 150};
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_RenderFillRect(renderer, &spritePanel);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer, &spritePanel);

    renderText(renderer, "Sprite Properties", spritePanel.x + 10, spritePanel.y + 5, black);

    Sprite* active = getActiveSprite(game);
    if (active)
    {
        SDL_Rect currentSprite = {spritePanel.x + 10, spritePanel.y + 25, 50, 50};

        if (active->texture)
        {
            SDL_RenderCopy(renderer, active->texture, NULL, &currentSprite);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
            SDL_RenderFillRect(renderer, &currentSprite);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &currentSprite);
        }

        // بعد از رسم مشخصات اسپرایت (حدود خط ۱۶۰۰)
        if (active && active->costumes.size() > 0)
        {
            int btnSize = 30;
            int spacing = 10;
            int totalWidth = (btnSize * 2) + spacing + 40; // 40 برای نمایش شماره

            int costumeBtnX = spritePanel.x + stageWidth - totalWidth - 10; // 10 فاصله از لبه راست
            int costumeBtnY = spritePanel.y + 25; // هم‌ردیف با اسم

            // دکمه قبلی
            SDL_Rect prevCostumeRect = {costumeBtnX, costumeBtnY, btnSize, btnSize};
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
            SDL_RenderFillRect(renderer, &prevCostumeRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &prevCostumeRect);
            renderText(renderer, "<", prevCostumeRect.x + 10, prevCostumeRect.y + 8, black);

            // نمایش شماره costume
            char costumeStr[20];
            sprintf(costumeStr, "%d/%d", active->currentCostume + 1, (int)active->costumes.size());
            renderText(renderer, costumeStr, prevCostumeRect.x + btnSize + 5, prevCostumeRect.y + 8, black);

            // دکمه بعدی
            SDL_Rect nextCostumeRect = {prevCostumeRect.x + btnSize + 40, costumeBtnY, btnSize, btnSize};
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
            SDL_RenderFillRect(renderer, &nextCostumeRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &nextCostumeRect);
            renderText(renderer, ">", nextCostumeRect.x + 10, nextCostumeRect.y + 8, black);
        }

        int textX = spritePanel.x + 70;
        int textY = spritePanel.y + 25;
        int lineHeight = 20;
        SDL_Color blue = {0, 0, 255, 255};
        SDL_Color red = {255, 0, 0, 255};

        if (game.editingMode && game.editingField == 0)
        {
            string displayText = "Name: " + game.editingBuffer + "_";
            renderText(renderer, displayText.c_str(), textX, textY, blue);
        }
        else
        {
            string nameText = "Name: " + active->name;
            renderText(renderer, nameText.c_str(), textX, textY, black);
        }

        char buffer[100];
        if (game.editingMode && game.editingField == 1)
        {
            sprintf(buffer, "X: %s_", game.editingBuffer.c_str());
            renderText(renderer, buffer, textX, textY + lineHeight, blue);
        }
        else
        {
            sprintf(buffer, "X: %.1f", active->x);
            renderText(renderer, buffer, textX, textY + lineHeight, black);
        }

        if (game.editingMode && game.editingField == 2)
        {
            sprintf(buffer, "Y: %s_", game.editingBuffer.c_str());
            renderText(renderer, buffer, textX + 120, textY + lineHeight, blue);
        }
        else
        {
            sprintf(buffer, "Y: %.1f", active->y);
            renderText(renderer, buffer, textX + 120, textY + lineHeight, black);
        }

        if (game.editingMode && game.editingField == 3)
        {
            sprintf(buffer, "Size: %s_", game.editingBuffer.c_str());
            renderText(renderer, buffer, textX, textY + lineHeight * 2, blue);
        }
        else
        {
            sprintf(buffer, "Size: %d", active->w);
            renderText(renderer, buffer, textX, textY + lineHeight * 2, black);
        }

        if (game.editingMode && game.editingField == 4)
        {
            sprintf(buffer, "Dir: %s_", game.editingBuffer.c_str());
            renderText(renderer, buffer, textX, textY + lineHeight * 3, blue);
        }
        else
        {
            sprintf(buffer, "Dir: %.1f", active->direction);
            renderText(renderer, buffer, textX, textY + lineHeight * 3, black);
        }

        if (game.editingMode && game.editingField == 5)
        {
            sprintf(buffer, "Volume: %s_ %%", game.editingBuffer.c_str());
            renderText(renderer, buffer, textX, textY + lineHeight * 5, blue);
        }
        else
        {
            sprintf(buffer, "Volume: %d %%", game.volume);
            renderText(renderer, buffer, textX, textY + lineHeight * 5, black);
        }

        string visibleText = active->visible ? "Visible: Yes" : "Visible: No";
        renderText(renderer, visibleText.c_str(), textX, textY + lineHeight * 4, active->visible ? black : red);
    }
    else
    {
        renderText(renderer, "No sprite", spritePanel.x + 10, spritePanel.y + 25, black);
    }

    // خطوط جداکننده
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawLine(renderer, categoriesPanelWidth + 15, 70, categoriesPanelWidth + 15, game.screenHeight - 80);
    SDL_RenderDrawLine(renderer, examplesPanelX + examplesPanelWidth + 15, 70,
                       examplesPanelX + examplesPanelWidth + 15, game.screenHeight - 80);
    SDL_RenderDrawLine(renderer, codeAreaX + codeAreaWidth + 5, 70,
                       codeAreaX + codeAreaWidth + 5, game.screenHeight - 80);

    // ========== دکمه‌های پایین صفحه ==========
    int buttonY = game.screenHeight - 80;
    int buttonWidth = 100;
    int buttonHeight = 40;
    int buttonSpacing = 10;
    int startX = (game.screenWidth - (6 * (buttonWidth + buttonSpacing))) / 2;

    // دکمه Run
    SDL_Rect runRect = {game.runButton.x, game.runButton.y, game.runButton.w, game.runButton.h};
    if (game.runButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &runRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &runRect);
    renderText(renderer, "Run", runRect.x+30, runRect.y+12, white);

    // دکمه Pause
    SDL_Rect pauseRect = {game.pauseButton.x, game.pauseButton.y, game.pauseButton.w, game.pauseButton.h};
    if (game.pauseButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &pauseRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &pauseRect);
    renderText(renderer, "Pause", pauseRect.x+25, pauseRect.y+12, white);

    // دکمه Step
    SDL_Rect stepRect = {game.stepButton.x, game.stepButton.y, game.stepButton.w, game.stepButton.h};
    if (game.stepButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 0, 100, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
    SDL_RenderFillRect(renderer, &stepRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &stepRect);
    renderText(renderer, "Step", stepRect.x+30, stepRect.y+12, white);

    // دکمه Reset
    SDL_Rect resetRect = {game.resetButton.x, game.resetButton.y, game.resetButton.w, game.resetButton.h};
    if (game.resetButton.isPressed)
        boxRGBA(renderer, resetRect.x, resetRect.y, resetRect.x + resetRect.w, resetRect.y + resetRect.h, 100, 50, 10, 255);
    else
        boxRGBA(renderer, resetRect.x, resetRect.y, resetRect.x + resetRect.w, resetRect.y + resetRect.h, 139, 69, 19, 255);
    rectangleRGBA(renderer, resetRect.x, resetRect.y, resetRect.x + resetRect.w, resetRect.y + resetRect.h, 255, 255, 255, 255);
    renderText(renderer, "Reset", resetRect.x+25, resetRect.y+12, white);

    // دکمه Save
    SDL_Rect saveRect = {game.saveButton.x, game.saveButton.y, game.saveButton.w, game.saveButton.h};
    if (game.saveButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 100, 200, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &saveRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &saveRect);
    renderText(renderer, "Save", saveRect.x+30, saveRect.y+12, white);

    // دکمه Load
    SDL_Rect loadRect = {game.loadButton.x, game.loadButton.y, game.loadButton.w, game.loadButton.h};
    if (game.loadButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 150, 0, 150, 255);
    else
        SDL_SetRenderDrawColor(renderer, 200, 0, 200, 255);
    SDL_RenderFillRect(renderer, &loadRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &loadRect);
    renderText(renderer, "Load", loadRect.x+30, loadRect.y+12, white);




    // دکمه‌های مدیریت اسپرایت
    int spriteBtnStartX = game.screenWidth - 350;

    SDL_Rect addRect = {spriteBtnStartX, buttonY, 60, buttonHeight};
    if (game.addSpriteBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 150, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
    SDL_RenderFillRect(renderer, &addRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &addRect);
    renderText(renderer, "+", addRect.x+25, addRect.y+12, white);

    SDL_Rect delRect = {spriteBtnStartX + 70, buttonY, 60, buttonHeight};
    if (game.deleteSpriteBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 150, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderFillRect(renderer, &delRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &delRect);
    renderText(renderer, "-", delRect.x+25, delRect.y+12, white);

    SDL_Rect prevRect = {spriteBtnStartX + 140, buttonY, 40, buttonHeight};
    if (game.prevSpriteBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    else
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &prevRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &prevRect);
    renderText(renderer, "<", prevRect.x+15, prevRect.y+12, white);

    SDL_Rect nextRect = {spriteBtnStartX + 190, buttonY, 40, buttonHeight};
    if (game.nextSpriteBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    else
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &nextRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &nextRect);
    renderText(renderer, ">", nextRect.x+15, nextRect.y+12, white);

    char spriteCount[50];
    sprintf(spriteCount, "%d/%d", game.activeSpriteIndex + 1, game.sprites.size());
    renderText(renderer, spriteCount, nextRect.x + 50, nextRect.y + 12, black);

    // دکمه‌های پس‌زمینه
    int backdropStartX = spriteBtnStartX - 220;

    SDL_Rect prevBgRect = {game.prevBackdropBtn.x, game.prevBackdropBtn.y,
                           game.prevBackdropBtn.w, game.prevBackdropBtn.h};
    if (game.prevBackdropBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    else
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &prevBgRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &prevBgRect);
    renderText(renderer, "<", prevBgRect.x+10, prevBgRect.y+12, white);

    SDL_Rect nextBgRect = {game.nextBackdropBtn.x, game.nextBackdropBtn.y,
                           game.nextBackdropBtn.w, game.nextBackdropBtn.h};
    if (game.nextBackdropBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    else
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &nextBgRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &nextBgRect);
    renderText(renderer, ">", nextBgRect.x+10, nextBgRect.y+12, white);

    SDL_Rect uploadRect = {game.uploadBackdropBtn.x, game.uploadBackdropBtn.y,
                           game.uploadBackdropBtn.w+40, game.uploadBackdropBtn.h};
    if (game.uploadBackdropBtn.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 100, 200, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    SDL_RenderFillRect(renderer, &uploadRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &uploadRect);
    renderText(renderer, "Upload Background", uploadRect.x+6, uploadRect.y+12, white);

    // ===== اضافه کردن اسم تصویر فعلی =====
    if (game.backdrops.size() > 0 && game.currentBackdrop >= 0)
    {
        string backdropName = game.backdrops[game.currentBackdrop].name;
        int nameX = uploadRect.x + uploadRect.w + 10;
        int nameY = uploadRect.y + 12;
        renderText(renderer, backdropName.c_str(), nameX, nameY, black);
    }

    // ===== نمایش متغیرها در زمان اجرا =====
    if (game.showVariables && game.variables.size() > 0)
    {
        int varPanelX = game.screenWidth - 220;
        int varPanelY = 70;
        int varPanelW = 200;
        int varPanelH = 150;

        SDL_Rect varPanel = {varPanelX, varPanelY, varPanelW, varPanelH};
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 200);
        SDL_RenderFillRect(renderer, &varPanel);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &varPanel);

        renderText(renderer, "Variables", varPanelX + 10, varPanelY + 5, black);

        int yOffset = 25;
        for (const auto& var : game.variables)
        {
            if (yOffset > varPanelH - 20) break;

            string display;
            if (var.second.type == VALUE_NUMBER)
            {
                double val = var.second.asNumber();
                // اگر عدد صحیح است
                if (val == (int)val)
                    display = var.first + " = " + to_string((int)val);
                else
                    display = var.first + " = " + to_string(val);
            }
            else
            {
                display = var.first + " = " + var.second.asString();
            }
            renderText(renderer, display.c_str(), varPanelX + 10, varPanelY + yOffset, black);
            yOffset += 20;
        }
    }

    // ===== دکمه Toggle Variables =====
    int toggleX = game.loadButton.x + game.loadButton.w + 20;
    int toggleY = game.screenHeight - 80;
    SDL_Rect toggleRect = {toggleX, toggleY, 60, 40};

    if (game.showVariables)
        boxRGBA(renderer, toggleX, toggleY, toggleX + 60, toggleY + 40, 100, 200, 100, 255); // سبز
    else
        boxRGBA(renderer, toggleX, toggleY, toggleX + 60, toggleY + 40, 200, 100, 100, 255); // قرمز

    rectangleRGBA(renderer, toggleX, toggleY, toggleX + 60, toggleY + 40, 255, 255, 255, 255);
    renderText(renderer, "Vars", toggleX + 15, toggleY + 12, white);
}