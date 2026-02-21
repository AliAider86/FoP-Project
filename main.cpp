#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "engine.h"
#include "ui.h"

using namespace std;

TTF_Font* g_font = nullptr;

bool loadSound(GameState& game, const string& path)
{
    game.soundEffect = Mix_LoadWAV(path.c_str());
    if (!game.soundEffect)
    {
        cout << "Failed to load sound: " << path << " - " << Mix_GetError() << endl;
        return false;
    }
    cout << "Sound loaded: " << path << endl;
    return true;
}

bool loadSpriteTexture(Sprite& sprite, SDL_Renderer* renderer, const string& path)
{
    // اگه قبلاً بافری داشت، آزادش کن
    if (sprite.texture)
    {
        SDL_DestroyTexture(sprite.texture);
        sprite.texture = nullptr;
    }

    // بارگذاری تصویر
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface)
    {
        cout << "Failed to load image: " << path << " - " << IMG_GetError() << endl;
        return false;
    }

    // ساخت بافت
    sprite.texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!sprite.texture)
    {
        cout << "Failed to create texture: " << SDL_GetError() << endl;
        SDL_FreeSurface(surface);
        return false;
    }

    // تنظیم اندازه اسپرایت بر اساس تصویر
    sprite.w = surface->w;
    sprite.h = surface->h;
    sprite.imagePath = path;

    SDL_FreeSurface(surface);
    cout << "Image loaded: " << path << " (" << sprite.w << "x" << sprite.h << ")" << endl;
    return true;
}

void renderText(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color)
{
    if (!g_font || !text) return;
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, text, color);
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

// توی main، بعد از SDL_Init و TTF_Init:
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        cout << "SDL_mixer could not initialize! Error: " << Mix_GetError() << endl;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    SDL_Window* window = SDL_CreateWindow(
            "FoP Project - Scratch Simulator",
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

    // بارگذاری فونت
    g_font = TTF_OpenFont("arial.ttf", 18);
    if (!g_font)
    {
        cout << "Failed to load font: " << TTF_GetError() << endl;
        cout << "Using rectangle placeholders instead." << endl;
    }

    // بعد از ایجاد renderer
    loadSpriteTexture(game.player, renderer, "cat.png"); // یا هر عکس دیگه
    game.player.w = 158;
    game.player.h = 170;

    loadSound(game, "meow.wav");  // فایل صوتی رو توی پوشه پروژه بذار

    // مقداردهی اولیه game
//    game.player.w = 50;
//    game.player.h = 50;
    game.player.x = double(dm.w) / 2 - 25;
    game.player.y = double(dm.h) / 2 - 25;
    game.screenWidth = dm.w;
    game.screenHeight = dm.h;
    game.player.name = "Sprite1";

    // دکمه‌های کنترلی پایین
    int buttonY = game.screenHeight - 80;
    int buttonWidth = 100;
    int buttonHeight = 40;
    int buttonSpacing = 10;
    int startX = (game.screenWidth - (6 * (buttonWidth + buttonSpacing))) / 2;

    game.runButton = Button(startX, buttonY, buttonWidth, buttonHeight);
    game.pauseButton = Button(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);
    game.stepButton = Button(startX + 2*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight);
    game.resetButton = Button(startX + 3*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight);
    game.saveButton = Button(startX + 4*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight);
    game.loadButton = Button(startX + 5*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight);

    // دکمه‌های دسته‌بندی (کتگوری‌ها)
    int toolPanelWidth = 180;
    game.moveCategoryBtn = Button(20, 110, toolPanelWidth-20, 35);
    game.looksCategoryBtn = Button(20, 155, toolPanelWidth-20, 35);
    game.soundCategoryBtn = Button(20, 200, toolPanelWidth-20, 35);
    game.eventsCategoryBtn = Button(20, 245, toolPanelWidth-20, 35);
    game.controlCategoryBtn = Button(20, 290, toolPanelWidth-20, 35);
    game.sensingCategoryBtn = Button(20, 335, toolPanelWidth-20, 35);
    game.operatorsCategoryBtn = Button(20, 380, toolPanelWidth-20, 35);
    game.variablesCategoryBtn = Button(20, 425, toolPanelWidth-20, 35);

    game.showSpriteName = true;

//// ===== برنامه تست =====
//    game.program.clear();
//    game.scriptStartIndices.clear();
//    game.scriptActive.clear();
//    game.scriptCurrentBlock.clear();
//
//// بلوک شروع (WHEN_GREEN_FLAG) - این باعث میشه بقیه بلوک‌ها اجرا بشن
//    Block flagBlock(WHEN_GREEN_FLAG);
//    game.program.push_back(flagBlock);
//
//// حالا بلوک‌های دیگه رو می‌تونی با کلیک اضافه کنی
//// یا می‌تونی چندتا نمونه اضافه کنی برای تست:
//    Block testMove(MOVE_RIGHT);
//    testMove.parameters.push_back(Value(50.0));
//    game.program.push_back(testMove);
//
//    Block testWait(WAIT);
//    testWait.parameters.push_back(Value(1.0));
//    game.program.push_back(testWait);
//
//    Block testMove2(MOVE_DOWN);
//    testMove2.parameters.push_back(Value(30.0));
//    game.program.push_back(testMove2);

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
    if (game.soundEffect)
        Mix_FreeChunk(game.soundEffect);

    Mix_CloseAudio();

    return 0;
}