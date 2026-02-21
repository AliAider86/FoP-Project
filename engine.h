#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "blocks.h"
#include "value.h"

using namespace std;

struct Button
{
    int x, y, w, h;
    bool isPressed;
};

struct Sprite
{
    double x, y;
    int w, h;
    bool visible;
    double direction;

    string message;
    string name;
    bool isThinking;

    bool penDown;
    int penSize;
    Uint8 penR, penG, penB;
    double lastPenX, lastPenY;
    bool penMoved;

    SDL_Texture* texture;
    string imagePath;
    int index;
    bool isActive;
};

struct GameState
{
    vector<Sprite> sprites;
    int activeSpriteIndex;

    vector<Block> program;

    int currentBlockIndex;

    bool isRunningCode;
    bool stepMode;

    double remainingMove;
    bool isExecutingBlock;

    int screenWidth;
    int screenHeight;

    Button runButton;
    Button pauseButton;
    Button stepButton;
    Button resetButton;
    Button saveButton;
    Button loadButton;

    Button addSpriteBtn;
    Button deleteSpriteBtn;
    Button prevSpriteBtn;
    Button nextSpriteBtn;

    Button moveCategoryBtn;
    Button looksCategoryBtn;
    Button soundCategoryBtn;
    Button eventsCategoryBtn;
    Button controlCategoryBtn;
    Button sensingCategoryBtn;
    Button operatorsCategoryBtn;
    Button variablesCategoryBtn;

    int currentCategory;

    vector<int> repeatCountStack;
    vector<int> repeatStartStack;

    Uint32 waitStartTime;
    Uint32 waitDuration;
    bool isWaiting;

    Uint32 messageStartTime;
    Uint32 messageDuration;
    bool isShowingMessage;

    unordered_map<string, Value> variables;

    bool greenFlagPressed;
    Uint8 pressedKeys[SDL_NUM_SCANCODES];
    Uint8 pressedThisFrame[SDL_NUM_SCANCODES];
    bool spriteClicked;
    int mouseX, mouseY;
    bool mousePressed;

    vector<int> scriptStartIndices;
    vector<bool> scriptActive;
    vector<int> scriptCurrentBlock;

    string askQuestion;
    string answer;
    bool waitingForAnswer;
    Uint32 timerStartTime;
    bool dragMode;

    vector<int> penX1, penY1, penX2, penY2;
    vector<Uint8> penR_, penG_, penB_;
    vector<int> penSize_;

    Block selectedBlock;
    bool placingBlock;

    bool editingMode;
    int editingField;
    string editingBuffer;
    bool showSpriteName;

    int volume;
    bool isPlayingSound;
    Mix_Chunk* soundEffect;
    int soundChannel;

    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;
    int clickedSpriteIndex;
    SDL_Texture* logoTexture;  // اضافه کن
};

// توابع اصلی
void update(GameState& game);
void saveProject(const GameState& game, const string& filename);
void loadProject(GameState& game, const string& filename);

// --- توابع مدیریت اسپرایت (اعلان‌ها) ---
Sprite* getActiveSprite(GameState& game);
void setActiveSprite(GameState& game, int index);
void addSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath);
void removeSprite(GameState& game, int index);

#endif