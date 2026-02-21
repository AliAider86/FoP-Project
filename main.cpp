#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "engine.h"
#include "ui.h"
#include "logger.h"

TTF_Font* g_font = nullptr;

bool loadSound(GameState& game, const char* path)
{
    game.soundEffect = Mix_LoadWAV(path);
    if (!game.soundEffect)
    {
        log_error("Failed to load sound");
        return false;
    }
    log_info("Sound loaded");
    return true;
}

bool loadSpriteTexture(Sprite* sprite, SDL_Renderer* renderer, const char* path)
{
    if (sprite->texture)
    {
        SDL_DestroyTexture(sprite->texture);
        sprite->texture = nullptr;
    }

    SDL_Surface* surface = IMG_Load(path);
    if (!surface)
    {
        log_error("Failed to load image");
        return false;
    }

    sprite->texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!sprite->texture)
    {
        log_error("Failed to create texture");
        SDL_FreeSurface(surface);
        return false;
    }

    sprite->w = surface->w;
    sprite->h = surface->h;

    SDL_FreeSurface(surface);
    log_info("Image loaded");
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
    log_info("Program started");

    GameState game = {0};

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        log_error("SDL Init Error");
        return -1;
    }
    log_info("SDL initialized");

    if (TTF_Init() != 0)
    {
        log_error("TTF Init Error");
        return -1;
    }
    log_info("TTF initialized");

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        log_error("SDL_mixer could not initialize");
    }
    else
    {
        log_info("SDL_mixer initialized");
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
        log_error("Window Error");
        return -1;
    }
    log_info("Window created");

    SDL_Renderer* renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        log_error("Renderer Error");
        return -1;
    }
    log_info("Renderer created");

    g_font = TTF_OpenFont("arial.ttf", 18);
    if (!g_font)
    {
        log_error("Failed to load font");
    }
    else
    {
        log_info("Font loaded");
    }

    if (loadSpriteTexture(&game.player, renderer, "cat.png"))
    {
        game.player.w = 158;
        game.player.h = 170;
    }

    loadSound(game, "meow.wav");

    game.player.x = (double)dm.w / 2 - 25;
    game.player.y = (double)dm.h / 2 - 25;
    game.screenWidth = dm.w;
    game.screenHeight = dm.h;
    game.player.name = "Sprite1";

    int buttonY = game.screenHeight - 80;
    int buttonWidth = 100;
    int buttonHeight = 40;
    int buttonSpacing = 10;
    int startX = (game.screenWidth - (6 * (buttonWidth + buttonSpacing))) / 2;

    game.runButton = (Button){startX, buttonY, buttonWidth, buttonHeight, 0};
    game.pauseButton = (Button){startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight, 0};
    game.stepButton = (Button){startX + 2*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};
    game.resetButton = (Button){startX + 3*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};
    game.saveButton = (Button){startX + 4*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};
    game.loadButton = (Button){startX + 5*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};

    int toolPanelWidth = 180;
    game.moveCategoryBtn = (Button){20, 110, toolPanelWidth-20, 35, 0};
    game.looksCategoryBtn = (Button){20, 155, toolPanelWidth-20, 35, 0};
    game.soundCategoryBtn = (Button){20, 200, toolPanelWidth-20, 35, 0};
    game.eventsCategoryBtn = (Button){20, 245, toolPanelWidth-20, 35, 0};
    game.controlCategoryBtn = (Button){20, 290, toolPanelWidth-20, 35, 0};
    game.sensingCategoryBtn = (Button){20, 335, toolPanelWidth-20, 35, 0};
    game.operatorsCategoryBtn = (Button){20, 380, toolPanelWidth-20, 35, 0};
    game.variablesCategoryBtn = (Button){20, 425, toolPanelWidth-20, 35, 0};

    game.player.visible = true;
    game.volume = 100;

    game.showSpriteName = 1;

    SDL_StartTextInput();

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

    log_info("Program ended");

    return 0;
}