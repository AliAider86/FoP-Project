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

    Button() : x(0), y(0), w(0), h(0), isPressed(false) {}
    Button(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h), isPressed(false) {}
};

struct Sprite
{
    double x, y;
    int w, h;
    bool visible;
    double direction;

    string message;     // متن حباب (برای SAY/THINK)
    string name;        // اسم اسپرایت
    bool isThinking;

    // برای قلم
    bool penDown;
    int penSize;
    Uint8 penR, penG, penB;
    double lastPenX, lastPenY;
    bool penMoved;

    // برای تصویر اسپرایت
    SDL_Texture* texture;
    string imagePath;

    Sprite() : x(0), y(0), w(50), h(50), visible(true), direction(0),
               message(""), name("Sprite1"), isThinking(false), penDown(false), penSize(2),
               penR(0), penG(0), penB(0), lastPenX(0), lastPenY(0), penMoved(false),
               texture(nullptr), imagePath("") {}
};

struct GameState
{
    Sprite player;

    vector<Block> program;

    int currentBlockIndex = 0;

    bool isRunningCode = false;
    bool stepMode = false;

    double remainingMove = 0;
    bool isExecutingBlock = false;

    int screenWidth;
    int screenHeight;

    Button runButton;
    Button pauseButton;
    Button stepButton;
    Button resetButton;
    Button saveButton;
    Button loadButton;

    // دکمه‌های دسته‌بندی
    Button moveCategoryBtn;
    Button looksCategoryBtn;
    Button soundCategoryBtn;
    Button eventsCategoryBtn;
    Button controlCategoryBtn;
    Button sensingCategoryBtn;
    Button operatorsCategoryBtn;
    Button variablesCategoryBtn;

    // دسته فعلی
    int currentCategory;  // 0=حرکت, 1=ظاهر, 2=صدا, 3=رویدادها, 4=کنترل, 5=حسگر, 6=عملگرها, 7=متغیرها

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

    // برای عملی کردن بلوک‌ها
    Block selectedBlock;
    bool placingBlock;

    // برای ویرایش اسپرایت
    bool editingMode;           // آیا در حالت ویرایش هستیم؟
    int editingField;           // کدوم فیلد در حال ویرایشه (0=نام, 1=x, 2=y, 3=size, 4=direction)
    string editingBuffer;       // متن در حال ویرایش
    bool showSpriteName;        // نمایش نام اسپرایت بالای سرش

    // برای صدا
    int volume;
    bool isPlayingSound;
    Mix_Chunk* soundEffect;  // برای پخش صدا
    int soundChannel;

    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;

    GameState() :
            currentBlockIndex(0),
            isRunningCode(false),
            stepMode(false),
            remainingMove(0),
            isExecutingBlock(false),
            screenWidth(800),
            screenHeight(600),
            waitStartTime(0),
            waitDuration(0),
            isWaiting(false),
            messageStartTime(0),
            messageDuration(0),
            isShowingMessage(false),
            greenFlagPressed(false),
            spriteClicked(false),
            mouseX(0),
            mouseY(0),
            mousePressed(false),
            waitingForAnswer(false),
            timerStartTime(0),
            dragMode(false),
            currentCategory(0),
            placingBlock(false),
            editingMode(false),
            editingField(-1),
            showSpriteName(true),
            volume(100),
            isPlayingSound(false),
            soundEffect(nullptr),
            soundChannel(-1),
            isDragging(false),
            dragOffsetX(0),
            dragOffsetY(0)
    {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        {
            pressedKeys[i] = 0;
            pressedThisFrame[i] = 0;
        }
        timerStartTime = SDL_GetTicks();
    }
};

void update(GameState& game);
void saveProject(const GameState& game, const string& filename);
void loadProject(GameState& game, const string& filename);

#endif