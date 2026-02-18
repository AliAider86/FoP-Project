#include "ui.h"
#include <SDL2/SDL2_gfx.h>
#include <string>
#include <iostream>

using namespace std;

// تابع برای کشیدن متن

void handleEvents(bool &running, GameState& game)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            running = false;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            running = false;

        // ===== تشخیص کلیدهای فشرده =====
        if (e.type == SDL_KEYDOWN)
        {
            game.pressedKeys[e.key.keysym.scancode] = 1;
        }

        if (e.type == SDL_KEYUP)
        {
            game.pressedKeys[e.key.keysym.scancode] = 0;
        }

        // ===== تشخیص موقعیت ماوس =====
        if (e.type == SDL_MOUSEMOTION)
        {
            game.mouseX = e.motion.x;
            game.mouseY = e.motion.y;
        }

        // ===== تشخیص کلیک ماوس =====
        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            game.mousePressed = true;
            int mx = e.button.x;
            int my = e.button.y;

            // چک کردن کلیک روی اسپرایت
            if (mx >= game.player.x && mx <= game.player.x + game.player.w &&
                my >= game.player.y && my <= game.player.y + game.player.h)
            {
                game.spriteClicked = true;
            }

            // Reset all button press states
            game.runButton.isPressed = false;
            game.pauseButton.isPressed = false;
            game.stepButton.isPressed = false;
            game.resetButton.isPressed = false;

            // دکمه Run
            if (mx >= game.runButton.x && mx <= game.runButton.x + game.runButton.w &&
                my >= game.runButton.y && my <= game.runButton.y + game.runButton.h)
            {
                game.runButton.isPressed = true;
                game.isRunningCode = true;
                game.greenFlagPressed = true;  // پرچم سبز زده شد

                // ریست کردن وضعیت اجرا
                game.currentBlockIndex = 0;
                game.isExecutingBlock = false;
                game.remainingMove = 0;
                game.isWaiting = false;
                game.isShowingMessage = false;
                game.repeatCountStack.clear();
                game.repeatStartStack.clear();
                game.player.message = "";
            }

            // دکمه Pause
            if (mx >= game.pauseButton.x && mx <= game.pauseButton.x + game.pauseButton.w &&
                my >= game.pauseButton.y && my <= game.pauseButton.y + game.pauseButton.h)
            {
                game.pauseButton.isPressed = true;
                game.isRunningCode = false;
            }

            // دکمه Step
            if (mx >= game.stepButton.x && mx <= game.stepButton.x + game.stepButton.w &&
                my >= game.stepButton.y && my <= game.stepButton.y + game.stepButton.h)
            {
                game.stepButton.isPressed = true;
                game.stepMode = true;
                game.isRunningCode = true;
            }

            // دکمه Reset
            if (mx >= game.resetButton.x &&
                mx <= game.resetButton.x + game.resetButton.w &&
                my >= game.resetButton.y &&
                my <= game.resetButton.y + game.resetButton.h)
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
                game.player.message = "";
                game.player.isThinking = false;
            }
        }

        if (e.type == SDL_MOUSEBUTTONUP)
        {
            game.mousePressed = false;
            game.spriteClicked = false;

            game.runButton.isPressed = false;
            game.pauseButton.isPressed = false;
            game.stepButton.isPressed = false;
            game.resetButton.isPressed = false;
        }
    }

    // ===== حرکت مستقیم با کلیدهای جهت‌دار =====
    if (game.isRunningCode)
    {
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        double moveSpeed = 3.0;

        if (keys[SDL_SCANCODE_UP])
            game.player.y -= moveSpeed;
        if (keys[SDL_SCANCODE_DOWN])
            game.player.y += moveSpeed;
        if (keys[SDL_SCANCODE_LEFT])
            game.player.x -= moveSpeed;
        if (keys[SDL_SCANCODE_RIGHT])
            game.player.x += moveSpeed;

        // رسم خط اگر قلم پایینه
        if (game.player.penDown)
        {
            double centerX = game.player.x + game.player.w/2;
            double centerY = game.player.y + game.player.h/2;

            if (game.player.penMoved)
            {
                game.penX1.push_back((int)game.player.lastPenX);
                game.penY1.push_back((int)game.player.lastPenY);
                game.penX2.push_back((int)centerX);
                game.penY2.push_back((int)centerY);
                game.penR_.push_back(game.player.penR);
                game.penG_.push_back(game.player.penG);
                game.penB_.push_back(game.player.penB);
                game.penSize_.push_back(game.player.penSize);
            }

            game.player.lastPenX = centerX;
            game.player.lastPenY = centerY;
            game.player.penMoved = true;
        }

        // محدودیت مرزها
        if (game.player.x < 0) game.player.x = 0;
        if (game.player.y < 0) game.player.y = 0;
        if (game.player.x + game.player.w > game.screenWidth)
            game.player.x = game.screenWidth - game.player.w;
        if (game.player.y + game.player.h > game.screenHeight)
            game.player.y = game.screenHeight - game.player.h;
    }
}

