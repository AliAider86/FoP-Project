#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include "engine.h"

// اعلان تابع کشیدن متن
void renderText(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color);

void render(SDL_Renderer* renderer, GameState& game);
void handleEvents(bool& running, GameState& game, SDL_Renderer* renderer);

#endif