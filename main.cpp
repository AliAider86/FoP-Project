#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
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
    if (!sprite || !renderer || !path) return false;

    if (sprite->texture)
    {
        SDL_DestroyTexture(sprite->texture);
        sprite->texture = nullptr;
    }

    SDL_Surface* surface = IMG_Load(path);
    if (!surface)
    {
        log_error(("Failed to load image: " + string(path) + " - " + string(IMG_GetError())).c_str());
        return false;
    }

    sprite->texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!sprite->texture)
    {
        log_error(("Failed to create texture: " + string(SDL_GetError())).c_str());
        SDL_FreeSurface(surface);
        return false;
    }

    sprite->w = surface->w;
    sprite->h = surface->h;

    if (sprite->w > 100 || sprite->h > 100)
    {
        double ratio = (double)sprite->w / sprite->h;
        if (sprite->w > 100)
        {
            sprite->w = 100;
            sprite->h = (int)(100 / ratio);
        }
        if (sprite->h > 100)
        {
            sprite->h = 100;
            sprite->w = (int)(100 * ratio);
        }
        log_debug(("Resized large sprite to: " + to_string(sprite->w) + "x" + to_string(sprite->h)).c_str());
    }

    sprite->imagePath = path;
    SDL_FreeSurface(surface);

    log_info(("Image loaded: " + string(path) + " (" + to_string(sprite->w) + "x" + to_string(sprite->h) + ")").c_str());
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

void addDefaultSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath)
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
    newSprite.texture = nullptr;
    newSprite.imagePath = imagePath ? imagePath : "";
    newSprite.index = game.sprites.size();
    newSprite.isActive = false;

    if (imagePath && renderer && strlen(imagePath) > 0)
    {
        bool loaded = loadSpriteTexture(&newSprite, renderer, imagePath);
        if (!loaded)
        {
            log_warning(("Failed to load image: " + string(imagePath) + ", using rectangle").c_str());
            newSprite.w = 50;
            newSprite.h = 50;
        }
    }
    else
    {
        log_warning(("No image path provided for sprite: " + string(name) + ", using rectangle").c_str());
        newSprite.w = 50;
        newSprite.h = 50;
    }

    game.sprites.push_back(newSprite);
    log_info(("Sprite added: " + string(name) + " - size: " + to_string(newSprite.w) + "x" + to_string(newSprite.h)).c_str());
}

void initDefaultBackdrops(GameState& game, SDL_Renderer* renderer)
{
    const char* defaultBackdrops[] = {
            "backdrops/blue_sky.png",
            "backdrops/green_field.png",
            "backdrops/space.png",
            "backdrops/underwater.png"
    };

    const char* backdropNames[] = {
            "Blue Sky",
            "Green Field",
            "Space",
            "Underwater"
    };

    for (int i = 0; i < 4; i++)
    {
        Backdrop b;
        b.name = backdropNames[i];
        b.filePath = defaultBackdrops[i];
        b.isCustom = false;

        SDL_Surface* surface = IMG_Load(defaultBackdrops[i]);
        if (surface)
        {
            b.texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            log_info(("Loaded backdrop: " + string(backdropNames[i])).c_str());
        }
        else
        {
            b.texture = nullptr;
            log_warning(("Could not load backdrop: " + string(defaultBackdrops[i])).c_str());
        }

        game.backdrops.push_back(b);
    }

    game.currentBackdrop = 0;
}

int main(int argc, char* argv[])
{
    log_info("Program started");

    // استفاده از سازنده GameState برای مقداردهی خودکار
    GameState game;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
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
        log_error("Failed to load font, trying arial.ttf");
        g_font = TTF_OpenFont("arial.ttf", 18);
        if (!g_font)
            log_error("Failed to load any font");
    }
    else
    {
        log_info("Font loaded");
    }

    SDL_Surface* logoSrf = IMG_Load("sut.png");
    if (logoSrf)
    {
        game.logoTexture = SDL_CreateTextureFromSurface(renderer, logoSrf);
        SDL_FreeSurface(logoSrf);

        if (game.logoTexture)
        {
            log_info("Logo texture created");
        }
        else
        {
            log_error("Failed to create logo texture");
            game.logoTexture = nullptr;
        }
    }
    else
    {
        log_error("Failed to load sut.png");
        game.logoTexture = nullptr;
    }

    game.screenWidth = dm.w;
    game.screenHeight = dm.h;

    addDefaultSprite(game, renderer, "Sprite1", "cat.png");

    if (game.sprites.size() > 0)
    {
        game.activeSpriteIndex = 0;
        game.sprites[0].isActive = true;
        log_info(("Active sprite: " + game.sprites[0].name).c_str());
    }

    loadSound(game, "meow.wav");

    int buttonY = game.screenHeight - 80;
    int buttonWidth = 100;
    int buttonHeight = 40;
    int buttonSpacing = 10;
    int startX = (game.screenWidth - (6 * (buttonWidth + buttonSpacing))) / 2;

    int spriteBtnStartX = game.screenWidth - 350;
    game.addSpriteBtn = (Button){spriteBtnStartX, buttonY, 60, buttonHeight, 0};
    game.deleteSpriteBtn = (Button){spriteBtnStartX + 70, buttonY, 60, buttonHeight, 0};
    game.prevSpriteBtn = (Button){spriteBtnStartX + 140, buttonY, 40, buttonHeight, 0};
    game.nextSpriteBtn = (Button){spriteBtnStartX + 190, buttonY, 40, buttonHeight, 0};

    int backdropStartX = 50;
    game.prevBackdropBtn = (Button){backdropStartX, buttonY, 30, buttonHeight, 0};
    game.nextBackdropBtn = (Button){backdropStartX + 40, buttonY, 30, buttonHeight, 0};
    game.uploadBackdropBtn = (Button){backdropStartX + 80, buttonY, 100, buttonHeight, 0};

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
    game.penCategoryBtn = (Button){20, 470, toolPanelWidth-20, 35, 0}; // اضافه شد

    initDefaultBackdrops(game, renderer);
    initPaletteBlocks(game);

    game.volume = 100;
    game.showSpriteName = 1;

    SDL_StartTextInput();

    bool running = true;

    while (running)
    {
        handleEvents(running, game, renderer);
        update(game, renderer);

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);

        render(renderer, game);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    for (auto& sprite : game.sprites)
    {
        if (sprite.texture)
            SDL_DestroyTexture(sprite.texture);
    }

    for (auto& b : game.backdrops)
    {
        if (b.texture)
            SDL_DestroyTexture(b.texture);
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

    if (game.logoTexture)
    {
        SDL_DestroyTexture(game.logoTexture);
    }

    log_info("Program ended");

    return 0;
}