void render(SDL_Renderer* renderer, GameState& game)
{
    // ========== ابتدا خطوط قلم رو بکش ==========
    for (int i = 0; i < game.penX1.size(); i++)
    {
        // رسم خط با ضخامت مشخص
        for (int thickness = 0; thickness < game.penSize_[i]; thickness++)
        {
            int offset = thickness - game.penSize_[i]/2;
            thickLineRGBA(renderer,
                          game.penX1[i], game.penY1[i] + offset,
                          game.penX2[i], game.penY2[i] + offset,
                          1,
                          game.penR_[i], game.penG_[i], game.penB_[i], 255);
        }
    }

    // ========== نوار بالایی ==========
    int red, green, blue;

    if (game.stepMode)
    {
        red = 220; green = 220; blue = 80;
    }
    else if (game.isRunningCode)
    {
        red = 80; green = 220; blue = 80;
    }
    else
    {
        red = 80; green = 80; blue = 220;
    }

    SDL_Rect topBar = {0, 0, game.screenWidth, 40};
    SDL_SetRenderDrawColor(renderer, red/2, green/2, blue/2, 255);
    SDL_RenderFillRect(renderer, &topBar);

    // ========== پنل بلوک‌ها ==========
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

        BlockType type = game.program[i].type;

        if (type >= MOVE_UP && type <= MOVE_RIGHT)
            SDL_SetRenderDrawColor(renderer, 70, 120, 255, 255);
        else if (type >= REPEAT && type <= WAIT)
            SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
        else if (type >= OP_ADD && type <= OP_XOR)
            SDL_SetRenderDrawColor(renderer, 70, 200, 70, 255);
        else if (type >= SAY && type <= THINK_FOR)
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        else if (type >= PEN_DOWN && type <= CHANGE_PEN_SIZE)
            SDL_SetRenderDrawColor(renderer, 50, 200, 150, 255);  // فیروزه‌ای برای قلم
        else
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);

        if (i == game.currentBlockIndex && game.isRunningCode)
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);

        SDL_RenderFillRect(renderer, &blockRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &blockRect);
    }

    // ========== دکمه‌ها ==========
    SDL_Rect runRect = {game.runButton.x, game.runButton.y, game.runButton.w, game.runButton.h};
    if (game.runButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &runRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &runRect);

    SDL_Rect pauseRect = {game.pauseButton.x, game.pauseButton.y, game.pauseButton.w, game.pauseButton.h};
    if (game.pauseButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &pauseRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &pauseRect);

    SDL_Rect stepRect = {game.stepButton.x, game.stepButton.y, game.stepButton.w, game.stepButton.h};
    if (game.stepButton.isPressed)
        SDL_SetRenderDrawColor(renderer, 0, 0, 100, 255);
    else
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
    SDL_RenderFillRect(renderer, &stepRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &stepRect);

    if (game.resetButton.isPressed)
        boxRGBA(renderer,
                game.resetButton.x,
                game.resetButton.y,
                game.resetButton.x + game.resetButton.w,
                game.resetButton.y + game.resetButton.h,
                100, 50, 10, 255);
    else
        boxRGBA(renderer,
                game.resetButton.x,
                game.resetButton.y,
                game.resetButton.x + game.resetButton.w,
                game.resetButton.y + game.resetButton.h,
                139, 69, 19, 255);

    rectangleRGBA(renderer,
                  game.resetButton.x,
                  game.resetButton.y,
                  game.resetButton.x + game.resetButton.w,
                  game.resetButton.y + game.resetButton.h,
                  255, 255, 255, 255);

    // ========== اسپرایت ==========
    if (game.player.visible)
    {
        SDL_Rect spriteRect = {
                (int)game.player.x,
                (int)game.player.y,
                game.player.w,
                game.player.h
        };

        // اگه قلم پایینه، یه علامت کنار اسپرایت بذار
        if (game.player.penDown)
        {
            // یه دایره قرمز کنار اسپرایت
            filledCircleRGBA(renderer,
                             (int)game.player.x + game.player.w + 10,
                             (int)game.player.y,
                             5, 255, 0, 0, 255);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &spriteRect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &spriteRect);

        // ========== حباب گفتن ==========
        if (!game.player.message.empty())
        {
            SDL_Rect bubble;
            bubble.x = (int)game.player.x + game.player.w/2 - 75;
            bubble.y = (int)game.player.y - 80;
            bubble.w = 150;
            bubble.h = 50;

            if (game.player.isThinking)
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
            else
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            SDL_RenderFillRect(renderer, &bubble);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &bubble);
        }
    }

    // ========== پنل متغیرها ==========
    if (!game.variables.empty())
    {
        int panelX = game.screenWidth - 250;
        int panelY = 100;
        int panelWidth = 230;
        int panelHeight = (int)game.variables.size() * 30 + 20;

        SDL_Rect varPanel = {panelX, panelY, panelWidth, panelHeight};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200);
        SDL_RenderFillRect(renderer, &varPanel);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &varPanel);
    }
}