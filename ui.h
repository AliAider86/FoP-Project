#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "engine.h"

void renderText(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color);
void addCustomBackdrop(GameState& game, SDL_Renderer* renderer, const char* filePath);

void render(SDL_Renderer* renderer, GameState& game);
void handleEvents(bool& running, GameState& game, SDL_Renderer* renderer);

#endif