#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "engine.h"
#include "ui.h"

using namespace std;

TTF_Font* g_font = nullptr;

void renderText(SDL_Renderer* renderer, const string& text, int x, int y, SDL_Color color)
{
    if (!g_font || text.empty()) return;
    SDL_Surface* surface = TTF_RenderText_Solid(g_font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char* argv[])
{
    GameState game;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        cout << "SDL Init Error: " << SDL_GetError() << endl;
        return -1;
    }

    if (TTF_Init() != 0)
    {
        cout << "TTF Init Error: " << TTF_GetError() << endl;
        return -1;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    SDL_Window* window = SDL_CreateWindow(
            "FoP Project - Pen Test",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            dm.w,
            dm.h,
            SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (!window)
    {
        cout << "Window Error: " << SDL_GetError() << endl;
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        cout << "Renderer Error: " << SDL_GetError() << endl;
        return -1;
    }

    g_font = TTF_OpenFont("arial.ttf", 24);
    if (!g_font)
    {
        cout << "Failed to load font: " << TTF_GetError() << endl;
    }

    // مقداردهی اولیه game
    game.player.w = 50;
    game.player.h = 50;
    game.player.x = double(dm.w) / 2 - 25;
    game.player.y = double(dm.h) / 2 - 25;
    game.screenWidth = dm.w;
    game.screenHeight = dm.h;

    game.runButton = Button(50, game.screenHeight - 100, 120, 50);
    game.pauseButton = Button(200, game.screenHeight - 100, 120, 50);
    game.stepButton = Button(350, game.screenHeight - 100, 120, 50);
    game.resetButton = Button(500, game.screenHeight - 100, 120, 50);

    // ===== برنامه تست قلم =====
    game.program.clear();
    game.scriptStartIndices.clear();
    game.scriptActive.clear();
    game.scriptCurrentBlock.clear();

// ===== اسکریپت ۱: فعال کردن قلم با کلید P (با FOREVER) =====
    Block whenKeyPBlock(WHEN_KEY_PRESSED);
    whenKeyPBlock.keyCode = SDL_SCANCODE_P;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKeyPBlock);

// حلقه بی‌نهایت
    Block foreverBlock(FOREVER);
    game.program.push_back(foreverBlock);

// قلم پایین
    Block penDownBlock(PEN_DOWN);
    game.program.push_back(penDownBlock);

// یه کم صبر کن تا برنامه قفل نشه
    Block waitBlock(WAIT);
    waitBlock.parameters.push_back(Value(0.1));
    game.program.push_back(waitBlock);

    Block endForeverBlock(END_FOREVER);
    game.program.push_back(endForeverBlock);

    // ===== اسکریپت ۲: غیرفعال کردن قلم با کلید O =====
    Block whenKeyOBlock(WHEN_KEY_PRESSED);
    whenKeyOBlock.keyCode = SDL_SCANCODE_O;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKeyOBlock);

    Block penUpBlock(PEN_UP);
    game.program.push_back(penUpBlock);

    // ===== اسکریپت ۳: تنظیم رنگ قرمز با کلید R =====
    Block whenKeyRBlock(WHEN_KEY_PRESSED);
    whenKeyRBlock.keyCode = SDL_SCANCODE_R;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKeyRBlock);

    Block setRedBlock(SET_PEN_COLOR);
    setRedBlock.parameters.push_back(Value(255.0));
    setRedBlock.parameters.push_back(Value(0.0));
    setRedBlock.parameters.push_back(Value(0.0));
    game.program.push_back(setRedBlock);

    // ===== اسکریپت ۴: تنظیم رنگ سبز با کلید G =====
    Block whenKeyGBlock(WHEN_KEY_PRESSED);
    whenKeyGBlock.keyCode = SDL_SCANCODE_G;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKeyGBlock);

    Block setGreenBlock(SET_PEN_COLOR);
    setGreenBlock.parameters.push_back(Value(0.0));
    setGreenBlock.parameters.push_back(Value(255.0));
    setGreenBlock.parameters.push_back(Value(0.0));
    game.program.push_back(setGreenBlock);

    // ===== اسکریپت ۵: تنظیم رنگ آبی با کلید B =====
    Block whenKeyBBlock(WHEN_KEY_PRESSED);
    whenKeyBBlock.keyCode = SDL_SCANCODE_B;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKeyBBlock);

    Block setBlueBlock(SET_PEN_COLOR);
    setBlueBlock.parameters.push_back(Value(0.0));
    setBlueBlock.parameters.push_back(Value(0.0));
    setBlueBlock.parameters.push_back(Value(255.0));
    game.program.push_back(setBlueBlock);

    // ===== اسکریپت ۶: ضخامت ۱ با کلید 1 =====
    Block whenKey1Block(WHEN_KEY_PRESSED);
    whenKey1Block.keyCode = SDL_SCANCODE_1;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKey1Block);

    Block setSize1Block(SET_PEN_SIZE);
    setSize1Block.parameters.push_back(Value(1.0));
    game.program.push_back(setSize1Block);

    // ===== اسکریپت ۷: ضخامت ۵ با کلید 5 =====
    Block whenKey5Block(WHEN_KEY_PRESSED);
    whenKey5Block.keyCode = SDL_SCANCODE_5;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKey5Block);

    Block setSize5Block(SET_PEN_SIZE);
    setSize5Block.parameters.push_back(Value(5.0));
    game.program.push_back(setSize5Block);

    // ===== اسکریپت ۸: پاک کردن صفحه با کلید X =====
    Block whenKeyXBlock(WHEN_KEY_PRESSED);
    whenKeyXBlock.keyCode = SDL_SCANCODE_X;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenKeyXBlock);

    Block clearBlock(PEN_CLEAR);
    game.program.push_back(clearBlock);

    // ===== اسکریپت ۹: حرکت با کلیدهای جهت‌دار =====
