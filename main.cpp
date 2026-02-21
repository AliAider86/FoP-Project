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
    if (!sprite || !renderer || !path) return false;

    if (sprite->texture)
    {
        SDL_DestroyTexture(sprite->texture);
        sprite->texture = nullptr;
    }

    SDL_Surface* surface = IMG_Load(path);
    if (!surface) return false;

    sprite->texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!sprite->texture)
    {
        SDL_FreeSurface(surface);
        return false;
    }

    // سایز ثابت 50 پیکسل (مهم نیست عکس اصلی چقدر بزرگه)
    sprite->w = 150;
    sprite->h = 150;

    sprite->imagePath = path;
    SDL_FreeSurface(surface);

    log_info(("Image loaded and set to 50x50: " + string(path)).c_str());
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

// --- تابع کمکی برای اضافه کردن اسپرایت جدید ---
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
    newSprite.penDown = false;
    newSprite.penSize = 1;
    newSprite.penR = 0;
    newSprite.penG = 0;
    newSprite.penB = 0;
    newSprite.texture = nullptr;
    newSprite.imagePath = imagePath ? imagePath : "";
    newSprite.index = game.sprites.size();
    newSprite.isActive = false;

    // بارگذاری تصویر اگر مسیر داده شده
    if (imagePath && renderer)
        loadSpriteTexture(&newSprite, renderer, imagePath);

    game.sprites.push_back(newSprite);
    log_info(("Sprite added: " + string(name)).c_str());
}

int main(int argc, char* argv[])
{
    log_info("Program started");


    GameState game;
    game.sprites = vector<Sprite>();
    game.activeSpriteIndex = -1;
// ... بقیه مقداردهی‌ها رو هم می‌تونی بعداً انجام بدی

    // --- مقداردهی اولیه درست vector ---
    game.sprites = vector<Sprite>();  // اینجا مطمئن میشیم خالی شروع میشه
    game.activeSpriteIndex = -1;  // هیچ اسپرایتی فعال نیست

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

    g_font = TTF_OpenFont("consolas.ttf", 18);
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
            printf("Logo pointer: %p\n", game.logoTexture);  // توی کنسول ببین
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

    // --- اضافه کردن چند اسپرایت پیش‌فرض (بعد از ست شدن screenWidth/screenHeight) ---
    addDefaultSprite(game, renderer, "Cat1", "cat.png");
    addDefaultSprite(game, renderer, "Cat2", "cat.png");
    addDefaultSprite(game, renderer, "Cat3", "cat.png");

    // فعال کردن اولین اسپرایت
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

    game.runButton = (Button){startX, buttonY, buttonWidth, buttonHeight, 0};
    game.pauseButton = (Button){startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight, 0};
    game.stepButton = (Button){startX + 2*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};
    game.resetButton = (Button){startX + 3*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};
    game.saveButton = (Button){startX + 4*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};
    game.loadButton = (Button){startX + 5*(buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight, 0};

    // --- دکمه‌های مدیریت اسپرایت ---
    game.addSpriteBtn = (Button){startX - 120, buttonY, 100, buttonHeight, 0};
    game.deleteSpriteBtn = (Button){startX - 230, buttonY, 100, buttonHeight, 0};
    game.prevSpriteBtn = (Button){startX - 340, buttonY, 50, buttonHeight, 0};
    game.nextSpriteBtn = (Button){startX - 280, buttonY, 50, buttonHeight, 0};

    int toolPanelWidth = 180;
    game.moveCategoryBtn = (Button){20, 110, toolPanelWidth-20, 35, 0};
    game.looksCategoryBtn = (Button){20, 155, toolPanelWidth-20, 35, 0};
    game.soundCategoryBtn = (Button){20, 200, toolPanelWidth-20, 35, 0};
    game.eventsCategoryBtn = (Button){20, 245, toolPanelWidth-20, 35, 0};
    game.controlCategoryBtn = (Button){20, 290, toolPanelWidth-20, 35, 0};
    game.sensingCategoryBtn = (Button){20, 335, toolPanelWidth-20, 35, 0};
    game.operatorsCategoryBtn = (Button){20, 380, toolPanelWidth-20, 35, 0};
    game.variablesCategoryBtn = (Button){20, 425, toolPanelWidth-20, 35, 0};


    game.volume = 100;
    game.showSpriteName = 1;

    SDL_StartTextInput();

    bool running = true;

    while (running)
    {
        handleEvents(running, game, renderer);
        update(game);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render(renderer, game);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // پاکسازی texture همه اسپرایت‌ها
    for (auto& sprite : game.sprites)
    {
        if (sprite.texture)
            SDL_DestroyTexture(sprite.texture);
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

    // بعد از حلقه اصلی، قبل از خروج
    if (game.logoTexture)
    {
        SDL_DestroyTexture(game.logoTexture);
    }

    return 0;
}