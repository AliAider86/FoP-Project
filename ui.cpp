#include "ui.h"
#include "engine.h"
#include <SDL2/SDL2_gfx.h>
#include <string>
#include <iostream>
#include <cmath>

using namespace std;

// تابع renderText در main.cpp تعریف شده

void handleEvents(bool &running, GameState& game)
{
    // reset pressedThisFrame
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        game.pressedThisFrame[i] = 0;

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

            if (e.key.keysym.scancode == SDL_SCANCODE_P)
            {
                game.player.penDown = true;
                game.player.lastPenX = game.player.x + game.player.w / 2;
                game.player.lastPenY = game.player.y + game.player.h / 2;
                game.player.penMoved = true;
            }
            if (e.key.keysym.scancode == SDL_SCANCODE_O)
            {
                game.player.penDown = false;
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
            if (game.isDragging)
            {
                // محاسبه مختصات استیج
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

                // موقعیت جدید در استیج
                double newScreenX = e.motion.x - game.dragOffsetX;
                double newScreenY = e.motion.y - game.dragOffsetY;

                // محدود کردن به استیج
                if (newScreenX < stageX) newScreenX = stageX;
                if (newScreenY < stageY) newScreenY = stageY;
                if (newScreenX + (game.player.w * stageWidth / game.screenWidth) > stageX + stageWidth)
                    newScreenX = stageX + stageWidth - (game.player.w * stageWidth / game.screenWidth);
                if (newScreenY + (game.player.h * stageHeight / game.screenHeight) > stageY + stageHeight)
                    newScreenY = stageY + stageHeight - (game.player.h * stageHeight / game.screenHeight);

                // تبدیل به مختصات اصلی
                game.player.x = (newScreenX - stageX) * game.screenWidth / stageWidth;
                game.player.y = (newScreenY - stageY) * game.screenHeight / stageHeight;

                // محدود کردن به صفحه اصلی
                if (game.player.x < 0) game.player.x = 0;
                if (game.player.y < 0) game.player.y = 0;
                if (game.player.x + game.player.w > game.screenWidth)
                    game.player.x = game.screenWidth - game.player.w;
                if (game.player.y + game.player.h > game.screenHeight)
                    game.player.y = game.screenHeight - game.player.h;
            }
        }

// ===== ویرایش با صفحه‌کلید =====
        if (e.type == SDL_KEYDOWN)
        {
            // کلیدهای خاص: Backspace, Enter, Escape
            bool blockEditing = false;

            for (int i = 0; i < game.program.size(); i++)
            {
                if (game.program[i].editingMode)
                {
                    blockEditing = true;
                    Block& b = game.program[i];

                    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                    {
                        // پایان ویرایش
                        if (!b.editingBuffer.empty())
                        {
                            if (b.type == SAY_FOR || b.type == THINK_FOR)
                            {
                                if (b.editingField == 0)  // ویرایش متن
                                {
                                    b.parameters[0] = Value(b.editingBuffer);
                                }
                                else if (b.editingField == 1)  // ویرایش زمان
                                {
                                    double newVal = stod(b.editingBuffer);
                                    b.parameters[1] = Value(newVal);
                                }
                            }
                            else if (b.type == SAY || b.type == THINK)
                            {
                                b.parameters[0] = Value(b.editingBuffer);
                            }
                            else if (b.type == GOTO_XY && b.parameters.size() >= 2)
                            {
                                double newVal = stod(b.editingBuffer);
                                if (b.editingField < b.parameters.size())
                                {
                                    b.parameters[b.editingField] = Value(newVal);
                                }
                            }
                            else if ((b.type == MOVE_UP || b.type == MOVE_DOWN ||
                                      b.type == MOVE_LEFT || b.type == MOVE_RIGHT ||
                                      b.type == TURN_RIGHT || b.type == TURN_LEFT ||
                                      b.type == CHANGE_X || b.type == CHANGE_Y ||
                                      b.type == SET_X || b.type == SET_Y ||
                                      b.type == CHANGE_SIZE || b.type == WAIT) &&
                                     !b.parameters.empty())
                            {
                                double newVal = stod(b.editingBuffer);
                                b.parameters[0] = Value(newVal);
                            }
                            else if (b.type == SET_SIZE && !b.parameters.empty())
                            {
                                int newVal = stoi(b.editingBuffer);
                                if (newVal < 1) newVal = 1;
                                b.parameters[0] = Value((double)newVal);
                            }
                            else if (b.type == CHANGE_VOLUME || b.type == SET_VOLUME)
                            {
                                int newVal = stoi(b.editingBuffer);
                                if (newVal < 0) newVal = 0;
                                if (newVal > 100) newVal = 100;
                                b.parameters[0] = Value((double)newVal);
                            }
                        }
                        b.editingMode = false;
                        b.editingField = -1;
                        b.editingBuffer = "";
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

            // اگه ویرایش مربوط به بلوک نبود، برو سراغ ویرایش اسپرایت
            if (!blockEditing && game.editingMode)
            {
                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                {
                    // پایان ویرایش
                    if (game.editingField == 0)  // نام
                    {
                        if (!game.editingBuffer.empty())
                            game.player.name = game.editingBuffer;
                    }
                    else if (game.editingField == 1)  // x
                    {
                        game.player.x = stod(game.editingBuffer);
                    }
                    else if (game.editingField == 2)  // y
                    {
                        game.player.y = stod(game.editingBuffer);
                    }
                    else if (game.editingField == 3)  // size
                    {
                        int newSize = stoi(game.editingBuffer);
                        if (newSize < 5) newSize = 5;
                        game.player.w = newSize;
                        game.player.h = newSize;
                    }
                    else if (game.editingField == 4)  // direction
                    {
                        game.player.direction = stod(game.editingBuffer);
                    }
                    else if (game.editingField == 5)  // volume
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

// ===== دریافت متن از کاربر =====
        if (e.type == SDL_TEXTINPUT && !game.isDragging)
        {

            // بررسی ویرایش بلوک
            bool blockEditing = false;

            for (int i = 0; i < game.program.size(); i++)
            {
                if (game.program[i].editingMode)
                {
                    blockEditing = true;
                    Block& b = game.program[i];

                    // تشخیص نوع فیلد (عددی یا متنی)
                    bool isTextField = false;

                    if ((b.type == SAY_FOR || b.type == THINK_FOR) && b.editingField == 0)
                        isTextField = true;
                    else if ((b.type == SAY || b.type == THINK) && b.editingField == 0)
                        isTextField = true;

                    if (isTextField)
                    {
                        // فیلد متنی - همه کاراکترها
                        b.editingBuffer += e.text.text;
                    }
                    else
                    {
                        // فیلد عددی - فقط اعداد، نقطه و منفی
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
                    break;
                }
            }

            // ویرایش اسپرایت
            if (!blockEditing && game.editingMode)
            {
                if (game.editingField == 0) // نام
                {
                    game.editingBuffer += e.text.text;
                }
                else // فیلدهای عددی
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

            // ===== تشخیص کلیک روی اسپرایت برای Drag =====
            // تبدیل مختصات اسپرایت به مختصات داخل استیج
            double playerScreenX = stageX + (game.player.x * stageWidth / game.screenWidth);
            double playerScreenY = stageY + (game.player.y * stageHeight / game.screenHeight);
            double playerScreenW = game.player.w * stageWidth / game.screenWidth;
            double playerScreenH = game.player.h * stageHeight / game.screenHeight;

            if (mx >= playerScreenX && mx <= playerScreenX + playerScreenW &&
                my >= playerScreenY && my <= playerScreenY + playerScreenH)
            {
                game.spriteClicked = true;
                game.isDragging = true;
                game.dragOffsetX = mx - playerScreenX;
                game.dragOffsetY = my - playerScreenY;
            }

            // ریست وضعیت دکمه‌ها
            game.runButton.isPressed = false;
            game.pauseButton.isPressed = false;
            game.stepButton.isPressed = false;
            game.resetButton.isPressed = false;
            game.saveButton.isPressed = false;
            game.loadButton.isPressed = false;

            game.moveCategoryBtn.isPressed = false;
            game.looksCategoryBtn.isPressed = false;
            game.soundCategoryBtn.isPressed = false;
            game.eventsCategoryBtn.isPressed = false;
            game.controlCategoryBtn.isPressed = false;
            game.sensingCategoryBtn.isPressed = false;
            game.operatorsCategoryBtn.isPressed = false;
            game.variablesCategoryBtn.isPressed = false;

            // ===== دکمه‌های پایین صفحه =====
            int buttonY = game.screenHeight - 80;
            int buttonWidth = 100;
            int buttonHeight = 40;
            int buttonSpacing = 10;
            int startX = (game.screenWidth - (6 * (buttonWidth + buttonSpacing))) / 2;

            // مقداردهی دستی دکمه‌ها (بدون استفاده از constructor)
            game.runButton.x = startX;
            game.runButton.y = buttonY;
            game.runButton.w = buttonWidth;
            game.runButton.h = buttonHeight;
            game.runButton.isPressed = 0;

            game.pauseButton.x = startX + buttonWidth + buttonSpacing;
            game.pauseButton.y = buttonY;
            game.pauseButton.w = buttonWidth;
            game.pauseButton.h = buttonHeight;
            game.pauseButton.isPressed = 0;

            game.stepButton.x = startX + 2*(buttonWidth + buttonSpacing);
            game.stepButton.y = buttonY;
            game.stepButton.w = buttonWidth;
            game.stepButton.h = buttonHeight;
            game.stepButton.isPressed = 0;

            game.resetButton.x = startX + 3*(buttonWidth + buttonSpacing);
            game.resetButton.y = buttonY;
            game.resetButton.w = buttonWidth;
            game.resetButton.h = buttonHeight;
            game.resetButton.isPressed = 0;

            game.saveButton.x = startX + 4*(buttonWidth + buttonSpacing);
            game.saveButton.y = buttonY;
            game.saveButton.w = buttonWidth;
            game.saveButton.h = buttonHeight;
            game.saveButton.isPressed = 0;

            game.loadButton.x = startX + 5*(buttonWidth + buttonSpacing);
            game.loadButton.y = buttonY;
            game.loadButton.w = buttonWidth;
            game.loadButton.h = buttonHeight;
            game.loadButton.isPressed = 0;

            if (mx >= game.runButton.x && mx <= game.runButton.x + game.runButton.w &&
                my >= game.runButton.y && my <= game.runButton.y + game.runButton.h)
            {
                game.runButton.isPressed = true;
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

            // ===== دکمه Pause =====
            if (mx >= game.pauseButton.x && mx <= game.pauseButton.x + game.pauseButton.w &&
                my >= game.pauseButton.y && my <= game.pauseButton.y + game.pauseButton.h)
            {
                game.pauseButton.isPressed = true;
                game.isRunningCode = false;
            }

            // ===== دکمه Step =====
            if (mx >= game.stepButton.x && mx <= game.stepButton.x + game.stepButton.w &&
                my >= game.stepButton.y && my <= game.stepButton.y + game.stepButton.h)
            {
                game.stepButton.isPressed = true;
                game.stepMode = true;
                game.isRunningCode = true;
            }

            // ===== دکمه Reset =====
            if (mx >= game.resetButton.x && mx <= game.resetButton.x + game.resetButton.w &&
                my >= game.resetButton.y && my <= game.resetButton.y + game.resetButton.h)
            {
                game.resetButton.isPressed = true;
                game.isRunningCode = false;
                game.currentBlockIndex = 0;
                game.isExecutingBlock = false;
                game.remainingMove = 0;
                game.isWaiting = false;
                game.isShowingMessage = false;
                game.repeatCountStack.clear();
                game.repeatStartStack.clear();
                game.player.x = game.screenWidth / 2 - game.player.w / 2;
                game.player.y = game.screenHeight / 2 - game.player.h / 2;
                game.player.isThinking = false;
                game.penX1.clear();
                game.penY1.clear();
                game.penX2.clear();
                game.penY2.clear();
                game.penR_.clear();
                game.penG_.clear();
                game.penB_.clear();
                game.penSize_.clear();
            }

            // ===== دکمه Save =====
            if (mx >= game.saveButton.x && mx <= game.saveButton.x + game.saveButton.w &&
                my >= game.saveButton.y && my <= game.saveButton.y + game.saveButton.h)
            {
                game.saveButton.isPressed = true;
                saveProject(game, "project.txt");
            }

            // ===== دکمه Load =====
            if (mx >= game.loadButton.x && mx <= game.loadButton.x + game.loadButton.w &&
                my >= game.loadButton.y && my <= game.loadButton.y + game.loadButton.h)
            {
                game.loadButton.isPressed = true;
                loadProject(game, "project.txt");
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

            // ===== تشخیص کلیک روی بلوک‌های مثال =====
            int exampleBlockStartY = 110;
            int blockCount = 0;

            if (game.currentCategory == 0) blockCount = 10;
            else if (game.currentCategory == 1) blockCount = 8;
            else if (game.currentCategory == 2) blockCount = 5;
            else if (game.currentCategory == 3) blockCount = 7;
            else if (game.currentCategory == 4) blockCount = 8;
            else if (game.currentCategory == 5) blockCount = 9;
            else if (game.currentCategory == 6) blockCount = 8;
            else if (game.currentCategory == 7) blockCount = 6;

            for (int i = 0; i < blockCount; i++)
            {
                int blockY = exampleBlockStartY + i * 45;
                SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

                if (mx >= blockRect.x && mx <= blockRect.x + blockRect.w &&
                    my >= blockRect.y && my <= blockRect.y + blockRect.h)
                {
                    // ساختن بلوک جدید
                    Block newBlock;
                    string blockName;

                    if (game.currentCategory == 0)  // حرکت
                    {
                        if (i == 0) { newBlock.type = MOVE_UP; blockName = "move 10 steps"; newBlock.parameters.push_back(Value(10.0)); }
                        else if (i == 1) { newBlock.type = TURN_RIGHT; blockName = "turn 15 degrees"; newBlock.parameters.push_back(Value(15.0)); }
                        else if (i == 2) { newBlock.type = TURN_LEFT; blockName = "turn -15 degrees"; newBlock.parameters.push_back(Value(15.0)); }
                        else if (i == 3) { newBlock.type = GOTO_XY; blockName = "go to x: y:"; newBlock.parameters.push_back(Value(100.0)); newBlock.parameters.push_back(Value(100.0)); }
                        else if (i == 4) { newBlock.type = GOTO_MOUSE; blockName = "go to mouse-pointer"; }
                        else if (i == 5) { newBlock.type = GOTO_RANDOM; blockName = "go to random position"; newBlock.parameters.push_back(Value(1.0)); }
                        else if (i == 6) { newBlock.type = CHANGE_X; blockName = "change x by 10"; newBlock.parameters.push_back(Value(10.0)); }
                        else if (i == 7) { newBlock.type = SET_X; blockName = "set x to 0"; newBlock.parameters.push_back(Value(0.0)); }
                        else if (i == 8) { newBlock.type = CHANGE_Y; blockName = "change y by 10"; newBlock.parameters.push_back(Value(10.0)); }
                        else if (i == 9) { newBlock.type = SET_Y; blockName = "set y to 0"; newBlock.parameters.push_back(Value(0.0)); }
                    }
                    else if (game.currentCategory == 1)  // ظاهر
                    {
                        if (i == 0) { newBlock.type = SAY_FOR; blockName = "say Hello for 2 secs"; newBlock.parameters.push_back(Value(string("Hello!"))); newBlock.parameters.push_back(Value(2.0)); }
                        else if (i == 1) { newBlock.type = SAY; blockName = "say Hello"; newBlock.parameters.push_back(Value(string("Hello!"))); }
                        else if (i == 2) { newBlock.type = THINK_FOR; blockName = "think Hmm... for 2 secs"; newBlock.parameters.push_back(Value(string("Hmm..."))); newBlock.parameters.push_back(Value(2.0)); }
                        else if (i == 3) { newBlock.type = THINK; blockName = "think Hmm..."; newBlock.parameters.push_back(Value(string("Hmm..."))); }
                        else if (i == 4) { newBlock.type = SHOW; blockName = "show"; }
                        else if (i == 5) { newBlock.type = HIDE; blockName = "hide"; }
                        else if (i == 6) { newBlock.type = CHANGE_SIZE; blockName = "change size by 10"; newBlock.parameters.push_back(Value(10.0)); }
                        else if (i == 7) { newBlock.type = SET_SIZE; blockName = "set size to 100 %"; newBlock.parameters.push_back(Value(100.0)); }
                    }
                    else if (game.currentCategory == 2)  // صدا
                    {
                        if (i == 0) { newBlock.type = PLAY_SOUND; blockName = "play sound Meow"; newBlock.parameters.push_back(Value(string("Meow"))); }
                        else if (i == 1) { newBlock.type = PLAY_SOUND_UNTIL_DONE; blockName = "play sound Meow until done"; newBlock.parameters.push_back(Value(string("Meow"))); }
                        else if (i == 2) { newBlock.type = STOP_ALL_SOUNDS; blockName = "stop all sounds"; }
                        else if (i == 3) { newBlock.type = CHANGE_VOLUME; blockName = "change volume by 10"; newBlock.parameters.push_back(Value(10.0)); }
                        else if (i == 4) { newBlock.type = SET_VOLUME; blockName = "set volume to 100 %"; newBlock.parameters.push_back(Value(100.0)); }
                    }
                    else if (game.currentCategory == 3)  // رویدادها
                    {
                        if (i == 0) { newBlock.type = WHEN_GREEN_FLAG; blockName = "when flag clicked"; }
                        else if (i == 1)
                        {
                            newBlock.type = WHEN_KEY_PRESSED;
                            newBlock.keyCode = SDL_SCANCODE_SPACE;
                            blockName = "when key pressed";
                        }
                        else if (i == 2) { newBlock.type = WHEN_SPRITE_CLICKED; blockName = "when sprite clicked"; }
                        else { newBlock.type = WAIT; blockName = "event block"; newBlock.parameters.push_back(Value(1.0)); }
                    }
                    else if (game.currentCategory == 4)  // کنترل
                    {
                        if (i == 0) { newBlock.type = WAIT; blockName = "wait 1 seconds"; newBlock.parameters.push_back(Value(1.0)); }
                        else if (i == 1)
                        {
                            newBlock.type = REPEAT;
                            newBlock.repeatCount = 10;
                            blockName = "repeat 10";
                        }
                        else if (i == 2) { newBlock.type = FOREVER; blockName = "forever"; }
                        else { newBlock.type = WAIT; blockName = "control block"; newBlock.parameters.push_back(Value(1.0)); }
                    }
                    else if (game.currentCategory == 5)  // حسگر
                    {
                        newBlock.type = WAIT;
                        blockName = "sensing block";
                        newBlock.parameters.push_back(Value(1.0));
                    }
                    else if (game.currentCategory == 6)  // عملگرها
                    {
                        if (i == 0) { newBlock.type = OP_ADD; blockName = "0 + 0"; newBlock.parameters.push_back(Value(10.0)); newBlock.parameters.push_back(Value(5.0)); }
                        else if (i == 1) { newBlock.type = OP_SUBTRACT; blockName = "0 - 0"; newBlock.parameters.push_back(Value(10.0)); newBlock.parameters.push_back(Value(5.0)); }
                        else if (i == 2) { newBlock.type = OP_MULTIPLY; blockName = "0 * 0"; newBlock.parameters.push_back(Value(10.0)); newBlock.parameters.push_back(Value(5.0)); }
                        else if (i == 3) { newBlock.type = OP_DIVIDE; blockName = "0 / 0"; newBlock.parameters.push_back(Value(10.0)); newBlock.parameters.push_back(Value(5.0)); }
                        else { newBlock.type = WAIT; blockName = "operator block"; newBlock.parameters.push_back(Value(1.0)); }
                    }
                    else if (game.currentCategory == 7)  // متغیرها
                    {
                        if (i == 0 || i == 1)
                        {
                            newBlock.type = SET_VARIABLE;
                            newBlock.variableName = "my variable";
                            blockName = "set variable to 0";
                            newBlock.parameters.push_back(Value(0.0));
                        }
                        else { newBlock.type = WAIT; blockName = "variable block"; newBlock.parameters.push_back(Value(1.0)); }
                    }

                    // ذخیره نام بلوک برای نمایش
                    newBlock.eventName = blockName;

                    game.program.push_back(newBlock);
                }
            }

            // ===== تشخیص کلیک روی بلوک‌های Code Area (برای ویرایش) =====
            for (int i = 0; i < game.program.size() && i < 20; i++)
            {
                int blockY = 100 + i * 45;
                SDL_Rect blockRect = {codeAreaX + 10, blockY, codeAreaWidth - 20, 35};

                // اگه روی بلوک کلیک شد
                if (mx >= blockRect.x && mx <= blockRect.x + blockRect.w &&
                    my >= blockRect.y && my <= blockRect.y + blockRect.h)
                {
                    Block& b = game.program[i];

                    // غیرفعال کردن ویرایش قبلی
                    for (int j = 0; j < game.program.size(); j++)
                    {
                        if (j != i)
                        {
                            game.program[j].editingMode = false;
                            game.program[j].editingField = -1;
                            game.program[j].editingBuffer = "";
                        }
                    }

                    // مختصات کلیک رو نسبت به بلوک حساب کن
                    int localX = mx - blockRect.x;

                    // تشخیص نوع بلوک و فیلد قابل ویرایش
                    if (b.type == GOTO_XY && b.parameters.size() >= 2)
                    {
                        if (localX < 70)
                        {
                            b.editingMode = true;
                            b.editingField = 0;
                            char buffer[50];
                            sprintf(buffer, "%.1f", b.parameters[0].asNumber());
                            b.editingBuffer = buffer;
                        }
                        else if (localX > 100)
                        {
                            b.editingMode = true;
                            b.editingField = 1;
                            char buffer[50];
                            sprintf(buffer, "%.1f", b.parameters[1].asNumber());
                            b.editingBuffer = buffer;
                        }
                    }
                    else if ((b.type == SAY_FOR || b.type == THINK_FOR) && b.parameters.size() >= 2)
                    {
                        if (localX < 100)
                        {
                            b.editingMode = true;
                            b.editingField = 0;
                            b.editingBuffer = b.parameters[0].asString();
                        }
                        else if (localX > 120)
                        {
                            b.editingMode = true;
                            b.editingField = 1;
                            char buffer[50];
                            sprintf(buffer, "%.1f", b.parameters[1].asNumber());
                            b.editingBuffer = buffer;
                        }
                    }
                    else if ((b.type == SAY || b.type == THINK) && !b.parameters.empty())
                    {
                        b.editingMode = true;
                        b.editingField = 0;
                        b.editingBuffer = b.parameters[0].asString();
                    }
                    else if ((b.type == MOVE_UP || b.type == MOVE_DOWN ||
                              b.type == MOVE_LEFT || b.type == MOVE_RIGHT ||
                              b.type == TURN_RIGHT || b.type == TURN_LEFT ||
                              b.type == CHANGE_X || b.type == CHANGE_Y ||
                              b.type == SET_X || b.type == SET_Y ||
                              b.type == CHANGE_SIZE || b.type == WAIT) &&
                             !b.parameters.empty())
                    {
                        b.editingMode = true;
                        b.editingField = 0;
                        char buffer[50];
                        sprintf(buffer, "%.1f", b.parameters[0].asNumber());
                        b.editingBuffer = buffer;
                    }
                    else if (b.type == SET_SIZE && !b.parameters.empty())
                    {
                        b.editingMode = true;
                        b.editingField = 0;
                        char buffer[50];
                        sprintf(buffer, "%d", (int)b.parameters[0].asNumber());
                        b.editingBuffer = buffer;
                    }
                    else if (b.type == CHANGE_VOLUME || b.type == SET_VOLUME)
                    {
                        b.editingMode = true;
                        b.editingField = 0;
                        char buffer[50];
                        sprintf(buffer, "%d", (int)b.parameters[0].asNumber());
                        b.editingBuffer = buffer;
                    }
                }
            }

            // ===== تشخیص کلیک روی مشخصات اسپرایت =====
            SDL_Rect stage = {stageX, stageY, stageWidth, stageHeight};

            int spritePanelY = stage.y + stage.h + 10;
            SDL_Rect spritePanel = {stagePanelX + 5, spritePanelY, stageWidth, 150};

            int textX = spritePanel.x + 70;
            int textY = spritePanel.y + 25;
            int lineHeight = 20;

            // کلیک روی اسم
            SDL_Rect nameRect = {textX, textY, 150, 18};
            if (mx >= nameRect.x && mx <= nameRect.x + nameRect.w &&
                my >= nameRect.y && my <= nameRect.y + nameRect.h)
            {
                game.editingMode = true;
                game.editingField = 0;
                game.editingBuffer = game.player.name;
            }

            // کلیک روی X
            SDL_Rect xRect = {textX, textY + lineHeight, 100, 18};
            if (mx >= xRect.x && mx <= xRect.x + xRect.w &&
                my >= xRect.y && my <= xRect.y + xRect.h)
            {
                game.editingMode = true;
                game.editingField = 1;
                char buffer[50];
                sprintf(buffer, "%.1f", game.player.x);
                game.editingBuffer = buffer;
            }

            // کلیک روی Y
            SDL_Rect yRect = {textX + 120, textY + lineHeight, 100, 18};
            if (mx >= yRect.x && mx <= yRect.x + yRect.w &&
                my >= yRect.y && my <= yRect.y + yRect.h)
            {
                game.editingMode = true;
                game.editingField = 2;
                char buffer[50];
                sprintf(buffer, "%.1f", game.player.y);
                game.editingBuffer = buffer;
            }

            // کلیک روی Size
            SDL_Rect sizeRect = {textX, textY + lineHeight * 2, 100, 18};
            if (mx >= sizeRect.x && mx <= sizeRect.x + sizeRect.w &&
                my >= sizeRect.y && my <= sizeRect.y + sizeRect.h)
            {
                game.editingMode = true;
                game.editingField = 3;
                char buffer[50];
                sprintf(buffer, "%d", game.player.w);
                game.editingBuffer = buffer;
            }

            // کلیک روی Direction
            SDL_Rect dirRect = {textX, textY + lineHeight * 3, 100, 18};
            if (mx >= dirRect.x && mx <= dirRect.x + dirRect.w &&
                my >= dirRect.y && my <= dirRect.y + dirRect.h)
            {
                game.editingMode = true;
                game.editingField = 4;
                char buffer[50];
                sprintf(buffer, "%.1f", game.player.direction);
                game.editingBuffer = buffer;
            }

            // کلیک روی Visible toggle
            SDL_Rect visibleRect = {textX, textY + lineHeight * 4, 80, 18};
            if (mx >= visibleRect.x && mx <= visibleRect.x + visibleRect.w &&
                my >= visibleRect.y && my <= visibleRect.y + visibleRect.h)
            {
                game.player.visible = !game.player.visible;
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
            }
        }

        // ===== تشخیص کلیک راست روی بلوک‌های Code Area =====
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
        {
            int mx = e.button.x;
            int my = e.button.y;

            int categoriesPanelWidth = 180;
            int examplesPanelX = categoriesPanelWidth + 30;
            int examplesPanelWidth = 250;
            int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
            int codeAreaWidth = 600;

            for (int i = 0; i < game.program.size() && i < 20; i++)
            {
                int blockY = 100 + i * 45;
                SDL_Rect blockRect = {codeAreaX + 10, blockY, codeAreaWidth - 20, 35};

                if (mx >= blockRect.x && mx <= blockRect.x + blockRect.w &&
                    my >= blockRect.y && my <= blockRect.y + blockRect.h)
                {
                    game.program.erase(game.program.begin() + i);
                    break;
                }
            }
        }

        if (e.type == SDL_MOUSEBUTTONUP)
        {
            game.mousePressed = false;
            game.spriteClicked = false;
            game.isDragging = false;

            game.runButton.isPressed = false;
            game.pauseButton.isPressed = false;
            game.stepButton.isPressed = false;
            game.resetButton.isPressed = false;
            game.saveButton.isPressed = false;
            game.loadButton.isPressed = false;
            game.moveCategoryBtn.isPressed = false;
            game.looksCategoryBtn.isPressed = false;
            game.soundCategoryBtn.isPressed = false;
            game.eventsCategoryBtn.isPressed = false;
            game.controlCategoryBtn.isPressed = false;
            game.sensingCategoryBtn.isPressed = false;
            game.operatorsCategoryBtn.isPressed = false;
            game.variablesCategoryBtn.isPressed = false;
        }
    }

    // حرکت با کلیدهای جهت‌دار
    if (game.isRunningCode)
    {
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        double moveSpeed = 3.0;
        bool moved = false;

        if (keys[SDL_SCANCODE_UP])
        {
            game.player.y -= moveSpeed;
            moved = true;
        }
        if (keys[SDL_SCANCODE_DOWN])
        {
            game.player.y += moveSpeed;
            moved = true;
        }
        if (keys[SDL_SCANCODE_LEFT])
        {
            game.player.x -= moveSpeed;
            moved = true;
        }
        if (keys[SDL_SCANCODE_RIGHT])
        {
            game.player.x += moveSpeed;
            moved = true;
        }

        if (game.player.x < 0) game.player.x = 0;
        if (game.player.y < 0) game.player.y = 0;
        if (game.player.x + game.player.w > game.screenWidth)
            game.player.x = game.screenWidth - game.player.w;
        if (game.player.y + game.player.h > game.screenHeight)
            game.player.y = game.screenHeight - game.player.h;

        if (game.player.penDown && moved)
        {
            double centerX = game.player.x + game.player.w / 2;
            double centerY = game.player.y + game.player.h / 2;

            if (centerX != game.player.lastPenX || centerY != game.player.lastPenY)
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
        else if (!game.player.penDown)
        {
            game.player.lastPenX = game.player.x + game.player.w / 2;
            game.player.lastPenY = game.player.y + game.player.h / 2;
        }
    }
}

void render(SDL_Renderer* renderer, GameState& game)
{
    // رسم خطوط قلم
    for (int i = 0; i < game.penX1.size(); i++)
    {
        for (int thickness = 0; thickness < game.penSize_[i]; thickness++)
        {
            int offset = thickness - game.penSize_[i] / 2;
            SDL_SetRenderDrawColor(renderer, game.penR_[i], game.penG_[i], game.penB_[i], 255);
            SDL_RenderDrawLine(renderer,
                               game.penX1[i], game.penY1[i] + offset,
                               game.penX2[i], game.penY2[i] + offset);
        }
    }

    // پس‌زمینه اصلی
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderClear(renderer);

    // نوار منوی بالا
    SDL_Rect menuBar = {0, 0, game.screenWidth, 60};
    SDL_SetRenderDrawColor(renderer, 70, 150, 70, 255);
    SDL_RenderFillRect(renderer, &menuBar);

    // لوگو
    filledCircleRGBA(renderer, 40, 30, 20, 255, 255, 255, 255);
    SDL_Color white = {255,255,255,255};
    renderText(renderer, "Scratch", 70, 20, white);

    // وضعیت اجرا
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

    // رنگ‌های دسته‌بندی
    Uint8 catColors[8][3] = {
            {70, 120, 255},   // حرکت
            {100, 150, 255},  // ظاهر
            {200, 50, 200},   // صدا
            {255, 200, 50},   // رویدادها
            {255, 140, 0},    // کنترل
            {0, 200, 200},    // حسگر
            {70, 200, 70},    // عملگرها
            {255, 100, 50}    // متغیرها
    };

    const char* catNames[] = {
            "Motion", "Looks", "Sound", "Events",
            "Control", "Sensing", "Operators", "Variables"
    };

    Button* catButtons[] = {
            &game.moveCategoryBtn, &game.looksCategoryBtn, &game.soundCategoryBtn,
            &game.eventsCategoryBtn, &game.controlCategoryBtn, &game.sensingCategoryBtn,
            &game.operatorsCategoryBtn, &game.variablesCategoryBtn
    };

    for (int i = 0; i < 8; i++)
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

    // ========== ۲. پنل بلوک‌های مثال ==========
    int examplesPanelX = categoriesPanelWidth + 30;
    int examplesPanelWidth = 250;
    SDL_Rect examplesPanel = {examplesPanelX, 70, examplesPanelWidth, game.screenHeight - 150};
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderFillRect(renderer, &examplesPanel);
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderDrawRect(renderer, &examplesPanel);

    renderText(renderer, "Blocks", examplesPanelX + 10, 75, black);

    int exampleBlockStartY = 110;
    int maxBlocks = (examplesPanel.h - 60) / 45;

    if (game.currentCategory == 0)  // حرکت
    {
        const char* moveBlocks[] = {"move 10 steps", "turn 15 degrees", "turn -15 degrees",
                                    "go to x: y:", "go to mouse-pointer", "go to random position",
                                    "change x by 10", "set x to 0", "change y by 10", "set y to 0"};
        int numBlocks = 10;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 70, 120, 255, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, moveBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 1)  // ظاهر
    {
        const char* looksBlocks[] = {"say Hello for 2 secs", "say Hello", "think Hmm... for 2 secs",
                                     "think Hmm...", "show", "hide", "change size by 10",
                                     "set size to 100 %"};
        int numBlocks = 8;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, looksBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 2)  // صدا
    {
        const char* soundBlocks[] = {"play sound Meow", "play sound Meow until done",
                                     "stop all sounds", "change volume by 10",
                                     "set volume to 100 %"};
        int numBlocks = 5;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 200, 50, 200, 255); // بنفش
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, soundBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 3)  // رویدادها
    {
        const char* eventBlocks[] = {"when flag clicked", "when key pressed", "when sprite clicked",
                                     "when backdrop switches", "when I receive msg1",
                                     "broadcast msg1", "broadcast and wait"};
        int numBlocks = 7;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, eventBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 4)  // کنترل
    {
        const char* controlBlocks[] = {"wait 1 seconds", "repeat 10", "forever",
                                       "if then", "if then else", "wait until",
                                       "repeat until", "stop all"};
        int numBlocks = 8;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, controlBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 5)  // حسگر
    {
        const char* sensingBlocks[] = {"touching mouse-pointer?", "touching color?",
                                       "distance to mouse", "ask and wait", "answer",
                                       "key pressed?", "mouse down?", "mouse x", "mouse y"};
        int numBlocks = 9;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 0, 200, 200, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, sensingBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 6)  // عملگرها
    {
        const char* operatorBlocks[] = {"0 + 0", "0 - 0", "0 * 0", "0 / 0",
                                        "pick random 1 to 10", "0 < 0", "0 = 0",
                                        "0 > 0", "and", "or", "not"};
        int numBlocks = 8;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 70, 200, 70, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, operatorBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }
    else if (game.currentCategory == 7)  // متغیرها
    {
        const char* varBlocks[] = {"set variable to 0", "change variable by 1",
                                   "show variable", "hide variable",
                                   "set my var to 0", "change my var by 1"};
        int numBlocks = 6;
        for (int i = 0; i < numBlocks && i < maxBlocks; i++)
        {
            int blockY = exampleBlockStartY + i * 45;
            SDL_Rect blockRect = {examplesPanelX + 10, blockY, examplesPanelWidth - 20, 35};

            SDL_SetRenderDrawColor(renderer, 255, 100, 50, 255);
            SDL_RenderFillRect(renderer, &blockRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &blockRect);

            renderText(renderer, varBlocks[i], blockRect.x + 5, blockRect.y + 10, black);
        }
    }

    // ========== ۳. فضای کدنویسی (Code Area) ==========
    int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
    int codeAreaWidth = 600;
    SDL_Rect codeArea = {codeAreaX, 70, codeAreaWidth, game.screenHeight - 150};

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &codeArea);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &codeArea);

    renderText(renderer, "Code Area", codeArea.x + 10, codeArea.y + 5, black);

    // نمایش بلوک‌های برنامه در فضای کدنویسی
    for (int i = 0; i < game.program.size() && i < 20; i++)
    {
        int blockY = codeArea.y + 30 + i * 45;
        SDL_Rect blockRect = {codeArea.x + 10, blockY, codeArea.w - 20, 35};

        BlockType type = game.program[i].type;

        // ===== رنگ‌بندی بلوک‌ها بر اساس دسته =====
        if (type >= MOVE_UP && type <= MOVE_RIGHT)
            SDL_SetRenderDrawColor(renderer, 70, 120, 255, 255); // آبی حرکتی
        else if (type >= TURN_RIGHT && type <= GOTO_MOUSE)
            SDL_SetRenderDrawColor(renderer, 70, 120, 255, 255); // آبی حرکتی
        else if (type >= REPEAT && type <= WAIT)
            SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); // نارنجی کنترلی
        else if (type >= OP_ADD && type <= OP_XOR)
            SDL_SetRenderDrawColor(renderer, 70, 200, 70, 255); // سبز عملگرها
        else if (type >= SAY && type <= THINK_FOR)
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255); // نیلی ظاهری
        else if (type >= SHOW && type <= SET_SIZE)
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255); // نیلی ظاهری
        else if (type >= WHEN_GREEN_FLAG && type <= WHEN_BROADCAST)
            SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255); // زرد رویدادها
        else if (type >= SET_VARIABLE && type <= HIDE_VARIABLE)
            SDL_SetRenderDrawColor(renderer, 255, 100, 50, 255); // نارنجی پررنگ متغیرها
        else if (type >= TOUCHING_MOUSE && type <= RESET_TIMER)
            SDL_SetRenderDrawColor(renderer, 0, 200, 200, 255); // فیروزه‌ای حسگر
        else if (type >= PEN_DOWN && type <= CHANGE_PEN_SIZE)
            SDL_SetRenderDrawColor(renderer, 50, 200, 150, 255); // سبز روشن قلم
        else if (type >= PLAY_SOUND && type <= SET_VOLUME)
            SDL_SetRenderDrawColor(renderer, 200, 50, 200, 255); // بنفش صدا
        else
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // خاکستری

        SDL_RenderFillRect(renderer, &blockRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &blockRect);

        // ===== نمایش متن بلوک =====
        string blockText;
        SDL_Color textColor = black;
        SDL_Color blue = {0, 0, 255, 255};

        // === اولویت با حالت ویرایش ===
        if (game.program[i].editingMode)
        {
            blockText = game.program[i].editingBuffer + "_";
            textColor = blue;
        }
        else
        {
            // تشخیص نوع بلوک و ساختن متن مناسب
            if (type == GOTO_XY && game.program[i].parameters.size() >= 2)
            {
                blockText = "go to x: " + to_string((int)game.program[i].parameters[0].asNumber()) +
                            " y: " + to_string((int)game.program[i].parameters[1].asNumber());
            }
            else if (type == GOTO_MOUSE)
            {
                blockText = "go to mouse-pointer";
            }
            else if (type == GOTO_RANDOM)
            {
                blockText = "go to random position";
            }
            else if (type == MOVE_UP || type == MOVE_DOWN || type == MOVE_LEFT || type == MOVE_RIGHT)
            {
                if (!game.program[i].parameters.empty())
                    blockText = "move " + to_string((int)game.program[i].parameters[0].asNumber()) + " steps";
                else
                    blockText = "move 10 steps";
            }
            else if (type == TURN_RIGHT || type == TURN_LEFT)
            {
                if (!game.program[i].parameters.empty())
                    blockText = "turn " + to_string((int)game.program[i].parameters[0].asNumber()) + " degrees";
                else
                    blockText = "turn 15 degrees";
            }
            else if (type == CHANGE_X || type == CHANGE_Y)
            {
                if (!game.program[i].parameters.empty())
                    blockText = (type == CHANGE_X ? "change x by " : "change y by ") +
                                to_string((int)game.program[i].parameters[0].asNumber());
            }
            else if (type == SET_X || type == SET_Y)
            {
                if (!game.program[i].parameters.empty())
                    blockText = (type == SET_X ? "set x to " : "set y to ") +
                                to_string((int)game.program[i].parameters[0].asNumber());
            }
            else if (type == WAIT && !game.program[i].parameters.empty())
            {
                char buffer[50];
                sprintf(buffer, "wait %.1f seconds", game.program[i].parameters[0].asNumber());
                blockText = buffer;
            }
            else if (type == REPEAT)
            {
                blockText = "repeat " + to_string(game.program[i].repeatCount);
            }
            else if (type == FOREVER)
            {
                blockText = "forever";
            }
            else if (type == WHEN_GREEN_FLAG)
            {
                blockText = "when flag clicked";
            }
            else if (type == WHEN_KEY_PRESSED)
            {
                blockText = "when key pressed";
            }
            else if (type == WHEN_SPRITE_CLICKED)
            {
                blockText = "when sprite clicked";
            }
            else if (type == SAY_FOR || type == THINK_FOR)
            {
                if (game.program[i].parameters.size() >= 2)
                {
                    blockText = (type == SAY_FOR ? "say " : "think ") +
                                game.program[i].parameters[0].asString() +
                                " for " + to_string((int)game.program[i].parameters[1].asNumber()) + " secs";
                }
            }
            else if (type == SAY || type == THINK)
            {
                if (!game.program[i].parameters.empty())
                    blockText = (type == SAY ? "say " : "think ") + game.program[i].parameters[0].asString();
            }
            else if (type == SHOW)
            {
                blockText = "show";
            }
            else if (type == HIDE)
            {
                blockText = "hide";
            }
            else if (type == CHANGE_SIZE)
            {
                if (!game.program[i].parameters.empty())
                    blockText = "change size by " + to_string((int)game.program[i].parameters[0].asNumber());
            }
            else if (type == SET_SIZE)
            {
                if (!game.program[i].parameters.empty())
                    blockText = "set size to " + to_string((int)game.program[i].parameters[0].asNumber()) + " %";
            }
            else if (type == PLAY_SOUND)
            {
                blockText = "play sound Meow";
            }
            else if (type == PLAY_SOUND_UNTIL_DONE)
            {
                blockText = "play sound Meow until done";
            }
            else if (type == STOP_ALL_SOUNDS)
            {
                blockText = "stop all sounds";
            }
            else if (type == CHANGE_VOLUME)
            {
                if (!game.program[i].parameters.empty())
                    blockText = "change volume by " + to_string((int)game.program[i].parameters[0].asNumber());
            }
            else if (type == SET_VOLUME)
            {
                if (!game.program[i].parameters.empty())
                    blockText = "set volume to " + to_string((int)game.program[i].parameters[0].asNumber()) + " %";
            }
            else if (type == PEN_DOWN)
            {
                blockText = "pen down";
            }
            else if (type == PEN_UP)
            {
                blockText = "pen up";
            }
            else if (type == PEN_CLEAR)
            {
                blockText = "clear";
            }
            else if (type >= OP_ADD && type <= OP_XOR)
            {
                // برای عملگرها
                if (type == OP_ADD) blockText = "0 + 0";
                else if (type == OP_SUBTRACT) blockText = "0 - 0";
                else if (type == OP_MULTIPLY) blockText = "0 * 0";
                else if (type == OP_DIVIDE) blockText = "0 / 0";
                else if (type == OP_EQUAL) blockText = "0 = 0";
                else if (type == OP_LESS_THAN) blockText = "0 < 0";
                else if (type == OP_GREATER_THAN) blockText = "0 > 0";
                else if (type == OP_NOT) blockText = "not";
                else if (type == OP_OR) blockText = "or";
                else if (type == OP_AND) blockText = "and";
                else blockText = "operator";
            }
            else if (type == SET_VARIABLE)
            {
                blockText = "set " + game.program[i].variableName + " to 0";
            }
            else
            {
                // اگه هیچکدوم نبود، از eventName استفاده کن
                blockText = game.program[i].eventName.empty() ? "Block" : game.program[i].eventName;
            }
        }

        renderText(renderer, blockText.c_str(), blockRect.x + 5, blockRect.y + 10, textColor);

        // اگه بلوک جاری در حال اجراست، حاشیه زرد
        if (i == game.currentBlockIndex && game.isRunningCode)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            for (int t = 0; t < 3; t++)
                SDL_RenderDrawRect(renderer, &blockRect);
        }
    }

    // ========== ۴. پنل استیج و اسپرایت (سمت راست) ==========
    int stagePanelX = codeAreaX + codeAreaWidth + 10;
    int stagePanelWidth = game.screenWidth - stagePanelX - 5;
    if (stagePanelWidth > 500) stagePanelWidth = 500;

    int stageWidth = stagePanelWidth - 20;
    int stageHeight = 300;
    SDL_Rect stage = {stagePanelX + 5, 70, stageWidth, stageHeight};
    SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
    SDL_RenderFillRect(renderer, &stage);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &stage);
    renderText(renderer, "Stage", stage.x + 10, stage.y + 5, black);

    // اسپرایت داخل صحنه
    if (game.player.visible)
    {
        int spriteStageX = stage.x + (int)((game.player.x / game.screenWidth) * stage.w);
        int spriteStageY = stage.y + (int)((game.player.y / game.screenHeight) * stage.h);

        if (spriteStageX < stage.x) spriteStageX = stage.x;
        if (spriteStageY < stage.y) spriteStageY = stage.y;
        if (spriteStageX + game.player.w/2 > stage.x + stage.w)
            spriteStageX = stage.x + stage.w - game.player.w/2;
        if (spriteStageY + game.player.h/2 > stage.y + stage.h)
            spriteStageY = stage.y + stage.h - game.player.h/2;

        SDL_Rect spriteRect = {spriteStageX, spriteStageY, game.player.w/2, game.player.h/2};

        // کشیدن اسپرایت
        if (game.player.texture)
        {
            SDL_Point center = {spriteRect.w/2, spriteRect.h/2};
            double angle = game.player.direction;
            SDL_RenderCopyEx(renderer, game.player.texture, NULL, &spriteRect, angle, &center, SDL_FLIP_NONE);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
            SDL_RenderFillRect(renderer, &spriteRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &spriteRect);
        }

        // ===== نمایش نام اسپرایت (با فاصله 15 پیکسل از اسپرایت) =====
        string spriteName = game.player.name;
        int nameX = spriteStageX + (game.player.w/4) - (spriteName.length() * 4);
        int nameY = spriteStageY - 15;  // 15 پیکسل فاصله از بالای اسپرایت
        renderText(renderer, spriteName.c_str(), nameX, nameY, black);

        // ===== نمایش حباب گفتگو یا فکر (بالاتر از اسم) =====
        if (!game.player.message.empty())
        {
            int bubbleWidth = 150;
            int bubbleHeight = 50;
            int bubbleX = spriteStageX + (game.player.w/4) - bubbleWidth/2;
            int bubbleY = nameY - bubbleHeight - 20;  // 20 پیکسل فاصله از اسم

            // محدود کردن به داخل استیج
            if (bubbleX < stage.x) bubbleX = stage.x;
            if (bubbleX + bubbleWidth > stage.x + stage.w)
                bubbleX = stage.x + stage.w - bubbleWidth;
            if (bubbleY < stage.y) bubbleY = stage.y + 5;  // اگه از استیج بیرون زد

            SDL_Rect bubbleRect = {bubbleX, bubbleY, bubbleWidth, bubbleHeight};

            // رنگ حباب
            if (game.player.isThinking)
            {
                // حباب فکر (خاکستری روشن با حاشیه نقطه‌چین)
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderFillRect(renderer, &bubbleRect);
                SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);

                // رسم حاشیه نقطه‌چین برای فکر
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
                // حباب گفتگو (سفید با حاشیه سیاه)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &bubbleRect);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &bubbleRect);
            }

            // نمایش متن داخل حباب
            string message = game.player.message;
            // اگه متن طولانی بود، می‌تونی برش بزنی
            if (message.length() > 20)
                message = message.substr(0, 17) + "...";

            int textX = bubbleX + (bubbleWidth - message.length() * 8) / 2;
            int textY = bubbleY + (bubbleHeight - 16) / 2;
            renderText(renderer, message.c_str(), textX, textY, black);

            // ===== دم حباب =====
            if (game.player.isThinking)
            {
                // دم حباب فکر (سه دایره)
                int circleX = spriteStageX + game.player.w/4;
                int circleY = bubbleY + bubbleHeight + 5;

                filledCircleRGBA(renderer, circleX - 15, circleY, 5, 220, 220, 220, 255);
                filledCircleRGBA(renderer, circleX, circleY + 5, 5, 220, 220, 220, 255);
                filledCircleRGBA(renderer, circleX + 15, circleY + 10, 5, 220, 220, 220, 255);
            }
            else
            {
                // دم حباب گفتگو (مثلث)
                int x1 = spriteStageX + game.player.w/4;
                int y1 = bubbleY + bubbleHeight;
                int x2 = x1 - 10;
                int y2 = y1 + 10;
                int x3 = x1 + 10;
                int y3 = y1 + 10;

                filledTrigonRGBA(renderer, x1, y1, x2, y2, x3, y3, 255, 255, 255, 255);
                aatrigonRGBA(renderer, x1, y1, x2, y2, x3, y3, 0, 0, 0, 255);
            }
        }

        if (game.player.penDown)
        {
            filledCircleRGBA(renderer, spriteRect.x + spriteRect.w + 5, spriteRect.y, 3, 255, 0, 0, 255);
        }
    }

    // پنل اسپرایت‌ها (زیر استیج) با مشخصات
    int spritePanelY = stage.y + stage.h + 10;
    SDL_Rect spritePanel = {stagePanelX + 5, spritePanelY, stageWidth, 150};
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_RenderFillRect(renderer, &spritePanel);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer, &spritePanel);

    renderText(renderer, "Sprite Properties", spritePanel.x + 10, spritePanel.y + 5, black);

    SDL_Rect currentSprite = {spritePanel.x + 10, spritePanel.y + 25, 50, 50};

    if (game.player.texture)
    {
        SDL_RenderCopy(renderer, game.player.texture, NULL, &currentSprite);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
        SDL_RenderFillRect(renderer, &currentSprite);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &currentSprite);
    }

    // نمایش مشخصات اسپرایت (قابل کلیک)
    int textX = spritePanel.x + 70;
    int textY = spritePanel.y + 25;
    int lineHeight = 20;
    SDL_Color blue = {0, 0, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    // اسم
    if (game.editingMode && game.editingField == 0)
    {
        string displayText = "Name: " + game.editingBuffer + "_";
        renderText(renderer, displayText.c_str(), textX, textY, blue);
    }
    else
    {
        string nameText = "Name: " + game.player.name;
        renderText(renderer, nameText.c_str(), textX, textY, black);
    }

    // X و Y
    char buffer[100];
    if (game.editingMode && game.editingField == 1)
    {
        sprintf(buffer, "X: %s_", game.editingBuffer.c_str());
        renderText(renderer, buffer, textX, textY + lineHeight, blue);
    }
    else
    {
        sprintf(buffer, "X: %.1f", game.player.x);
        renderText(renderer, buffer, textX, textY + lineHeight, black);
    }

    if (game.editingMode && game.editingField == 2)
    {
        sprintf(buffer, "Y: %s_", game.editingBuffer.c_str());
        renderText(renderer, buffer, textX + 120, textY + lineHeight, blue);
    }
    else
    {
        sprintf(buffer, "Y: %.1f", game.player.y);
        renderText(renderer, buffer, textX + 120, textY + lineHeight, black);
    }

    // Size
    if (game.editingMode && game.editingField == 3)
    {
        sprintf(buffer, "Size: %s_", game.editingBuffer.c_str());
        renderText(renderer, buffer, textX, textY + lineHeight * 2, blue);
    }
    else
    {
        sprintf(buffer, "Size: %d", game.player.w);
        renderText(renderer, buffer, textX, textY + lineHeight * 2, black);
    }

    // Direction
    if (game.editingMode && game.editingField == 4)
    {
        sprintf(buffer, "Dir: %s_", game.editingBuffer.c_str());
        renderText(renderer, buffer, textX, textY + lineHeight * 3, blue);
    }
    else
    {
        sprintf(buffer, "Dir: %.1f", game.player.direction);
        renderText(renderer, buffer, textX, textY + lineHeight * 3, black);
    }

    // Volume
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

    // Visible
    string visibleText = game.player.visible ? "Visible: Yes" : "Visible: No";
    renderText(renderer, visibleText.c_str(), textX, textY + lineHeight * 4, game.player.visible ? black : red);

    if (game.isPlayingSound)
    {
        // می‌توانید وضعیت پخش صدا را نمایش دهید
    }

    // ========== خطوط جداکننده ==========
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawLine(renderer, categoriesPanelWidth + 15, 70, categoriesPanelWidth + 15, game.screenHeight - 80);
    SDL_RenderDrawLine(renderer, examplesPanelX + examplesPanelWidth + 15, 70,
                       examplesPanelX + examplesPanelWidth + 15, game.screenHeight - 80);
    SDL_RenderDrawLine(renderer, codeAreaX + codeAreaWidth + 5, 70,
                       codeAreaX + codeAreaWidth + 5, game.screenHeight - 80);

    // ========== دکمه‌های کنترلی پایین صفحه ==========
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
}