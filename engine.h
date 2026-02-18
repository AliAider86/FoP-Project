#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <SDL2/SDL.h>
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
    double direction;  // جهت بر حسب درجه (0=راست, 90=بالا, و...)

    string message;
    bool isThinking;

    // برای قلم
    bool penDown;
    int penSize;
    Uint8 penR, penG, penB;
    double lastPenX, lastPenY;
    bool penMoved;

    Sprite() : x(0), y(0), w(50), h(50), visible(true), direction(0),
               message(""), isThinking(false), penDown(false), penSize(2),
               penR(0), penG(0), penB(0), lastPenX(0), lastPenY(0), penMoved(false) {}
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

    vector<int> repeatCountStack;
    vector<int> repeatStartStack;

    // برای WAIT ساده
    Uint32 waitStartTime;
    Uint32 waitDuration;
    bool isWaiting;

    // برای SAY_FOR و THINK_FOR
    Uint32 messageStartTime;
    Uint32 messageDuration;
    bool isShowingMessage;

    // برای متغیرها
    unordered_map<string, Value> variables;

    // برای رویدادها
    bool greenFlagPressed;
    Uint8 pressedKeys[SDL_NUM_SCANCODES];
    bool spriteClicked;
    int mouseX, mouseY;
    bool mousePressed;

    // برای مدیریت چند اسکریپت
    vector<int> scriptStartIndices;
    vector<bool> scriptActive;
    vector<int> scriptCurrentBlock;

    // برای حسگری
    string askQuestion;
    string answer;
    bool waitingForAnswer;
    Uint32 timerStartTime;
    bool dragMode;

    // برای قلم - ذخیره خطوط رسم شده
    vector<int> penX1, penY1, penX2, penY2;
    vector<Uint8> penR_, penG_, penB_;
    vector<int> penSize_;

    // سازنده
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
            dragMode(false)
    {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
            pressedKeys[i] = 0;
        timerStartTime = SDL_GetTicks();
    }
};

void update(GameState& game);

#endif