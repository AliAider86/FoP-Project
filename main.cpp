#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfx.h>

using namespace std;

struct Sprite
{
    double x, y;
    int w, h;
    bool visible;
};

Sprite player;

struct Button
{
    int x, y, w, h;
};

Button runButton;
Button pauseButton;

int screenWidth;
int screenHeight;
bool isRunningCode = false;

void handleEvents(bool &running)
{
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            running = false;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            running = false;

        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            int mx = e.button.x;
            int my = e.button.y;

            if (mx >= runButton.x && mx <= runButton.x + runButton.w &&
                my >= runButton.y && my <= runButton.y + runButton.h)
                isRunningCode = true;

            if (mx >= pauseButton.x && mx <= pauseButton.x + pauseButton.w &&
                my >= pauseButton.y && my <= pauseButton.y + pauseButton.h)
                isRunningCode = false;
        }
    }
}

void update()
{
    if (!isRunningCode)
        return;

    const double speed = 5.0;
    const Uint8* keystate = SDL_GetKeyboardState(nullptr);

    if (keystate[SDL_SCANCODE_UP])
    {
        if (player.y <= 0)
            player.y = 0;
        else
            player.y -= speed;
    }

    if (keystate[SDL_SCANCODE_DOWN])
    {
        if (player.y >= screenHeight - player.h)
            player.y = screenHeight - player.h;
        else
            player.y += speed;
    }

    if (keystate[SDL_SCANCODE_LEFT])
    {
        if (player.x <= 0)
            player.x = 0;
        else
            player.x -= speed;
    }

    if (keystate[SDL_SCANCODE_RIGHT])
    {
        if (player.x >= screenWidth - player.w)
            player.x = screenWidth - player.w;
        else
            player.x += speed;
    }
}

void render(SDL_Renderer* renderer)
{
    int green, red;
    if (isRunningCode)
    {
        green = 220;
        red = 80;
    }
    else
    {
        green = 80;
        red = 220;
    }

    SDL_Rect topBar = {0, 0, screenWidth, 40};

    if (isRunningCode)
        SDL_SetRenderDrawColor(renderer, 0, 180, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);

    SDL_RenderFillRect(renderer, &topBar);

    SDL_Rect r1 = {runButton.x, runButton.y, runButton.w, runButton.h};
    SDL_SetRenderDrawColor(renderer, 0, green, 0, 255);
    SDL_RenderFillRect(renderer, &r1);

    SDL_Rect r2 = {pauseButton.x, pauseButton.y, pauseButton.w, pauseButton.h};
    SDL_SetRenderDrawColor(renderer, red, 0, 0, 255);
    SDL_RenderFillRect(renderer, &r2);

    if (!player.visible) return;

    SDL_Rect rect;
    rect.x = (int)player.x;
    rect.y = (int)player.y;
    rect.w = player.w;
    rect.h = player.h;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &rect);
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        cout << "SDL Init Error: " << SDL_GetError() << endl;
        return -1;
    }

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    SDL_Window* window = SDL_CreateWindow(
            "FoP Project",
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

    player.w = 100;
    player.h = 100;
    player.x = double(dm.w) / 2 - 50;
    player.y = double(dm.h) / 2 - 50;
    player.visible = true;
    screenWidth = dm.w;
    screenHeight = dm.h;
    runButton = {50, screenHeight - 100, 120, 50};
    pauseButton = {200, screenHeight - 100, 120, 50};

    bool running = true;

    while (running)
    {
        handleEvents(running);
        update();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render(renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