// بالا
    Block whenUpBlock(WHEN_KEY_PRESSED);
    whenUpBlock.keyCode = SDL_SCANCODE_UP;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenUpBlock);

    Block moveUpBlock(MOVE_UP);
    moveUpBlock.parameters.push_back(Value(5.0));
    game.program.push_back(moveUpBlock);

// پایین
    Block whenDownBlock(WHEN_KEY_PRESSED);
    whenDownBlock.keyCode = SDL_SCANCODE_DOWN;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenDownBlock);

    Block moveDownBlock(MOVE_DOWN);
    moveDownBlock.parameters.push_back(Value(5.0));
    game.program.push_back(moveDownBlock);

// چپ
    Block whenLeftBlock(WHEN_KEY_PRESSED);
    whenLeftBlock.keyCode = SDL_SCANCODE_LEFT;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenLeftBlock);

    Block moveLeftBlock(MOVE_LEFT);
    moveLeftBlock.parameters.push_back(Value(5.0));
    game.program.push_back(moveLeftBlock);

// راست
    Block whenRightBlock(WHEN_KEY_PRESSED);
    whenRightBlock.keyCode = SDL_SCANCODE_RIGHT;
    game.scriptStartIndices.push_back(game.program.size());
    game.scriptActive.push_back(true);
    game.scriptCurrentBlock.push_back(game.program.size() + 1);
    game.program.push_back(whenRightBlock);

    Block moveRightBlock(MOVE_RIGHT);
    moveRightBlock.parameters.push_back(Value(5.0));
    game.program.push_back(moveRightBlock);

    cout << "Pen Test Program Loaded!" << endl;
    cout << "Controls:" << endl;
    cout << "  P: Pen Down" << endl;
    cout << "  O: Pen Up" << endl;
    cout << "  R: Red Color" << endl;
    cout << "  G: Green Color" << endl;
    cout << "  B: Blue Color" << endl;
    cout << "  1: Pen Size 1" << endl;
    cout << "  5: Pen Size 5" << endl;
    cout << "  X: Clear Screen" << endl;
    cout << "  Arrow Keys: Move" << endl;

    bool running = true;

    while (running)
    {
        handleEvents(running, game);
        update(game);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render(renderer, game);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (g_font)
        TTF_CloseFont(g_font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}