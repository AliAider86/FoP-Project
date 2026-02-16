#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include "engine.h"

void render(SDL_Renderer* renderer, GameState& game);
void handleEvents(bool& running, GameState& game);

#endif
