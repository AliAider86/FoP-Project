#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <SDL2/SDL_image.h>
#include "engine.h"
#include "operators.h"
#include "logger.h"
#include <fstream>

using namespace std;

extern bool loadSpriteTexture(Sprite* sprite, SDL_Renderer* renderer, const char* path);

bool copyFileToFolder(const string& sourcePath, const string& destFolder)
{
    if (destFolder.empty()) return false;

    // استخراج نام فایل از مسیر اصلی
    size_t pos = sourcePath.find_last_of("/\\");
    string fileName = (pos != string::npos) ? sourcePath.substr(pos + 1) : sourcePath;

    // ساخت مسیر مقصد
    string destPath = destFolder + fileName;

    // کپی فایل به روش ساده
    ifstream src(sourcePath, ios::binary);
    ofstream dst(destPath, ios::binary);

    if (!src.is_open() || !dst.is_open())
        return false;

    dst << src.rdbuf();
    return true;
}

// ==================== توابع Sensing ====================

void resetTimer(GameState& game)
{
    game.timerStartTime = SDL_GetTicks();
}

double getTimerValue(GameState& game)
{
    if (game.timerStartTime == 0)
        return 0.0;
    return (SDL_GetTicks() - game.timerStartTime) / 1000.0;
}

bool isKeyPressed(GameState& game, int keyCode)
{
    return game.pressedKeys[keyCode] != 0;
}

bool isMouseDown(GameState& game)
{
    return game.mousePressed;
}

int getMouseX(GameState& game)
{
    return game.mouseX;
}

int getMouseY(GameState& game)
{
    return game.mouseY;
}

bool isTouchingEdge(Sprite* sprite, GameState& game)
{
    if (!sprite) return false;
    return (sprite->x <= 0 || sprite->x + sprite->w >= game.screenWidth ||
            sprite->y <= 0 || sprite->y + sprite->h >= game.screenHeight);
}

bool isTouchingMouse(Sprite* sprite, GameState& game)
{
    if (!sprite) return false;
    // تبدیل مختصات موس به فضای صفحه (موس در فضای پنجره است)
    // باید مختصات موس را نسبت به stage در نظر گرفت
    // فعلاً یک پیاده‌سازی ساده
    int stagePanelX = game.codeAreaX + game.codeAreaWidth + 10;
    int stagePanelWidth = game.screenWidth - stagePanelX - 5;
    if (stagePanelWidth > 500) stagePanelWidth = 500;
    int stageX = stagePanelX + 5;
    int stageY = 70;
    int stageWidth = stagePanelWidth - 20;
    int stageHeight = 300;

    // تبدیل مختصات اسپرایت به مختصات stage
    int spriteStageX = stageX + (int)((sprite->x / game.screenWidth) * stageWidth);
    int spriteStageY = stageY + (int)((sprite->y / game.screenHeight) * stageHeight);
    int spriteStageW = (int)((sprite->w / game.screenWidth) * stageWidth);
    int spriteStageH = (int)((sprite->h / game.screenHeight) * stageHeight);

    return (game.mouseX >= spriteStageX && game.mouseX <= spriteStageX + spriteStageW &&
            game.mouseY >= spriteStageY && game.mouseY <= spriteStageY + spriteStageH);
}

double getDistanceToMouse(Sprite* sprite, GameState& game)
{
    if (!sprite) return 0.0;
    int stagePanelX = game.codeAreaX + game.codeAreaWidth + 10;
    int stagePanelWidth = game.screenWidth - stagePanelX - 5;
    if (stagePanelWidth > 500) stagePanelWidth = 500;
    int stageX = stagePanelX + 5;
    int stageY = 70;
    int stageWidth = stagePanelWidth - 20;
    int stageHeight = 300;

    int spriteStageX = stageX + (int)((sprite->x + sprite->w/2) / game.screenWidth * stageWidth);
    int spriteStageY = stageY + (int)((sprite->y + sprite->h/2) / game.screenHeight * stageHeight);

    int dx = spriteStageX - game.mouseX;
    int dy = spriteStageY - game.mouseY;
    return sqrt(dx*dx + dy*dy);
}

void askQuestion(GameState& game, const string& question)
{
    game.currentQuestion = question;
    game.waitingForAnswer = true;
    game.answer = "";
    // در اینجا می‌توان یک باکس ورودی در رابط کاربری نمایش داد
    // اما فعلاً فقط لاگ می‌کنیم
    log_info(("Ask: " + question).c_str());
}

// ==================== توابع Events ====================

void broadcastMessage(GameState& game, const string& messageName, int senderScriptIndex)
{
    QueuedMessage msg;
    msg.messageName = messageName;
    msg.senderScriptIndex = senderScriptIndex;
    msg.broadcastTime = SDL_GetTicks();
    game.messageQueue.push(msg);
    log_info(("Broadcast: " + messageName).c_str());
}

void broadcastMessageAndWait(GameState& game, const string& messageName, int senderScriptIndex)
{
    broadcastMessage(game, messageName, senderScriptIndex);
    game.waitingForBroadcast = true;
    game.waitingScriptIndex = senderScriptIndex;
    game.broadcastWaitStartTime = SDL_GetTicks();
}

void processMessageQueue(GameState& game)
{
    if (game.messageQueue.empty()) return;

    while (!game.messageQueue.empty())
    {
        QueuedMessage msg = game.messageQueue.front();
        game.messageQueue.pop();

        if (game.messageHandlers.find(msg.messageName) != game.messageHandlers.end())
        {
            const vector<int>& handlerIndices = game.messageHandlers[msg.messageName];

            for (int handlerIdx : handlerIndices)
            {
                if (handlerIdx < game.program.size() &&
                    game.program[handlerIdx].type == WHEN_I_RECEIVE &&
                    game.program[handlerIdx].messageName == msg.messageName)
                {
                    game.scriptStartIndices.push_back(handlerIdx + 1);
                    game.scriptActive.push_back(true);
                    game.scriptCurrentBlock.push_back(handlerIdx + 1);

                    log_info(("Script started for message: " + msg.messageName).c_str());
                }
            }
        }
    }
}

// ==================== توابع Pen ====================

void updatePenColorFromHSV(GameState& game)
{
    double h = game.penHue;
    double s = game.penSaturation / 100.0;
    double v = game.penBrightness / 100.0;

    double r, g, b;
    int i = (int)(h / 60.0) % 6;
    double f = h / 60.0 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch (i) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    game.penR = (Uint8)(r * 255);
    game.penG = (Uint8)(g * 255);
    game.penB = (Uint8)(b * 255);
}

void penEraseAll(GameState& game)
{
    game.penPoints.clear();
    log_info("Pen: Erase all");
}

void penStamp(Sprite* sprite, GameState& game, SDL_Renderer* renderer)
{
    if (!sprite) return;

    int stageX = game.stageX + (int)((sprite->x + sprite->w/2) / game.screenWidth * game.stageW);
    int stageY = game.stageY + (int)((sprite->y + sprite->h/2) / game.screenHeight * game.stageH);

    PenPoint p;
    p.x = stageX;
    p.y = stageY;
    p.r = game.penR;
    p.g = game.penG;
    p.b = game.penB;
    p.a = 255;
    p.size = max(5, (sprite->w + sprite->h) / 10);
    game.penPoints.push_back(p);
    log_info(("Stamp at: " + to_string(p.x) + "," + to_string(p.y)).c_str());
}

void penSetSize(GameState& game, int size)
{
    if (size < 1) size = 1;
    game.penSize = size;
}

void penChangeSize(GameState& game, int delta)
{
    game.penSize += delta;
    if (game.penSize < 1) game.penSize = 1;
}

void penSetParam(GameState& game, PenColorParam param, double value)
{
    switch (param) {
        case PEN_PARAM_COLOR:
            game.penHue = fmod(value, 360.0);
            if (game.penHue < 0) game.penHue += 360.0;
            break;
        case PEN_PARAM_SATURATION:
            game.penSaturation = value;
            if (game.penSaturation < 0) game.penSaturation = 0;
            if (game.penSaturation > 100) game.penSaturation = 100;
            break;
        case PEN_PARAM_BRIGHTNESS:
            game.penBrightness = value;
            if (game.penBrightness < 0) game.penBrightness = 0;
            if (game.penBrightness > 100) game.penBrightness = 100;
            break;
    }
    updatePenColorFromHSV(game);
}

void penChangeParam(GameState& game, PenColorParam param, double delta)
{
    switch (param) {
        case PEN_PARAM_COLOR:
            game.penHue += delta;
            game.penHue = fmod(game.penHue, 360.0);
            if (game.penHue < 0) game.penHue += 360.0;
            break;
        case PEN_PARAM_SATURATION:
            game.penSaturation += delta;
            if (game.penSaturation < 0) game.penSaturation = 0;
            if (game.penSaturation > 100) game.penSaturation = 100;
            break;
        case PEN_PARAM_BRIGHTNESS:
            game.penBrightness += delta;
            if (game.penBrightness < 0) game.penBrightness = 0;
            if (game.penBrightness > 100) game.penBrightness = 100;
            break;
    }
    updatePenColorFromHSV(game);
}

// ==================== توابع Control ====================

Value evaluateCondition(Block& b, GameState& game)
{
    // این تابع باید بر اساس نوع بلوک و پارامترها، مقدار شرط را محاسبه کند
    // در این نسخه ساده، فرض می‌کنیم اولین پارامتر یک مقدار عددی است
    // و اگر != 0 باشد، شرط درست است
    if (!b.parameters.empty())
    {
        return Value(b.parameters[0].asBoolean());
    }
    return Value(false);
}

void preprocessControlBlocks(GameState& game)
{
    // پیمایش تمام بلوک‌ها و پیدا کردن جفت‌های IF/END_IF
    vector<int> ifStack;
    vector<int> elseIndices;

    for (int i = 0; i < game.program.size(); i++)
    {
        Block& b = game.program[i];

        if (b.type == IF_THEN || b.type == IF_THEN_ELSE)
        {
            ifStack.push_back(i);
            b.endIfIndex = -1; // فعلاً نامشخص
        }
        else if (b.type == ELSE)
        {
            elseIndices.push_back(i);
        }
        else if (b.type == END_IF)
        {
            if (!ifStack.empty())
            {
                int ifIndex = ifStack.back();
                ifStack.pop_back();

                // تنظیم آدرس END_IF برای IF
                game.program[ifIndex].endIfIndex = i;

                // اگر IF از نوع IF_THEN_ELSE باشد، باید ELSE مربوطه را هم پیدا کنیم
                if (game.program[ifIndex].type == IF_THEN_ELSE)
                {
                    // پیدا کردن ELSE بین ifIndex و i
                    for (int j = ifIndex + 1; j < i; j++)
                    {
                        if (game.program[j].type == ELSE)
                        {
                            game.program[ifIndex].ifFalseJump = j + 1; // پرش به بعد از ELSE
                            break;
                        }
                    }
                }
            }
        }
    }
}

// ==================== initPaletteBlocks ====================

void initPaletteBlocks(GameState& game)
{
    game.paletteBlocks.clear();

    int startY = 110;
    int blockWidth = 220;
    int blockHeight = 35;
    int xPos = 10;
    int paletteId = 0;

    // ===== بلوک‌های حرکتی (آبی) - CAT_MOTION =====
    // حرکت به بالا
    Block moveUpBlock(MOVE_UP, CAT_MOTION);
    moveUpBlock.eventName = "move up 10 steps";
    moveUpBlock.parameters.push_back(Value(10.0));
    moveUpBlock.x = xPos;
    moveUpBlock.y = startY;
    moveUpBlock.width = blockWidth;
    moveUpBlock.height = blockHeight;
    moveUpBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(moveUpBlock);

    // حرکت به پایین
    Block moveDownBlock(MOVE_DOWN, CAT_MOTION);
    moveDownBlock.eventName = "move down 10 steps";
    moveDownBlock.parameters.push_back(Value(10.0));
    moveDownBlock.x = xPos;
    moveDownBlock.y = startY + 45;
    moveDownBlock.width = blockWidth;
    moveDownBlock.height = blockHeight;
    moveDownBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(moveDownBlock);

    // حرکت به چپ
    Block moveLeftBlock(MOVE_LEFT, CAT_MOTION);
    moveLeftBlock.eventName = "move left 10 steps";
    moveLeftBlock.parameters.push_back(Value(10.0));
    moveLeftBlock.x = xPos;
    moveLeftBlock.y = startY + 90;
    moveLeftBlock.width = blockWidth;
    moveLeftBlock.height = blockHeight;
    moveLeftBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(moveLeftBlock);

    // حرکت به راست
    Block moveRightBlock(MOVE_RIGHT, CAT_MOTION);
    moveRightBlock.eventName = "move right 10 steps";
    moveRightBlock.parameters.push_back(Value(10.0));
    moveRightBlock.x = xPos;
    moveRightBlock.y = startY + 135;
    moveRightBlock.width = blockWidth;
    moveRightBlock.height = blockHeight;
    moveRightBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(moveRightBlock);

    // چرخش به راست
    Block turnRightBlock(TURN_RIGHT, CAT_MOTION);
    turnRightBlock.eventName = "turn right 15 degrees";
    turnRightBlock.parameters.push_back(Value(15.0));
    turnRightBlock.x = xPos;
    turnRightBlock.y = startY + 180;
    turnRightBlock.width = blockWidth;
    turnRightBlock.height = blockHeight;
    turnRightBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(turnRightBlock);

    // چرخش به چپ
    Block turnLeftBlock(TURN_LEFT, CAT_MOTION);
    turnLeftBlock.eventName = "turn left 15 degrees";
    turnLeftBlock.parameters.push_back(Value(15.0));
    turnLeftBlock.x = xPos;
    turnLeftBlock.y = startY + 225;
    turnLeftBlock.width = blockWidth;
    turnLeftBlock.height = blockHeight;
    turnLeftBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(turnLeftBlock);

    // رفتن به مختصات
    Block gotoXYBlock(GOTO_XY, CAT_MOTION);
    gotoXYBlock.eventName = "go to x: 100 y: 100";
    gotoXYBlock.parameters.push_back(Value(100.0));
    gotoXYBlock.parameters.push_back(Value(100.0));
    gotoXYBlock.x = xPos;
    gotoXYBlock.y = startY + 270;
    gotoXYBlock.width = blockWidth;
    gotoXYBlock.height = blockHeight;
    gotoXYBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(gotoXYBlock);

    // رفتن به موس
    Block gotoMouseBlock(GOTO_MOUSE, CAT_MOTION);
    gotoMouseBlock.eventName = "go to mouse-pointer";
    gotoMouseBlock.x = xPos;
    gotoMouseBlock.y = startY + 315;
    gotoMouseBlock.width = blockWidth;
    gotoMouseBlock.height = blockHeight;
    gotoMouseBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(gotoMouseBlock);

    // رفتن به موقعیت تصادفی
    Block gotoRandomBlock(GOTO_RANDOM, CAT_MOTION);
    gotoRandomBlock.eventName = "go to random position";
    gotoRandomBlock.parameters.push_back(Value(1.0));
    gotoRandomBlock.x = xPos;
    gotoRandomBlock.y = startY + 360;
    gotoRandomBlock.width = blockWidth;
    gotoRandomBlock.height = blockHeight;
    gotoRandomBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(gotoRandomBlock);

    // تغییر x
    Block changeXBlock(CHANGE_X, CAT_MOTION);
    changeXBlock.eventName = "change x by 10";
    changeXBlock.parameters.push_back(Value(10.0));
    changeXBlock.x = xPos;
    changeXBlock.y = startY + 405;
    changeXBlock.width = blockWidth;
    changeXBlock.height = blockHeight;
    changeXBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeXBlock);

    // تنظیم x
    Block setXBlock(SET_X, CAT_MOTION);
    setXBlock.eventName = "set x to 0";
    setXBlock.parameters.push_back(Value(0.0));
    setXBlock.x = xPos;
    setXBlock.y = startY + 450;
    setXBlock.width = blockWidth;
    setXBlock.height = blockHeight;
    setXBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setXBlock);

    // تغییر y
    Block changeYBlock(CHANGE_Y, CAT_MOTION);
    changeYBlock.eventName = "change y by 10";
    changeYBlock.parameters.push_back(Value(10.0));
    changeYBlock.x = xPos;
    changeYBlock.y = startY + 495;
    changeYBlock.width = blockWidth;
    changeYBlock.height = blockHeight;
    changeYBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeYBlock);

    // تنظیم y
    Block setYBlock(SET_Y, CAT_MOTION);
    setYBlock.eventName = "set y to 0";
    setYBlock.parameters.push_back(Value(0.0));
    setYBlock.x = xPos;
    setYBlock.y = startY + 540;
    setYBlock.width = blockWidth;
    setYBlock.height = blockHeight;
    setYBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setYBlock);

    Block goFrontBlock(GO_TO_FRONT_LAYER, CAT_LOOKS);
    goFrontBlock.eventName = "go to front layer";
    goFrontBlock.x = xPos;
    goFrontBlock.y = startY + 360;  // بعد از set size
    goFrontBlock.width = blockWidth;
    goFrontBlock.height = blockHeight;
    goFrontBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(goFrontBlock);

// Go to back layer
    Block goBackBlock(GO_TO_BACK_LAYER, CAT_LOOKS);
    goBackBlock.eventName = "go to back layer";
    goBackBlock.x = xPos;
    goBackBlock.y = startY + 405;
    goBackBlock.width = blockWidth;
    goBackBlock.height = blockHeight;
    goBackBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(goBackBlock);

// Go forward layers
    Block goForwardBlock(GO_FORWARD_LAYERS, CAT_LOOKS);
    goForwardBlock.eventName = "go forward 1 layers";
    goForwardBlock.parameters.push_back(Value(1.0));
    goForwardBlock.x = xPos;
    goForwardBlock.y = startY + 450;
    goForwardBlock.width = blockWidth;
    goForwardBlock.height = blockHeight;
    goForwardBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(goForwardBlock);

// Go backward layers
    Block goBackwardBlock(GO_BACKWARD_LAYERS, CAT_LOOKS);
    goBackwardBlock.eventName = "go backward 1 layers";
    goBackwardBlock.parameters.push_back(Value(1.0));
    goBackwardBlock.x = xPos;
    goBackwardBlock.y = startY + 495;
    goBackwardBlock.width = blockWidth;
    goBackwardBlock.height = blockHeight;
    goBackwardBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(goBackwardBlock);

    // ===== بلوک‌های ظاهری (نیلی) - CAT_LOOKS =====
    startY = 110;

    Block sayForBlock(SAY_FOR, CAT_LOOKS);
    sayForBlock.eventName = "say Hello! for 2 secs";
    sayForBlock.parameters.push_back(Value(string("Hello!")));
    sayForBlock.parameters.push_back(Value(2.0));
    sayForBlock.x = xPos;
    sayForBlock.y = startY;
    sayForBlock.width = blockWidth;
    sayForBlock.height = blockHeight;
    sayForBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(sayForBlock);

    Block sayBlock(SAY, CAT_LOOKS);
    sayBlock.eventName = "say Hello!";
    sayBlock.parameters.push_back(Value(string("Hello!")));
    sayBlock.x = xPos;
    sayBlock.y = startY + 45;
    sayBlock.width = blockWidth;
    sayBlock.height = blockHeight;
    sayBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(sayBlock);

    Block thinkForBlock(THINK_FOR, CAT_LOOKS);
    thinkForBlock.eventName = "think Hmm... for 2 secs";
    thinkForBlock.parameters.push_back(Value(string("Hmm...")));
    thinkForBlock.parameters.push_back(Value(2.0));
    thinkForBlock.x = xPos;
    thinkForBlock.y = startY + 90;
    thinkForBlock.width = blockWidth;
    thinkForBlock.height = blockHeight;
    thinkForBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(thinkForBlock);

    Block thinkBlock(THINK, CAT_LOOKS);
    thinkBlock.eventName = "think Hmm...";
    thinkBlock.parameters.push_back(Value(string("Hmm...")));
    thinkBlock.x = xPos;
    thinkBlock.y = startY + 135;
    thinkBlock.width = blockWidth;
    thinkBlock.height = blockHeight;
    thinkBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(thinkBlock);

    Block showBlock(SHOW, CAT_LOOKS);
    showBlock.eventName = "show";
    showBlock.x = xPos;
    showBlock.y = startY + 180;
    showBlock.width = blockWidth;
    showBlock.height = blockHeight;
    showBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(showBlock);

    Block hideBlock(HIDE, CAT_LOOKS);
    hideBlock.eventName = "hide";
    hideBlock.x = xPos;
    hideBlock.y = startY + 225;
    hideBlock.width = blockWidth;
    hideBlock.height = blockHeight;
    hideBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(hideBlock);

    Block changeSizeBlock(CHANGE_SIZE, CAT_LOOKS);
    changeSizeBlock.eventName = "change size by 10";
    changeSizeBlock.parameters.push_back(Value(10.0));
    changeSizeBlock.x = xPos;
    changeSizeBlock.y = startY + 270;
    changeSizeBlock.width = blockWidth;
    changeSizeBlock.height = blockHeight;
    changeSizeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeSizeBlock);

    Block setSizeBlock(SET_SIZE, CAT_LOOKS);
    setSizeBlock.eventName = "set size to 100 %";
    setSizeBlock.parameters.push_back(Value(100.0));
    setSizeBlock.x = xPos;
    setSizeBlock.y = startY + 315;
    setSizeBlock.width = blockWidth;
    setSizeBlock.height = blockHeight;
    setSizeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setSizeBlock);

    // ===== بلوک‌های صدا (بنفش) - CAT_SOUND =====
    startY = 110;

    Block playSoundBlock(PLAY_SOUND, CAT_SOUND);
    playSoundBlock.eventName = "play sound Meow";
    playSoundBlock.parameters.push_back(Value(string("Meow")));
    playSoundBlock.x = xPos;
    playSoundBlock.y = startY;
    playSoundBlock.width = blockWidth;
    playSoundBlock.height = blockHeight;
    playSoundBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(playSoundBlock);

    Block playSoundUntilBlock(PLAY_SOUND_UNTIL_DONE, CAT_SOUND);
    playSoundUntilBlock.eventName = "play sound Meow until done";
    playSoundUntilBlock.parameters.push_back(Value(string("Meow")));
    playSoundUntilBlock.x = xPos;
    playSoundUntilBlock.y = startY + 45;
    playSoundUntilBlock.width = blockWidth;
    playSoundUntilBlock.height = blockHeight;
    playSoundUntilBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(playSoundUntilBlock);

    Block stopSoundBlock(STOP_ALL_SOUNDS, CAT_SOUND);
    stopSoundBlock.eventName = "stop all sounds";
    stopSoundBlock.x = xPos;
    stopSoundBlock.y = startY + 90;
    stopSoundBlock.width = blockWidth;
    stopSoundBlock.height = blockHeight;
    stopSoundBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(stopSoundBlock);

    Block changeVolumeBlock(CHANGE_VOLUME, CAT_SOUND);
    changeVolumeBlock.eventName = "change volume by 10";
    changeVolumeBlock.parameters.push_back(Value(10.0));
    changeVolumeBlock.x = xPos;
    changeVolumeBlock.y = startY + 135;
    changeVolumeBlock.width = blockWidth;
    changeVolumeBlock.height = blockHeight;
    changeVolumeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeVolumeBlock);

    Block setVolumeBlock(SET_VOLUME, CAT_SOUND);
    setVolumeBlock.eventName = "set volume to 100 %";
    setVolumeBlock.parameters.push_back(Value(100.0));
    setVolumeBlock.x = xPos;
    setVolumeBlock.y = startY + 180;
    setVolumeBlock.width = blockWidth;
    setVolumeBlock.height = blockHeight;
    setVolumeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setVolumeBlock);

    // ===== بلوک‌های رویداد (زرد) - CAT_EVENTS =====
    startY = 110;

    Block whenFlagBlock(WHEN_GREEN_FLAG, CAT_EVENTS);
    whenFlagBlock.eventName = "when flag clicked";
    whenFlagBlock.x = xPos;
    whenFlagBlock.y = startY;
    whenFlagBlock.width = blockWidth;
    whenFlagBlock.height = blockHeight;
    whenFlagBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(whenFlagBlock);

    Block whenKeyBlock(WHEN_KEY_PRESSED, CAT_EVENTS);
    whenKeyBlock.eventName = "when key pressed";
    whenKeyBlock.keyCode = SDL_SCANCODE_SPACE;
    whenKeyBlock.x = xPos;
    whenKeyBlock.y = startY + 45;
    whenKeyBlock.width = blockWidth;
    whenKeyBlock.height = blockHeight;
    whenKeyBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(whenKeyBlock);

    Block whenSpriteBlock(WHEN_SPRITE_CLICKED, CAT_EVENTS);
    whenSpriteBlock.eventName = "when sprite clicked";
    whenSpriteBlock.x = xPos;
    whenSpriteBlock.y = startY + 90;
    whenSpriteBlock.width = blockWidth;
    whenSpriteBlock.height = blockHeight;
    whenSpriteBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(whenSpriteBlock);

    Block whenReceiveBlock(WHEN_I_RECEIVE, CAT_EVENTS);
    whenReceiveBlock.eventName = "when I receive message1";
    whenReceiveBlock.messageName = "message1";
    whenReceiveBlock.x = xPos;
    whenReceiveBlock.y = startY + 135;
    whenReceiveBlock.width = blockWidth;
    whenReceiveBlock.height = blockHeight;
    whenReceiveBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(whenReceiveBlock);

    Block broadcastBlock(BROADCAST, CAT_EVENTS);
    broadcastBlock.eventName = "broadcast message1";
    broadcastBlock.messageName = "message1";
    broadcastBlock.x = xPos;
    broadcastBlock.y = startY + 180;
    broadcastBlock.width = blockWidth;
    broadcastBlock.height = blockHeight;
    broadcastBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(broadcastBlock);

    Block broadcastWaitBlock(BROADCAST_AND_WAIT, CAT_EVENTS);
    broadcastWaitBlock.eventName = "broadcast message1 and wait";
    broadcastWaitBlock.messageName = "message1";
    broadcastWaitBlock.x = xPos;
    broadcastWaitBlock.y = startY + 225;
    broadcastWaitBlock.width = blockWidth;
    broadcastWaitBlock.height = blockHeight;
    broadcastWaitBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(broadcastWaitBlock);

    // ===== بلوک‌های کنترل (نارنجی) - CAT_CONTROL (کامل شده) =====
    startY = 110;

    // Wait
    Block waitBlock(WAIT, CAT_CONTROL);
    waitBlock.eventName = "wait 1 seconds";
    waitBlock.parameters.push_back(Value(1.0));
    waitBlock.x = xPos;
    waitBlock.y = startY;
    waitBlock.width = blockWidth;
    waitBlock.height = blockHeight;
    waitBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(waitBlock);

    // Repeat
    Block repeatBlock(REPEAT, CAT_CONTROL);
    repeatBlock.eventName = "repeat 10";
    repeatBlock.repeatCount = 10;
    repeatBlock.x = xPos;
    repeatBlock.y = startY + 45;
    repeatBlock.width = blockWidth;
    repeatBlock.height = blockHeight;
    repeatBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(repeatBlock);

    // Forever
    Block foreverBlock(FOREVER, CAT_CONTROL);
    foreverBlock.eventName = "forever";
    foreverBlock.x = xPos;
    foreverBlock.y = startY + 90;
    foreverBlock.width = blockWidth;
    foreverBlock.height = blockHeight;
    foreverBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(foreverBlock);

    // If Then
    Block ifThenBlock(IF_THEN, CAT_CONTROL);
    ifThenBlock.eventName = "if <condition> then";
    ifThenBlock.parameters.push_back(Value(true)); // شرط پیش‌فرض
    ifThenBlock.x = xPos;
    ifThenBlock.y = startY + 135;
    ifThenBlock.width = blockWidth;
    ifThenBlock.height = blockHeight;
    ifThenBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(ifThenBlock);

    // If Then Else
    Block ifThenElseBlock(IF_THEN_ELSE, CAT_CONTROL);
    ifThenElseBlock.eventName = "if <condition> then else";
    ifThenElseBlock.parameters.push_back(Value(true));
    ifThenElseBlock.x = xPos;
    ifThenElseBlock.y = startY + 180;
    ifThenElseBlock.width = blockWidth;
    ifThenElseBlock.height = blockHeight;
    ifThenElseBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(ifThenElseBlock);

    // End If
    Block endIfBlock(END_IF, CAT_CONTROL);
    endIfBlock.eventName = "end if";
    endIfBlock.x = xPos;
    endIfBlock.y = startY + 225;
    endIfBlock.width = blockWidth;
    endIfBlock.height = blockHeight;
    endIfBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(endIfBlock);

    // Wait Until
    Block waitUntilBlock(WAIT_UNTIL, CAT_CONTROL);
    waitUntilBlock.eventName = "wait until <condition>";
    waitUntilBlock.parameters.push_back(Value(true));
    waitUntilBlock.x = xPos;
    waitUntilBlock.y = startY + 270;
    waitUntilBlock.width = blockWidth;
    waitUntilBlock.height = blockHeight;
    waitUntilBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(waitUntilBlock);

    // Repeat Until
    Block repeatUntilBlock(REPEAT_UNTIL, CAT_CONTROL);
    repeatUntilBlock.eventName = "repeat until <condition>";
    repeatUntilBlock.parameters.push_back(Value(false));
    repeatUntilBlock.x = xPos;
    repeatUntilBlock.y = startY + 315;
    repeatUntilBlock.width = blockWidth;
    repeatUntilBlock.height = blockHeight;
    repeatUntilBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(repeatUntilBlock);

    // Stop All
    Block stopAllBlock(STOP_ALL, CAT_CONTROL);
    stopAllBlock.eventName = "stop all";
    stopAllBlock.x = xPos;
    stopAllBlock.y = startY + 360;
    stopAllBlock.width = blockWidth;
    stopAllBlock.height = blockHeight;
    stopAllBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(stopAllBlock);

    // Stop This Script
    Block stopThisScriptBlock(STOP_THIS_SCRIPT, CAT_CONTROL);
    stopThisScriptBlock.eventName = "stop this script";
    stopThisScriptBlock.x = xPos;
    stopThisScriptBlock.y = startY + 405;
    stopThisScriptBlock.width = blockWidth;
    stopThisScriptBlock.height = blockHeight;
    stopThisScriptBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(stopThisScriptBlock);

    // End Repeat
    Block endRepeatBlock(END_REPEAT, CAT_CONTROL);
    endRepeatBlock.eventName = "end repeat";
    endRepeatBlock.x = xPos;
    endRepeatBlock.y = startY + 495; // تنظیم موقعیت مناسب
    endRepeatBlock.width = blockWidth;
    endRepeatBlock.height = blockHeight;
    endRepeatBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(endRepeatBlock);

// End Forever
    Block endForeverBlock(END_FOREVER, CAT_CONTROL);
    endForeverBlock.eventName = "end forever";
    endForeverBlock.x = xPos;
    endForeverBlock.y = startY + 540;
    endForeverBlock.width = blockWidth;
    endForeverBlock.height = blockHeight;
    endForeverBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(endForeverBlock);

    // ===== بلوک‌های حسگری (Sensing) - CAT_SENSING =====
    startY = 110;

    // Reset Timer
    Block resetTimerBlock(SENSOR_RESET_TIMER, CAT_SENSING);
    resetTimerBlock.eventName = "reset timer";
    resetTimerBlock.x = xPos;
    resetTimerBlock.y = startY;
    resetTimerBlock.width = blockWidth;
    resetTimerBlock.height = blockHeight;
    resetTimerBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(resetTimerBlock);

    // Ask and Wait
    Block askBlock(SENSOR_ASK_AND_WAIT, CAT_SENSING);
    askBlock.eventName = "ask What's your name? and wait";
    askBlock.parameters.push_back(Value(string("What's your name?")));
    askBlock.x = xPos;
    askBlock.y = startY + 45;
    askBlock.width = blockWidth;
    askBlock.height = blockHeight;
    askBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(askBlock);

    // Set variable to sensor (touching edge)
    Block setToTouchingEdge(SET_VARIABLE_TO_SENSOR, CAT_SENSING);
    setToTouchingEdge.eventName = "set myVar to touching edge?";
    setToTouchingEdge.variableName = "myVar";
    setToTouchingEdge.sensorType = SENSOR_TYPE_TOUCHING_EDGE;
    setToTouchingEdge.x = xPos;
    setToTouchingEdge.y = startY + 90;
    setToTouchingEdge.width = blockWidth;
    setToTouchingEdge.height = blockHeight;
    setToTouchingEdge.paletteId = paletteId++;
    game.paletteBlocks.push_back(setToTouchingEdge);

    // Set variable to sensor (mouse x)
    Block setToMouseX(SET_VARIABLE_TO_SENSOR, CAT_SENSING);
    setToMouseX.eventName = "set myVar to mouse x";
    setToMouseX.variableName = "myVar";
    setToMouseX.sensorType = SENSOR_TYPE_MOUSE_X;
    setToMouseX.x = xPos;
    setToMouseX.y = startY + 135;
    setToMouseX.width = blockWidth;
    setToMouseX.height = blockHeight;
    setToMouseX.paletteId = paletteId++;
    game.paletteBlocks.push_back(setToMouseX);

    // Set variable to sensor (timer)
    Block setToTimer(SET_VARIABLE_TO_SENSOR, CAT_SENSING);
    setToTimer.eventName = "set myVar to timer";
    setToTimer.variableName = "myVar";
    setToTimer.sensorType = SENSOR_TYPE_TIMER;
    setToTimer.x = xPos;
    setToTimer.y = startY + 180;
    setToTimer.width = blockWidth;
    setToTimer.height = blockHeight;
    setToTimer.paletteId = paletteId++;
    game.paletteBlocks.push_back(setToTimer);

    // Set variable to sensor (key pressed)
    Block setToKeyPressed(SET_VARIABLE_TO_SENSOR, CAT_SENSING);
    setToKeyPressed.eventName = "set myVar to key space pressed?";
    setToKeyPressed.variableName = "myVar";
    setToKeyPressed.sensorType = SENSOR_TYPE_KEY_PRESSED;
    setToKeyPressed.keyCode = SDL_SCANCODE_SPACE;
    setToKeyPressed.x = xPos;
    setToKeyPressed.y = startY + 225;
    setToKeyPressed.width = blockWidth;
    setToKeyPressed.height = blockHeight;
    setToKeyPressed.paletteId = paletteId++;
    game.paletteBlocks.push_back(setToKeyPressed);

    // ===== بلوک‌های عملگر (سبز) - CAT_OPERATORS =====
    startY = 110;

    Block addBlock(OP_ADD, CAT_OPERATORS);
    addBlock.eventName = "0 + 0";
    addBlock.parameters.push_back(Value(0.0));
    addBlock.parameters.push_back(Value(0.0));
    addBlock.x = xPos;
    addBlock.y = startY;
    addBlock.width = blockWidth;
    addBlock.height = blockHeight;
    addBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(addBlock);

    Block setCompBlock(SET_COMPARISON, CAT_OPERATORS);
    setCompBlock.eventName = "set result to compare (left > right)";
    setCompBlock.variableName = "result";
    setCompBlock.parameters.push_back(Value(0.0));
    setCompBlock.parameters.push_back(Value(0.0));
    setCompBlock.compareOp = CMP_GREATER;
    setCompBlock.leftVar = "";
    setCompBlock.rightVar = "";
    setCompBlock.x = xPos;
    setCompBlock.y = startY + 585; // بعد از سایر بلوک‌های عملگر
    setCompBlock.width = blockWidth;
    setCompBlock.height = blockHeight;
    setCompBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setCompBlock);

    Block subtractBlock(OP_SUBTRACT, CAT_OPERATORS);
    subtractBlock.eventName = "0 - 0";
    subtractBlock.parameters.push_back(Value(0.0));
    subtractBlock.parameters.push_back(Value(0.0));
    subtractBlock.x = xPos;
    subtractBlock.y = startY + 45;
    subtractBlock.width = blockWidth;
    subtractBlock.height = blockHeight;
    subtractBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(subtractBlock);

    Block multiplyBlock(OP_MULTIPLY, CAT_OPERATORS);
    multiplyBlock.eventName = "0 * 0";
    multiplyBlock.parameters.push_back(Value(0.0));
    multiplyBlock.parameters.push_back(Value(0.0));
    multiplyBlock.x = xPos;
    multiplyBlock.y = startY + 90;
    multiplyBlock.width = blockWidth;
    multiplyBlock.height = blockHeight;
    multiplyBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(multiplyBlock);

    Block divideBlock(OP_DIVIDE, CAT_OPERATORS);
    divideBlock.eventName = "0 / 0";
    divideBlock.parameters.push_back(Value(0.0));
    divideBlock.parameters.push_back(Value(0.0));
    divideBlock.x = xPos;
    divideBlock.y = startY + 135;
    divideBlock.width = blockWidth;
    divideBlock.height = blockHeight;
    divideBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(divideBlock);

    Block equalBlock(OP_EQUAL, CAT_OPERATORS);
    equalBlock.eventName = "0 = 0";
    equalBlock.parameters.push_back(Value(0.0));
    equalBlock.parameters.push_back(Value(0.0));
    equalBlock.x = xPos;
    equalBlock.y = startY + 180;
    equalBlock.width = blockWidth;
    equalBlock.height = blockHeight;
    equalBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(equalBlock);

    Block lessBlock(OP_LESS_THAN, CAT_OPERATORS);
    lessBlock.eventName = "0 < 0";
    lessBlock.parameters.push_back(Value(0.0));
    lessBlock.parameters.push_back(Value(0.0));
    lessBlock.x = xPos;
    lessBlock.y = startY + 225;
    lessBlock.width = blockWidth;
    lessBlock.height = blockHeight;
    lessBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(lessBlock);

    Block greaterBlock(OP_GREATER_THAN, CAT_OPERATORS);
    greaterBlock.eventName = "0 > 0";
    greaterBlock.parameters.push_back(Value(0.0));
    greaterBlock.parameters.push_back(Value(0.0));
    greaterBlock.x = xPos;
    greaterBlock.y = startY + 270;
    greaterBlock.width = blockWidth;
    greaterBlock.height = blockHeight;
    greaterBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(greaterBlock);

    Block notBlock(OP_NOT, CAT_OPERATORS);
    notBlock.eventName = "not";
    notBlock.parameters.push_back(Value(false));
    notBlock.x = xPos;
    notBlock.y = startY + 315;
    notBlock.width = blockWidth;
    notBlock.height = blockHeight;
    notBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(notBlock);

    Block orBlock(OP_OR, CAT_OPERATORS);
    orBlock.eventName = "or";
    orBlock.parameters.push_back(Value(false));
    orBlock.parameters.push_back(Value(false));
    orBlock.x = xPos;
    orBlock.y = startY + 360;
    orBlock.width = blockWidth;
    orBlock.height = blockHeight;
    orBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(orBlock);

    Block andBlock(OP_AND, CAT_OPERATORS);
    andBlock.eventName = "and";
    andBlock.parameters.push_back(Value(false));
    andBlock.parameters.push_back(Value(false));
    andBlock.x = xPos;
    andBlock.y = startY + 405;
    andBlock.width = blockWidth;
    andBlock.height = blockHeight;
    andBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(andBlock);

    Block joinStringsBlock(OP_JOIN_STRINGS, CAT_OPERATORS);
    joinStringsBlock.eventName = "join hello world";
    joinStringsBlock.parameters.push_back(Value(string("hello")));
    joinStringsBlock.parameters.push_back(Value(string("world")));
    joinStringsBlock.x = xPos;
    joinStringsBlock.y = startY + 450;
    joinStringsBlock.width = blockWidth;
    joinStringsBlock.height = blockHeight;
    joinStringsBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(joinStringsBlock);

    Block stringLengthBlock(OP_STRING_LENGTH, CAT_OPERATORS);
    stringLengthBlock.eventName = "length of world";
    stringLengthBlock.parameters.push_back(Value(string("world")));
    stringLengthBlock.x = xPos;
    stringLengthBlock.y = startY + 495;
    stringLengthBlock.width = blockWidth;
    stringLengthBlock.height = blockHeight;
    stringLengthBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(stringLengthBlock);

    Block letterOfBlock(OP_LETTER_OF, CAT_OPERATORS);
    letterOfBlock.eventName = "letter 1 of world";
    letterOfBlock.parameters.push_back(Value(1.0));
    letterOfBlock.parameters.push_back(Value(string("world")));
    letterOfBlock.x = xPos;
    letterOfBlock.y = startY + 540;
    letterOfBlock.width = blockWidth;
    letterOfBlock.height = blockHeight;
    letterOfBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(letterOfBlock);

    // ===== بلوک‌های متغیر (نارنجی پررنگ) - CAT_VARIABLES =====
    startY = 110;

    Block setVarBlock(SET_VARIABLE, CAT_VARIABLES);
    setVarBlock.eventName = "set my variable to 0";
    setVarBlock.variableName = "my variable";
    setVarBlock.parameters.push_back(Value(0.0));
    setVarBlock.x = xPos;
    setVarBlock.y = startY;
    setVarBlock.width = blockWidth;
    setVarBlock.height = blockHeight;
    setVarBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setVarBlock);

    Block changeVarBlock(CHANGE_VARIABLE, CAT_VARIABLES);
    changeVarBlock.eventName = "change my variable by 1";
    changeVarBlock.variableName = "my variable";
    changeVarBlock.parameters.push_back(Value(1.0));
    changeVarBlock.x = xPos;
    changeVarBlock.y = startY + 45;
    changeVarBlock.width = blockWidth;
    changeVarBlock.height = blockHeight;
    changeVarBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeVarBlock);

    Block showVarBlock(SHOW_VARIABLE, CAT_VARIABLES);
    showVarBlock.eventName = "show variable my variable";
    showVarBlock.variableName = "my variable";
    showVarBlock.x = xPos;
    showVarBlock.y = startY + 90;
    showVarBlock.width = blockWidth;
    showVarBlock.height = blockHeight;
    showVarBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(showVarBlock);

    Block hideVarBlock(HIDE_VARIABLE, CAT_VARIABLES);
    hideVarBlock.eventName = "hide variable my variable";
    hideVarBlock.variableName = "my variable";
    hideVarBlock.x = xPos;
    hideVarBlock.y = startY + 135;
    hideVarBlock.width = blockWidth;
    hideVarBlock.height = blockHeight;
    hideVarBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(hideVarBlock);

    // Costume number
    Block costumeNumBlock(COSTUME_NUMBER, CAT_LOOKS);
    costumeNumBlock.eventName = "set myVar to costume number";
    costumeNumBlock.variableName = "myVar";
    costumeNumBlock.x = xPos;
    costumeNumBlock.y = startY + 360; // بعد از set size
    costumeNumBlock.width = blockWidth;
    costumeNumBlock.height = blockHeight;
    costumeNumBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(costumeNumBlock);

// Sprite size
    Block spriteSizeBlock(SPRITE_SIZE, CAT_LOOKS);
    spriteSizeBlock.eventName = "set myVar to size";
    spriteSizeBlock.variableName = "myVar";
    spriteSizeBlock.x = xPos;
    spriteSizeBlock.y = startY + 405;
    spriteSizeBlock.width = blockWidth;
    spriteSizeBlock.height = blockHeight;
    spriteSizeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(spriteSizeBlock);

// Backdrop number
    Block backdropNumBlock(BACKDROP_NUMBER, CAT_LOOKS);
    backdropNumBlock.eventName = "set myVar to backdrop number";
    backdropNumBlock.variableName = "myVar";
    backdropNumBlock.x = xPos;
    backdropNumBlock.y = startY + 450;
    backdropNumBlock.width = blockWidth;
    backdropNumBlock.height = blockHeight;
    backdropNumBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(backdropNumBlock);

    // ===== بلوک‌های ترسیمی (Pen) - CAT_PEN =====
    startY = 110;

    // Erase All
    Block eraseAllBlock(PEN_ERASE_ALL, CAT_PEN);
    eraseAllBlock.eventName = "erase all";
    eraseAllBlock.x = xPos;
    eraseAllBlock.y = startY;
    eraseAllBlock.width = blockWidth;
    eraseAllBlock.height = blockHeight;
    eraseAllBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(eraseAllBlock);

    // Stamp
    Block stampBlock(PEN_STAMP, CAT_PEN);
    stampBlock.eventName = "stamp";
    stampBlock.x = xPos;
    stampBlock.y = startY + 45;
    stampBlock.width = blockWidth;
    stampBlock.height = blockHeight;
    stampBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(stampBlock);

    // Pen Down
    Block penDownBlock(PEN_PEN_DOWN, CAT_PEN);
    penDownBlock.eventName = "pen down";
    penDownBlock.x = xPos;
    penDownBlock.y = startY + 90;
    penDownBlock.width = blockWidth;
    penDownBlock.height = blockHeight;
    penDownBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(penDownBlock);

    // Pen Up
    Block penUpBlock(PEN_PEN_UP, CAT_PEN);
    penUpBlock.eventName = "pen up";
    penUpBlock.x = xPos;
    penUpBlock.y = startY + 135;
    penUpBlock.width = blockWidth;
    penUpBlock.height = blockHeight;
    penUpBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(penUpBlock);

    // Set Pen Color
    Block setColorBlock(PEN_SET_COLOR_PARAM, CAT_PEN);
    setColorBlock.eventName = "set pen color to 0";
    setColorBlock.parameters.push_back(Value(0.0));
    setColorBlock.penParam = PEN_PARAM_COLOR;
    setColorBlock.x = xPos;
    setColorBlock.y = startY + 180;
    setColorBlock.width = blockWidth;
    setColorBlock.height = blockHeight;
    setColorBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setColorBlock);

    // Change Pen Color by
    Block changeColorBlock(PEN_CHANGE_COLOR_PARAM, CAT_PEN);
    changeColorBlock.eventName = "change pen color by 10";
    changeColorBlock.parameters.push_back(Value(10.0));
    changeColorBlock.penParam = PEN_PARAM_COLOR;
    changeColorBlock.x = xPos;
    changeColorBlock.y = startY + 225;
    changeColorBlock.width = blockWidth;
    changeColorBlock.height = blockHeight;
    changeColorBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeColorBlock);

    // Set Pen Saturation
    Block setSaturationBlock(PEN_SET_COLOR_PARAM, CAT_PEN);
    setSaturationBlock.eventName = "set pen saturation to 100";
    setSaturationBlock.parameters.push_back(Value(100.0));
    setSaturationBlock.penParam = PEN_PARAM_SATURATION;
    setSaturationBlock.x = xPos;
    setSaturationBlock.y = startY + 270;
    setSaturationBlock.width = blockWidth;
    setSaturationBlock.height = blockHeight;
    setSaturationBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setSaturationBlock);

    // Change Pen Saturation by
    Block changeSaturationBlock(PEN_CHANGE_COLOR_PARAM, CAT_PEN);
    changeSaturationBlock.eventName = "change pen saturation by 10";
    changeSaturationBlock.parameters.push_back(Value(10.0));
    changeSaturationBlock.penParam = PEN_PARAM_SATURATION;
    changeSaturationBlock.x = xPos;
    changeSaturationBlock.y = startY + 315;
    changeSaturationBlock.width = blockWidth;
    changeSaturationBlock.height = blockHeight;
    changeSaturationBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeSaturationBlock);

    // Set Pen Brightness
    Block setBrightnessBlock(PEN_SET_COLOR_PARAM, CAT_PEN);
    setBrightnessBlock.eventName = "set pen brightness to 100";
    setBrightnessBlock.parameters.push_back(Value(100.0));
    setBrightnessBlock.penParam = PEN_PARAM_BRIGHTNESS;
    setBrightnessBlock.x = xPos;
    setBrightnessBlock.y = startY + 360;
    setBrightnessBlock.width = blockWidth;
    setBrightnessBlock.height = blockHeight;
    setBrightnessBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setBrightnessBlock);

    // Change Pen Brightness by
    Block changeBrightnessBlock(PEN_CHANGE_COLOR_PARAM, CAT_PEN);
    changeBrightnessBlock.eventName = "change pen brightness by 10";
    changeBrightnessBlock.parameters.push_back(Value(10.0));
    changeBrightnessBlock.penParam = PEN_PARAM_BRIGHTNESS;
    changeBrightnessBlock.x = xPos;
    changeBrightnessBlock.y = startY + 405;
    changeBrightnessBlock.width = blockWidth;
    changeBrightnessBlock.height = blockHeight;
    changeBrightnessBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changeBrightnessBlock);

    // Set Pen Size
    Block setPenSizeBlock(PEN_SET_SIZE, CAT_PEN);
    setPenSizeBlock.eventName = "set pen size to 1";
    setPenSizeBlock.parameters.push_back(Value(1.0));
    setPenSizeBlock.x = xPos;
    setPenSizeBlock.y = startY + 450;
    setPenSizeBlock.width = blockWidth;
    setPenSizeBlock.height = blockHeight;
    setPenSizeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(setPenSizeBlock);

    // Change Pen Size by
    Block changePenSizeBlock(PEN_CHANGE_SIZE, CAT_PEN);
    changePenSizeBlock.eventName = "change pen size by 1";
    changePenSizeBlock.parameters.push_back(Value(1.0));
    changePenSizeBlock.x = xPos;
    changePenSizeBlock.y = startY + 495;
    changePenSizeBlock.width = blockWidth;
    changePenSizeBlock.height = blockHeight;
    changePenSizeBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(changePenSizeBlock);

    log_info(("Palette blocks initialized: " + to_string(game.paletteBlocks.size()) + " blocks").c_str());
}

// ==================== بقیه توابع ====================

Block* getBlockAtPosition(GameState& game, int x, int y)
{
    // اول بلوک‌های برنامه رو بررسی کن (از آخر به اول برای اولویت)
    for (int i = game.program.size() - 1; i >= 0; i--)
    {
        Block& block = game.program[i];
        if (x >= block.x && x <= block.x + block.width &&
            y >= block.y && y <= block.y + block.height)
        {
            return &game.program[i];
        }
    }

    // بعد بلوک‌های پالت رو بررسی کن
    for (int i = game.paletteBlocks.size() - 1; i >= 0; i--)
    {
        Block& block = game.paletteBlocks[i];
        if (x >= block.x && x <= block.x + block.width &&
            y >= block.y && y <= block.y + block.height)
        {
            return &game.paletteBlocks[i];
        }
    }

    return nullptr;
}

void snapBlockToParent(Block* child, Block* parent)
{
    if (!child || !parent) return;

    if (child->parent)
    {
        auto& siblings = child->parent->children;
        siblings.erase(remove(siblings.begin(), siblings.end(), child), siblings.end());
    }

    child->parent = parent;
    parent->children.push_back(child);

    child->x = parent->x;
    child->y = parent->y + parent->height;
}

void detachBlock(Block* block)
{
    if (!block || !block->parent) return;

    auto& siblings = block->parent->children;
    siblings.erase(remove(siblings.begin(), siblings.end(), block), siblings.end());
    block->parent = nullptr;
}

void addCustomBackdrop(GameState& game, SDL_Renderer* renderer, const char* filePath)
{
    if (!filePath || filePath[0] == '\0') return;

    // کپی به پوشه پیش‌فرض
    string copiedPath = filePath;
    if (copyFileToFolder(filePath, game.defaultBackdropFolder))
    {
        // اگر کپی موفق بود، از فایل کپی شده استفاده کن
        size_t pos = string(filePath).find_last_of("/\\");
        string fileName = (pos != string::npos) ? string(filePath).substr(pos + 1) : string(filePath);
        copiedPath = game.defaultBackdropFolder + fileName;
        log_info(("Copied backdrop to: " + copiedPath).c_str());
    }

    Backdrop b;
    string path = copiedPath;
    size_t pos = path.find_last_of("/\\");
    string fileName = (pos != string::npos) ? path.substr(pos + 1) : path;
    size_t dotPos = fileName.find_last_of(".");
    string nameWithoutExt = (dotPos != string::npos) ? fileName.substr(0, dotPos) : fileName;

    b.name = "Custom: " + nameWithoutExt;
    b.filePath = copiedPath;  // از مسیر کپی شده استفاده کن
    b.isCustom = true;

    SDL_Surface* surface = IMG_Load(copiedPath.c_str());  // از مسیر جدید لود کن
    if (surface)
    {
        b.texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        game.backdrops.push_back(b);
        game.currentBackdrop = game.backdrops.size() - 1;
        log_info(("Custom backdrop added and copied: " + copiedPath).c_str());
    }
    else
    {
        log_error(("Failed to load custom backdrop: " + copiedPath + " - " + string(IMG_GetError())).c_str());
    }
}

void reloadAllTextures(GameState& game, SDL_Renderer* renderer)
{
    for (auto& sprite : game.sprites)
    {
        if (!sprite.imagePath.empty())
        {
            if (sprite.texture)
            {
                SDL_DestroyTexture(sprite.texture);
                sprite.texture = nullptr;
            }

            SDL_Surface* surface = IMG_Load(sprite.imagePath.c_str());
            if (!surface)
            {
                char* basePath = SDL_GetBasePath();
                if (basePath)
                {
                    string fullPath = string(basePath) + sprite.imagePath;
                    SDL_free(basePath);
                    surface = IMG_Load(fullPath.c_str());
                }
            }

            if (surface)
            {
                sprite.texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (sprite.texture)
                {
                    SDL_QueryTexture(sprite.texture, NULL, NULL, &sprite.w, &sprite.h);
                    log_info(("Reloaded texture for: " + sprite.name).c_str());
                }
                SDL_FreeSurface(surface);
            }
            else
            {
                log_warning(("Failed to reload texture for " + sprite.name).c_str());
                sprite.w = 50;
                sprite.h = 50;
            }
        }
    }

    for (auto& b : game.backdrops)
    {
        if (!b.filePath.empty())
        {
            if (b.texture)
            {
                SDL_DestroyTexture(b.texture);
                b.texture = nullptr;
            }

            SDL_Surface* surface = IMG_Load(b.filePath.c_str());
            if (!surface)
            {
                char* basePath = SDL_GetBasePath();
                if (basePath)
                {
                    string fullPath = string(basePath) + b.filePath;
                    SDL_free(basePath);
                    surface = IMG_Load(fullPath.c_str());
                }
            }

            if (surface)
            {
                b.texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                log_info(("Reloaded backdrop: " + b.name).c_str());
            }
            else
            {
                log_warning(("Failed to reload backdrop: " + b.name).c_str());
                b.texture = nullptr;
            }
        }
    }
}

Sprite* getActiveSprite(GameState& game)
{
    if (game.activeSpriteIndex >= 0 && game.activeSpriteIndex < game.sprites.size())
        return &game.sprites[game.activeSpriteIndex];
    return nullptr;
}

void setActiveSprite(GameState& game, int index)
{
    if (index < 0 || index >= game.sprites.size())
        return;

    if (game.activeSpriteIndex >= 0 && game.activeSpriteIndex < game.sprites.size())
        game.sprites[game.activeSpriteIndex].isActive = false;

    game.activeSpriteIndex = index;
    game.sprites[index].isActive = true;
    log_info(("Active sprite changed to: " + game.sprites[index].name).c_str());
}

void addSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath)
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
    newSprite.index = game.sprites.size();
    newSprite.isActive = false;
    newSprite.currentCostume = 0;

    // اضافه کردن costume‌های پیش‌فرض
    // costume اول (همان imagePath ورودی یا "cat.png")
    if (imagePath && strlen(imagePath) > 0) {
        newSprite.costumes.push_back(imagePath);
    } else {
        newSprite.costumes.push_back("cat.png");
    }
    // دو costume اضافی
    newSprite.costumes.push_back("dog.png");
    newSprite.costumes.push_back("bird.png");

    // تنظیم imagePath فعلی به اولین costume
    newSprite.imagePath = newSprite.costumes[0];

    if (renderer && !newSprite.imagePath.empty())
    {
        loadSpriteTexture(&newSprite, renderer, newSprite.imagePath.c_str());
    }

    game.sprites.push_back(newSprite);
    log_info(("Sprite added: " + string(name) + " with " + to_string(newSprite.costumes.size()) + " costumes").c_str());
}

void removeSprite(GameState& game, int index)
{
    if (index < 0 || index >= game.sprites.size())
        return;

    log_info(("Removing sprite: " + game.sprites[index].name).c_str());

    if (game.sprites[index].texture)
        SDL_DestroyTexture(game.sprites[index].texture);

    game.sprites.erase(game.sprites.begin() + index);

    if (game.activeSpriteIndex == index)
    {
        if (game.sprites.size() > 0)
            setActiveSprite(game, 0);
        else
            game.activeSpriteIndex = -1;
    }
    else if (game.activeSpriteIndex > index)
    {
        game.activeSpriteIndex--;
    }
}

void saveProject(const GameState& game, const string& filename)
{
    log_info(("Saving project to: " + filename).c_str());
    ofstream file(filename);
    if (!file.is_open())
    {
        log_error(("Could not save file: " + filename).c_str());
        return;
    }

    // Backdrops
    file << "Backdrops " << game.backdrops.size() << "\n";
    for (const auto& b : game.backdrops)
    {
        string safeName = b.name;
        for (char& c : safeName) if (c == ' ') c = '_';
        file << safeName << " " << b.filePath << " " << b.isCustom << "\n";
    }
    file << "CurrentBackdrop " << game.currentBackdrop << "\n";

    // Sprites
    file << "Sprites " << game.sprites.size() << "\n";
    for (const Sprite& s : game.sprites)
    {
        string safeName = s.name;
        for (char& c : safeName) if (c == ' ') c = '_';
        string safeImagePath = s.imagePath;
        for (char& c : safeImagePath) if (c == ' ') c = '_';
        file << "Sprite "
             << s.x << " " << s.y << " "
             << s.w << " " << s.h << " "
             << s.visible << " " << s.direction << " "
             << safeName << " " << safeImagePath << " "
             << s.index << " " << s.isActive << "\n";

        // Costumes
        file << "Costumes " << s.costumes.size() << " ";
        for (const string& costume : s.costumes)
        {
            string safeCostume = costume;
            for (char& c : safeCostume) if (c == ' ') c = '_';
            file << safeCostume << " ";
        }
        file << s.currentCostume << "\n";
    }
    file << "ActiveSprite " << game.activeSpriteIndex << "\n";

    // Blocks
    file << "Blocks " << game.program.size() << "\n";
    for (const Block& b : game.program)
    {
        file << (int)b.type << " " << b.parameters.size() << " ";
        for (const Value& v : b.parameters)
        {
            if (v.type == VALUE_NUMBER)
                file << "num " << v.asNumber() << " ";
            else if (v.type == VALUE_STRING)
            {
                string safeStr = v.asString();
                for (char& c : safeStr) if (c == ' ') c = '_';
                file << "str " << safeStr << " ";
            }
            else if (v.type == VALUE_BOOLEAN)
                file << "bool " << v.asBoolean() << " ";
        }

        string safeVarName = b.variableName;
        for (char& c : safeVarName) if (c == ' ') c = '_';
        string safeEventName = b.eventName;
        for (char& c : safeEventName) if (c == ' ') c = '_';
        string safeMessageName = b.messageName;
        for (char& c : safeMessageName) if (c == ' ') c = '_';
        string safeLeftVar = b.leftVar;
        for (char& c : safeLeftVar) if (c == ' ') c = '_';
        string safeRightVar = b.rightVar;
        for (char& c : safeRightVar) if (c == ' ') c = '_';
        string safeEditingBuffer = b.editingBuffer;
        for (char& c : safeEditingBuffer) if (c == ' ') c = '_';

        file << b.repeatCount << " " << safeVarName << " "
             << safeEventName << " " << b.keyCode << " "
             << safeMessageName << " "
             << b.ifTrueJump << " " << b.ifFalseJump << " " << b.endIfIndex << " "
             << (int)b.compareOp << " " << safeLeftVar << " " << safeRightVar << " "
             << (int)b.sensorType << " " << b.sensorParam << " "
             << (int)b.penParam << " "
             << b.editingMode << " " << b.editingField << " "
             << safeEditingBuffer << "\n";
    }

    // Variables
    file << "Variables " << game.variables.size() << "\n";
    for (const auto& var : game.variables)
    {
        string safeName = var.first;
        for (char& c : safeName) if (c == ' ') c = '_';
        file << safeName << " ";
        if (var.second.type == VALUE_NUMBER)
            file << "num " << var.second.asNumber() << "\n";
        else if (var.second.type == VALUE_STRING)
        {
            string safeStr = var.second.asString();
            for (char& c : safeStr) if (c == ' ') c = '_';
            file << "str " << safeStr << "\n";
        }
        else if (var.second.type == VALUE_BOOLEAN)
            file << "bool " << var.second.asBoolean() << "\n";
    }

    file.close();
    log_info(("Project saved to " + filename).c_str());
}

void loadProject(GameState& game, const string& filename)
{
    log_info(("Loading project from: " + filename).c_str());

    ifstream file(filename);
    if (!file.is_open())
    {
        log_error(("Could not load file: " + filename).c_str());
        return;
    }

    // پاک کردن داده‌های قبلی
    for (auto& s : game.sprites) if (s.texture) SDL_DestroyTexture(s.texture);
    for (auto& b : game.backdrops) if (b.texture) SDL_DestroyTexture(b.texture);
    game.sprites.clear();
    game.program.clear();
    game.variables.clear();
    game.backdrops.clear();
    game.scriptStartIndices.clear();
    game.scriptActive.clear();
    game.scriptCurrentBlock.clear();
    game.messageHandlers.clear();
    game.editingMode = false;
    game.editingField = -1;
    game.editingBuffer = "";

    string line;
    while (getline(file, line))
    {
        if (line.empty()) continue;
        istringstream iss(line);
        string token;
        iss >> token;

        if (token == "Backdrops")
        {
            int count; iss >> count;
            for (int i = 0; i < count; i++)
            {
                if (!getline(file, line)) break;
                istringstream iss2(line);
                Backdrop b;
                string name;
                iss2 >> name >> b.filePath >> b.isCustom;
                for (char& c : name) if (c == '_') c = ' ';
                b.name = name;
                b.texture = nullptr;
                game.backdrops.push_back(b);
            }
        }
        else if (token == "CurrentBackdrop")
        {
            iss >> game.currentBackdrop;
        }
        else if (token == "Sprites")
        {
            int count; iss >> count;
            for (int i = 0; i < count; i++)
            {
                // خط Sprite
                if (!getline(file, line)) break;
                istringstream iss2(line);
                string spriteToken;
                iss2 >> spriteToken; // باید "Sprite" باشد
                Sprite s;
                string name, imagePath;
                iss2 >> s.x >> s.y >> s.w >> s.h >> s.visible >> s.direction >> name >> imagePath >> s.index >> s.isActive;
                for (char& c : name) if (c == '_') c = ' ';
                for (char& c : imagePath) if (c == '_') c = ' ';
                s.name = name;
                s.imagePath = imagePath;
                s.message = "";
                s.isThinking = false;
                s.texture = nullptr;
                s.currentCostume = 0;

                // خط Costumes
                if (!getline(file, line)) break;
                istringstream iss3(line);
                string costumeToken;
                iss3 >> costumeToken; // باید "Costumes" باشد
                int costumeCount;
                iss3 >> costumeCount;
                for (int j = 0; j < costumeCount; j++)
                {
                    string costumePath;
                    iss3 >> costumePath;
                    for (char& c : costumePath) if (c == '_') c = ' ';
                    s.costumes.push_back(costumePath);
                }
                iss3 >> s.currentCostume;

                game.sprites.push_back(s);
            }
        }
        else if (token == "ActiveSprite")
        {
            iss >> game.activeSpriteIndex;
        }
        else if (token == "Blocks")
        {
            int count; iss >> count;
            for (int i = 0; i < count; i++)
            {
                if (!getline(file, line)) break;
                istringstream iss2(line);
                string blockToken;
                iss2 >> blockToken; // باید "Block" باشد
                Block b;
                int typeInt, paramCount;
                iss2 >> typeInt >> paramCount;
                b.type = (BlockType)typeInt;

                for (int j = 0; j < paramCount; j++)
                {
                    string valType;
                    iss2 >> valType;
                    if (valType == "num")
                    {
                        double d; iss2 >> d;
                        b.parameters.push_back(Value(d));
                    }
                    else if (valType == "str")
                    {
                        string s; iss2 >> s;
                        for (char& c : s) if (c == '_') c = ' ';
                        b.parameters.push_back(Value(s));
                    }
                    else if (valType == "bool")
                    {
                        bool bVal; iss2 >> bVal;
                        b.parameters.push_back(Value(bVal));
                    }
                }

                string varName, eventName, messageName, leftVar, rightVar, editingBuffer;
                int compareOpInt, sensorTypeInt, penParamInt;
                iss2 >> b.repeatCount >> varName >> eventName >> b.keyCode >> messageName
                     >> b.ifTrueJump >> b.ifFalseJump >> b.endIfIndex
                     >> compareOpInt >> leftVar >> rightVar
                     >> sensorTypeInt >> b.sensorParam
                     >> penParamInt
                     >> b.editingMode >> b.editingField >> editingBuffer;

                for (char& c : varName) if (c == '_') c = ' ';
                for (char& c : eventName) if (c == '_') c = ' ';
                for (char& c : messageName) if (c == '_') c = ' ';
                for (char& c : leftVar) if (c == '_') c = ' ';
                for (char& c : rightVar) if (c == '_') c = ' ';
                for (char& c : editingBuffer) if (c == '_') c = ' ';

                b.variableName = varName;
                b.eventName = eventName;
                b.messageName = messageName;
                b.leftVar = leftVar;
                b.rightVar = rightVar;
                b.compareOp = (CompareOp)compareOpInt;
                b.sensorType = (SensorType)sensorTypeInt;
                b.penParam = (PenColorParam)penParamInt;
                b.editingBuffer = editingBuffer;

                // reset editing mode
                b.editingMode = false;
                b.editingField = -1;
                b.editingBuffer = ""; // بازنشانی برای اجرا

                game.program.push_back(b);
            }
        }
        else if (token == "Variables")
        {
            int count; iss >> count;
            for (int i = 0; i < count; i++)
            {
                if (!getline(file, line)) break;
                istringstream iss2(line);
                string varName, valType;
                iss2 >> varName >> valType;
                for (char& c : varName) if (c == '_') c = ' ';
                if (valType == "num")
                {
                    double d; iss2 >> d;
                    game.variables[varName] = Value(d);
                }
                else if (valType == "str")
                {
                    string s; iss2 >> s;
                    for (char& c : s) if (c == '_') c = ' ';
                    game.variables[varName] = Value(s);
                }
                else if (valType == "bool")
                {
                    bool b; iss2 >> b;
                    game.variables[varName] = Value(b);
                }
            }
        }
    }

    file.close();
    log_info(("Project loaded from " + filename).c_str());
}

// ==================== تابع اصلی update (کامل شده با Control) ====================

void update(GameState& game, SDL_Renderer* renderer)
{
    if (!game.isRunningCode || game.isPaused)
        return;

    if (game.isShowingMessage && game.messageDuration > 0)
    {
        if (SDL_GetTicks() - game.messageStartTime >= game.messageDuration)
        {
            Sprite* active = getActiveSprite(game);
            if (active) active->message = "";
            game.isShowingMessage = false;
        }
    }

    // پیش‌پردازش بلوک‌های کنترلی (اگر لازم باشد)
    static bool preprocessed = false;
    if (!preprocessed)
    {
        preprocessControlBlocks(game);
        preprocessed = true;
    }

    // پردازش صف پیام‌ها
    processMessageQueue(game);

    if (!game.isRunningCode)
        return;

    Sprite* activeSprite = getActiveSprite(game);
    if (!activeSprite && game.sprites.size() > 0)
    {
        setActiveSprite(game, 0);
        activeSprite = getActiveSprite(game);
    }

    const double speed = 2.0;
    int safetyCounter = 0;
    const int MAX_OPERATIONS_PER_FRAME = 1000;

    if (game.waitingForBroadcast)
    {
        if (SDL_GetTicks() - game.broadcastWaitStartTime > 100)
        {
            game.waitingForBroadcast = false;
            game.waitingScriptIndex = -1;
        }
        return;
    }

    // رویداد Green Flag
    if (game.greenFlagPressed)
    {
        log_info("Green flag pressed - starting program");
        game.greenFlagPressed = false;

        game.scriptStartIndices.clear();
        game.scriptActive.clear();
        game.scriptCurrentBlock.clear();

        for (int i = 0; i < game.program.size(); i++)
        {
            if (game.program[i].type == WHEN_GREEN_FLAG)
            {
                game.scriptStartIndices.push_back(i + 1);
                game.scriptActive.push_back(true);
                game.scriptCurrentBlock.push_back(i + 1);
            }
        }
    }

    // رویداد کلیک روی اسپرایت
    if (game.spriteClicked && game.clickedSpriteIndex >= 0)
    {
        log_info(("Sprite clicked: " + game.sprites[game.clickedSpriteIndex].name).c_str());
        game.spriteClicked = false;

        for (int i = 0; i < game.program.size(); i++)
        {
            if (game.program[i].type == WHEN_SPRITE_CLICKED)
            {
                game.scriptStartIndices.push_back(i + 1);
                game.scriptActive.push_back(true);
                game.scriptCurrentBlock.push_back(i + 1);
            }
        }
    }

    // رویداد کلید فشرده
    for (int i = 0; i < game.program.size(); i++)
    {
        if (game.program[i].type == WHEN_KEY_PRESSED)
        {
            int key = game.program[i].keyCode;
            if (game.pressedThisFrame[key])
            {
                bool alreadyActive = false;
                for (int j = 0; j < game.scriptStartIndices.size(); j++)
                {
                    if (game.scriptStartIndices[j] == i + 1)
                    {
                        alreadyActive = true;
                        break;
                    }
                }

                if (!alreadyActive)
                {
                    game.scriptStartIndices.push_back(i + 1);
                    game.scriptActive.push_back(true);
                    game.scriptCurrentBlock.push_back(i + 1);
                }
            }
        }
    }

    // ===== اجرای اسکریپت‌های فعال =====
    if (game.stepMode)
    {
        bool anyBlockExecuted = false;
        for (int s = 0; s < game.scriptStartIndices.size(); s++)
        {
            if (!game.scriptActive[s]) continue;

            int& scriptPC = game.scriptCurrentBlock[s];

            if (scriptPC >= game.program.size())
            {
                game.scriptActive[s] = false;
                continue;
            }

            Block& b = game.program[scriptPC];
            bool blockCompleted = false;

            // STOP_ALL
            if (b.type == STOP_ALL)
            {
                log_info("Stop all scripts");
                game.isRunningCode = false;
                game.scriptActive.assign(game.scriptActive.size(), false);
                blockCompleted = true;
            }
                // STOP_THIS_SCRIPT
            else if (b.type == STOP_THIS_SCRIPT)
            {
                log_info("Stop this script");
                game.scriptActive[s] = false;
                blockCompleted = true;
            }
                // WAIT_UNTIL
            else if (b.type == WAIT_UNTIL)
            {
                if (!b.parameters.empty())
                {
                    bool condition = b.parameters[0].asBoolean();
                    if (condition)
                    {
                        scriptPC++;
                        blockCompleted = true;
                    }
                }
                else
                {
                    scriptPC++;
                    blockCompleted = true;
                }
            }
                // REPEAT_UNTIL
            else if (b.type == REPEAT_UNTIL)
            {
                if (!b.parameters.empty())
                {
                    bool condition = b.parameters[0].asBoolean();
                    if (condition)
                    {
                        int endRepeatIndex = -1;
                        for (int i = scriptPC + 1; i < game.program.size(); i++)
                            if (game.program[i].type == END_REPEAT) { endRepeatIndex = i; break; }
                        if (endRepeatIndex != -1) scriptPC = endRepeatIndex + 1;
                        else scriptPC++;
                        blockCompleted = true;
                    }
                    else
                    {
                        scriptPC++;
                        blockCompleted = true;
                    }
                }
                else
                {
                    scriptPC++;
                    blockCompleted = true;
                }
            }
                // IF_THEN
            else if (b.type == IF_THEN)
            {
                bool condition = b.parameters.empty() ? true : b.parameters[0].asBoolean();
                if (condition) scriptPC++;
                else
                {
                    if (b.endIfIndex != -1) scriptPC = b.endIfIndex + 1;
                    else scriptPC++;
                }
                blockCompleted = true;
            }
                // IF_THEN_ELSE
            else if (b.type == IF_THEN_ELSE)
            {
                bool condition = b.parameters.empty() ? true : b.parameters[0].asBoolean();
                IfStackFrame frame; frame.scriptIndex = s; frame.endIfIndex = b.endIfIndex; frame.inElseBranch = !condition;
                game.ifStack.push(frame);
                if (condition) scriptPC++;
                else
                {
                    for (int i = scriptPC + 1; i < game.program.size(); i++)
                        if (game.program[i].type == ELSE) { scriptPC = i + 1; break; }
                }
                blockCompleted = true;
            }
                // END_IF
            else if (b.type == END_IF)
            {
                if (!game.ifStack.empty()) game.ifStack.pop();
                scriptPC++; blockCompleted = true;
            }
                // REPEAT
            else if (b.type == REPEAT)
            {
                game.repeatCountStack.push_back(b.repeatCount);
                game.repeatStartStack.push_back(scriptPC + 1);
                scriptPC++; blockCompleted = true;
            }
                // END_REPEAT
            else if (b.type == END_REPEAT)
            {
                if (!game.repeatCountStack.empty())
                {
                    int& lastCount = game.repeatCountStack.back();
                    lastCount--;
                    if (lastCount > 0) scriptPC = game.repeatStartStack.back();
                    else { game.repeatCountStack.pop_back(); game.repeatStartStack.pop_back(); scriptPC++; }
                }
                blockCompleted = true;
            }
                // FOREVER
            else if (b.type == FOREVER)
            {
                game.repeatStartStack.push_back(scriptPC + 1);
                scriptPC++; blockCompleted = true;
            }
                // END_FOREVER
            else if (b.type == END_FOREVER)
            {
                if (!game.repeatStartStack.empty()) scriptPC = game.repeatStartStack.back();
                blockCompleted = true;
            }
                // WAIT
            else if (b.type == WAIT)
            {
                if (!game.isWaiting)
                {
                    game.waitStartTime = SDL_GetTicks();
                    if (!b.parameters.empty())
                        game.waitDuration = b.parameters[0].asNumber() * 1000;
                    else
                        game.waitDuration = 1000;
                    game.isWaiting = true;
                }
                if (SDL_GetTicks() - game.waitStartTime >= game.waitDuration)
                {
                    game.isWaiting = false;
                    scriptPC++;
                    blockCompleted = true;
                }
            }
                // BROADCAST
            else if (b.type == BROADCAST)
            {
                if (!b.messageName.empty()) broadcastMessage(game, b.messageName, s);
                scriptPC++; blockCompleted = true;
            }
                // BROADCAST_AND_WAIT
            else if (b.type == BROADCAST_AND_WAIT)
            {
                if (!b.messageName.empty()) broadcastMessageAndWait(game, b.messageName, s);
                scriptPC++; blockCompleted = true;
            }
                // SAY, THINK, ...
                // SAY, THINK, SAY_FOR, THINK_FOR
            else if (b.type == SAY || b.type == SAY_FOR || b.type == THINK || b.type == THINK_FOR)
            {
                if (!activeSprite) continue;

                if (!b.parameters.empty())
                {
                    activeSprite->message = b.parameters[0].asString();
                    activeSprite->isThinking = (b.type == THINK || b.type == THINK_FOR);

                    if (b.type == SAY_FOR || b.type == THINK_FOR)
                    {
                        if (b.parameters.size() >= 2)
                            game.messageDuration = b.parameters[1].asNumber() * 1000;
                        else
                            game.messageDuration = 2000;
                        game.messageStartTime = SDL_GetTicks();
                        game.isShowingMessage = true;
                    }
                    else
                    {
                        // برای SAY و THINK ساده، پیام مادام‌العمر است
                        game.isShowingMessage = true;
                        game.messageDuration = 0; // به معنی عدم انقضا
                    }
                }
                scriptPC++;
                blockCompleted = true;
            }
                // SHOW, HIDE, CHANGE_SIZE, SET_SIZE
            else if (b.type == SHOW)
            {
                if (activeSprite) activeSprite->visible = true;
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == HIDE)
            {
                if (activeSprite) activeSprite->visible = false;
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == CHANGE_SIZE)
            {
                if (activeSprite && !b.parameters.empty())
                {
                    double percent = b.parameters[0].asNumber();
                    int newW = (int)(activeSprite->w * (1 + percent/100));
                    int newH = (int)(activeSprite->h * (1 + percent/100));
                    if (newW < 5) newW = 5;
                    if (newH < 5) newH = 5;
                    if (newW > 500) newW = 500;
                    if (newH > 500) newH = 500;
                    activeSprite->w = newW;
                    activeSprite->h = newH;
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SET_SIZE)
            {
                if (activeSprite && !b.parameters.empty())
                {
                    double percent = b.parameters[0].asNumber();
                    int newSize = (int)(percent);
                    if (newSize < 5) newSize = 5;
                    if (newSize > 500) newSize = 500;
                    activeSprite->w = newSize;
                    activeSprite->h = newSize;
                }
                scriptPC++; blockCompleted = true;
            }
                // SOUND
            else if (b.type == PLAY_SOUND || b.type == PLAY_SOUND_UNTIL_DONE)
            {
                if (game.soundEffect)
                {
                    int volumeLevel = (game.volume * MIX_MAX_VOLUME) / 100;
                    Mix_VolumeChunk(game.soundEffect, volumeLevel);
                    Mix_PlayChannel(-1, game.soundEffect, 0);
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == STOP_ALL_SOUNDS)
            {
                Mix_HaltChannel(-1); game.isPlayingSound = false;
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == CHANGE_VOLUME)
            {
                if (!b.parameters.empty())
                {
                    int change = (int)b.parameters[0].asNumber();
                    game.volume += change;
                    if (game.volume < 0) game.volume = 0;
                    if (game.volume > 100) game.volume = 100;
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SET_VOLUME)
            {
                if (!b.parameters.empty())
                {
                    int newVolume = (int)b.parameters[0].asNumber();
                    if (newVolume < 0) newVolume = 0;
                    if (newVolume > 100) newVolume = 100;
                    game.volume = newVolume;
                }
                scriptPC++; blockCompleted = true;
            }
                // OPERATORS
            else if (b.type >= OP_ADD && b.type <= OP_XOR)
            {
                Value result = evaluateOperator(b.type, b.parameters);
                scriptPC++; blockCompleted = true;
            }
                // VARIABLES
            else if (b.type == SET_VARIABLE)
            {
                if (!b.parameters.empty() && !b.variableName.empty())
                    game.variables[b.variableName] = b.parameters[0];
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == CHANGE_VARIABLE)
            {
                if (!b.parameters.empty() && !b.variableName.empty())
                {
                    if (game.variables.find(b.variableName) == game.variables.end())
                        game.variables[b.variableName] = Value(0.0);
                    Value current = game.variables[b.variableName];
                    double newVal = current.asNumber() + b.parameters[0].asNumber();
                    game.variables[b.variableName] = Value(newVal);
                }
                scriptPC++; blockCompleted = true;
            }
                // MOTION
            else if (b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT)
            {
                if (activeSprite)
                {
                    double steps = b.parameters.empty() ? 5 : b.parameters[0].asNumber();
                    switch (b.type)
                    {
                        case MOVE_UP:    activeSprite->y -= steps; break;
                        case MOVE_DOWN:  activeSprite->y += steps; break;
                        case MOVE_LEFT:  activeSprite->x -= steps; break;
                        case MOVE_RIGHT: activeSprite->x += steps; break;
                        default: break;
                    }
                    if (activeSprite->x < 0) activeSprite->x = 0;
                    if (activeSprite->y < 0) activeSprite->y = 0;
                    if (activeSprite->x + activeSprite->w > game.screenWidth) activeSprite->x = game.screenWidth - activeSprite->w;
                    if (activeSprite->y + activeSprite->h > game.screenHeight) activeSprite->y = game.screenHeight - activeSprite->h;
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == TURN_RIGHT || b.type == TURN_LEFT)
            {
                if (activeSprite && !b.parameters.empty())
                {
                    double degrees = b.parameters[0].asNumber();
                    if (b.type == TURN_RIGHT) activeSprite->direction += degrees;
                    else activeSprite->direction -= degrees;
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == GOTO_XY)
            {
                if (activeSprite && b.parameters.size() >= 2)
                {
                    activeSprite->x = b.parameters[0].asNumber();
                    activeSprite->y = b.parameters[1].asNumber();
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == CHANGE_X || b.type == CHANGE_Y)
            {
                if (activeSprite && !b.parameters.empty())
                {
                    if (b.type == CHANGE_X) activeSprite->x += b.parameters[0].asNumber();
                    else activeSprite->y += b.parameters[0].asNumber();
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SET_X || b.type == SET_Y)
            {
                if (activeSprite && !b.parameters.empty())
                {
                    if (b.type == SET_X) activeSprite->x = b.parameters[0].asNumber();
                    else activeSprite->y = b.parameters[0].asNumber();
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == GOTO_RANDOM)
            {
                if (activeSprite)
                {
                    activeSprite->x = rand() % (game.screenWidth - activeSprite->w);
                    activeSprite->y = rand() % (game.screenHeight - activeSprite->h);
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == GOTO_MOUSE)
            {
                if (activeSprite)
                {
                    int categoriesPanelWidth = 180;
                    int examplesPanelX = categoriesPanelWidth + 30;
                    int examplesPanelWidth = 250;
                    int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
                    int codeAreaWidth = 600;
                    int stagePanelX = codeAreaX + codeAreaWidth + 10;
                    int stagePanelWidth = game.screenWidth - stagePanelX - 5;
                    if (stagePanelWidth > 500) stagePanelWidth = 500;
                    int stageX = stagePanelX + 5;
                    int stageY = 70;
                    int stageWidth = stagePanelWidth - 20;
                    int stageHeight = 300;

                    if (game.mouseX >= stageX && game.mouseX <= stageX + stageWidth &&
                        game.mouseY >= stageY && game.mouseY <= stageY + stageHeight)
                    {
                        activeSprite->x = (double)(game.mouseX - stageX) * game.screenWidth / stageWidth;
                        activeSprite->y = (double)(game.mouseY - stageY) * game.screenHeight / stageHeight;
                    }
                    else
                    {
                        activeSprite->x = game.screenWidth / 2 - activeSprite->w / 2;
                        activeSprite->y = game.screenHeight / 2 - activeSprite->h / 2;
                    }
                }
                scriptPC++; blockCompleted = true;
            }
                // PEN
            else if (b.type == PEN_ERASE_ALL)
            {
                penEraseAll(game);
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == PEN_STAMP)
            {
                if (activeSprite) penStamp(activeSprite, game, renderer);
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == PEN_PEN_DOWN)
            {
                game.penDown = true;
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == PEN_PEN_UP)
            {
                game.penDown = false;
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == PEN_SET_COLOR_PARAM || b.type == PEN_CHANGE_COLOR_PARAM)
            {
                if (!b.parameters.empty())
                {
                    double val = b.parameters[0].asNumber();
                    if (b.type == PEN_SET_COLOR_PARAM) penSetParam(game, b.penParam, val);
                    else penChangeParam(game, b.penParam, val);
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == PEN_SET_SIZE || b.type == PEN_CHANGE_SIZE)
            {
                if (!b.parameters.empty())
                {
                    int val = (int)b.parameters[0].asNumber();
                    if (b.type == PEN_SET_SIZE) penSetSize(game, val);
                    else penChangeSize(game, val);
                }
                scriptPC++; blockCompleted = true;
            }
                // SENSING
            else if (b.type == SENSOR_RESET_TIMER)
            {
                resetTimer(game);
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SENSOR_ASK_AND_WAIT)
            {
                if (!game.waitingForAnswer)
                {
                    string q = b.parameters.empty() ? "" : b.parameters[0].asString();
                    askQuestion(game, q);
                }
                if (game.waitingForAnswer)
                {
                    return;
                }
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SET_VARIABLE_TO_SENSOR)
            {
                if (!b.variableName.empty())
                {
                    Value val;
                    switch (b.sensorType)
                    {
                        case SENSOR_TYPE_TOUCHING_EDGE:   val = Value(isTouchingEdge(activeSprite, game)); break;
                        case SENSOR_TYPE_TOUCHING_MOUSE:  val = Value(isTouchingMouse(activeSprite, game)); break;
                        case SENSOR_TYPE_KEY_PRESSED:     val = Value(isKeyPressed(game, b.keyCode)); break;
                        case SENSOR_TYPE_MOUSE_DOWN:      val = Value(isMouseDown(game)); break;
                        case SENSOR_TYPE_MOUSE_X:         val = Value((double)getMouseX(game)); break;
                        case SENSOR_TYPE_MOUSE_Y:         val = Value((double)getMouseY(game)); break;
                        case SENSOR_TYPE_TIMER:           val = Value(getTimerValue(game)); break;
                        case SENSOR_TYPE_ANSWER:          val = Value(game.answer); break;
                        case SENSOR_TYPE_DISTANCE_TO_MOUSE: val = Value(getDistanceToMouse(activeSprite, game)); break;
                        default: val = Value(0.0);
                    }
                    game.variables[b.variableName] = val;
                }
                scriptPC++; blockCompleted = true;
            }
                // LOOKS REPORTERS
            else if (b.type == COSTUME_NUMBER)
            {
                if (!b.variableName.empty() && activeSprite)
                    game.variables[b.variableName] = Value((double)activeSprite->index);
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SPRITE_SIZE)
            {
                if (!b.variableName.empty() && activeSprite)
                    game.variables[b.variableName] = Value((double)activeSprite->w);
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == BACKDROP_NUMBER)
            {
                if (!b.variableName.empty())
                    game.variables[b.variableName] = Value((double)game.currentBackdrop);
                scriptPC++; blockCompleted = true;
            }
            else if (b.type == SET_COMPARISON)
            {
                Value leftVal;
                if (!b.leftVar.empty())
                {
                    if (game.variables.count(b.leftVar)) leftVal = game.variables[b.leftVar];
                    else leftVal = Value(0.0);
                }
                else if (b.parameters.size() > 0) leftVal = b.parameters[0];
                else leftVal = Value(0.0);

                Value rightVal;
                if (!b.rightVar.empty())
                {
                    if (game.variables.count(b.rightVar)) rightVal = game.variables[b.rightVar];
                    else rightVal = Value(0.0);
                }
                else if (b.parameters.size() > 1) rightVal = b.parameters[1];
                else rightVal = Value(0.0);

                double l = leftVal.asNumber();
                double r = rightVal.asNumber();
                bool result = false;
                switch (b.compareOp)
                {
                    case CMP_EQUAL: result = (l == r); break;
                    case CMP_NOT_EQUAL: result = (l != r); break;
                    case CMP_LESS: result = (l < r); break;
                    case CMP_LESS_OR_EQUAL: result = (l <= r); break;
                    case CMP_GREATER: result = (l > r); break;
                    case CMP_GREATER_OR_EQUAL: result = (l >= r); break;
                }
                if (!b.variableName.empty())
                    game.variables[b.variableName] = Value(result);
                scriptPC++; blockCompleted = true;
            }
            else
            {
                scriptPC++; blockCompleted = true;
            }

            if (blockCompleted)
            {
                game.stepMode = false;
                game.isRunningCode = false;
                return;
            }
            else
            {
                return;
            }
        }
        game.stepMode = false;
        game.isRunningCode = false;
        return;
    }
    else
    {
        for (int s = 0; s < game.scriptStartIndices.size(); s++)
        {
            if (!game.scriptActive[s])
                continue;

            int& scriptPC = game.scriptCurrentBlock[s];

            if (scriptPC >= game.program.size())
            {
                game.scriptActive[s] = false;
                continue;
            }

            Block& b = game.program[scriptPC];

            // ===== بلوک‌های Sensing =====

            // SENSOR_RESET_TIMER
            if (b.type == SENSOR_RESET_TIMER)
            {
                resetTimer(game);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // SENSOR_ASK_AND_WAIT
            if (b.type == SENSOR_ASK_AND_WAIT)
            {
                if (!game.waitingForAnswer)
                {
                    string question = b.parameters.empty() ? "" : b.parameters[0].asString();
                    askQuestion(game, question);
                    return; // تا دریافت پاسخ، اجرا متوقف می‌شود
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // SET_VARIABLE_TO_SENSOR
            if (b.type == SET_VARIABLE_TO_SENSOR)
            {
                if (!b.variableName.empty())
                {
                    Value val;
                    switch (b.sensorType)
                    {
                        case SENSOR_TYPE_TOUCHING_EDGE:
                            val = Value(isTouchingEdge(activeSprite, game));
                            break;
                        case SENSOR_TYPE_TOUCHING_MOUSE:
                            val = Value(isTouchingMouse(activeSprite, game));
                            break;
                        case SENSOR_TYPE_KEY_PRESSED:
                            val = Value(isKeyPressed(game, b.keyCode));
                            break;
                        case SENSOR_TYPE_MOUSE_DOWN:
                            val = Value(isMouseDown(game));
                            break;
                        case SENSOR_TYPE_MOUSE_X:
                            val = Value((double)getMouseX(game));
                            break;
                        case SENSOR_TYPE_MOUSE_Y:
                            val = Value((double)getMouseY(game));
                            break;
                        case SENSOR_TYPE_TIMER:
                            val = Value(getTimerValue(game));
                            break;
                        case SENSOR_TYPE_ANSWER:
                            val = Value(game.answer);
                            break;
                        case SENSOR_TYPE_DISTANCE_TO_MOUSE:
                            val = Value(getDistanceToMouse(activeSprite, game));
                            break;
                        case SENSOR_TYPE_DISTANCE_TO_SPRITE:
                            val = Value(0.0);
                            break;
                    }
                    game.variables[b.variableName] = val;
                    log_info(("Variable " + b.variableName + " set to sensor value: " + val.asString()).c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // ===== بلوک‌های کنترلی (کنترل جریان) =====

            // STOP_ALL - توقف همه اسکریپت‌ها
            if (b.type == STOP_ALL)
            {
                log_info("Stop all scripts");
                game.isRunningCode = false;
                game.scriptActive.assign(game.scriptActive.size(), false);
                return;
            }

            // STOP_THIS_SCRIPT - توقف این اسکریپت
            if (b.type == STOP_THIS_SCRIPT)
            {
                log_info("Stop this script");
                game.scriptActive[s] = false;
                continue;
            }

            // WAIT_UNTIL - صبر کن تا شرط درست شود
            if (b.type == WAIT_UNTIL)
            {
                if (!b.parameters.empty())
                {
                    bool condition = b.parameters[0].asBoolean();
                    if (!condition)
                    {
                        // شرط هنوز درست نشده، در همین خط بمان
                        return;
                    }
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // REPEAT_UNTIL - تکرار کن تا شرط درست شود
            if (b.type == REPEAT_UNTIL)
            {
                if (!b.parameters.empty())
                {
                    bool condition = b.parameters[0].asBoolean();
                    if (condition)
                    {
                        // شرط درست شده، از حلقه خارج شو
                        // باید به END_REPEAT برویم
                        int endRepeatIndex = -1;
                        for (int i = scriptPC + 1; i < game.program.size(); i++)
                        {
                            if (game.program[i].type == END_REPEAT)
                            {
                                endRepeatIndex = i;
                                break;
                            }
                        }
                        if (endRepeatIndex != -1)
                            scriptPC = endRepeatIndex + 1;
                        else
                        {
                            game.lastExecutedBlock = scriptPC;
                            scriptPC++;
                        }
                        continue;
                    }
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // IF_THEN
// ===== بلوک‌های کنترلی =====
            if (b.type == IF_THEN)
            {
                bool condition = b.parameters.empty() ? true : b.parameters[0].asBoolean();
                if (condition)
                {
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }
                else
                {
                    // شرط نادرست: باید به بعد از END_IF برویم
                    int endIdx = b.endIfIndex;
                    if (endIdx == -1)
                    {
                        // اگر endIfIndex تنظیم نشده بود، به صورت خطی جستجو کن
                        for (int j = scriptPC + 1; j < game.program.size(); j++)
                        {
                            if (game.program[j].type == END_IF)
                            {
                                endIdx = j;
                                break;
                            }
                        }
                    }
                    if (endIdx != -1)
                        scriptPC = endIdx + 1;
                    else
                    {
                        game.lastExecutedBlock = scriptPC;
                        scriptPC++;
                    }
                }
                continue;
            }

            // IF_THEN_ELSE
            if (b.type == IF_THEN_ELSE)
            {
                bool condition = b.parameters.empty() ? true : b.parameters[0].asBoolean();

                // ذخیره وضعیت در پشته
                IfStackFrame frame;
                frame.scriptIndex = s;
                frame.endIfIndex = b.endIfIndex;
                frame.inElseBranch = !condition;  // اگر شرط false بود، باید بریم به else

                game.ifStack.push(frame);

                if (condition)
                {
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }
                else
                {
                    // شرط نادرست: باید به ELSE برویم
                    // پیدا کردن ELSE (فعلاً ساده شده)
                    for (int i = scriptPC + 1; i < game.program.size(); i++)
                    {
                        if (game.program[i].type == ELSE)
                        {
                            scriptPC = i + 1;
                            break;
                        }
                    }
                }
                continue;
            }

            // END_IF
            if (b.type == END_IF)
            {
                if (!game.ifStack.empty())
                {
                    game.ifStack.pop();
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // ===== بلوک‌های کنترلی قبلی (حلقه‌ها) =====
            if (b.type == REPEAT)
            {
                game.repeatCountStack.push_back(b.repeatCount);
                game.repeatStartStack.push_back(scriptPC + 1);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                safetyCounter++;
                continue;
            }

            if (b.type == END_REPEAT)
            {
                if (!game.repeatCountStack.empty())
                {
                    int& lastCount = game.repeatCountStack.back();
                    lastCount--;

                    if (lastCount > 0)
                    {
                        scriptPC = game.repeatStartStack.back();
                    }
                    else
                    {
                        game.repeatCountStack.pop_back();
                        game.repeatStartStack.pop_back();
                        game.lastExecutedBlock = scriptPC;
                        scriptPC++;
                    }
                }
                safetyCounter++;
                continue;
            }

            if (b.type == FOREVER)
            {
                game.repeatStartStack.push_back(scriptPC + 1);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == END_FOREVER)
            {
                if (!game.repeatStartStack.empty())
                {
                    scriptPC = game.repeatStartStack.back();
                }
                continue;
            }

            if (b.type == WAIT)
            {
                if (!game.isWaiting)
                {
                    game.waitStartTime = SDL_GetTicks();
                    if (!b.parameters.empty())
                        game.waitDuration = b.parameters[0].asNumber() * 1000;
                    else
                        game.waitDuration = 1000;
                    game.isWaiting = true;
                    return;
                }

                if (SDL_GetTicks() - game.waitStartTime >= game.waitDuration)
                {
                    game.isWaiting = false;
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }
                return;
            }

            // ===== بلوک‌های رویداد (Events) =====
            if (b.type == BROADCAST)
            {
                if (!b.messageName.empty())
                {
                    broadcastMessage(game, b.messageName, s);
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == BROADCAST_AND_WAIT)
            {
                if (!b.messageName.empty())
                {
                    broadcastMessageAndWait(game, b.messageName, s);
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }
                else
                {
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }
                continue;
            }

            // ===== بلوک‌های ظاهری =====
            if (!activeSprite) continue;

            if (b.type == SAY || b.type == SAY_FOR || b.type == THINK || b.type == THINK_FOR)
            {
                if (b.type == SAY_FOR || b.type == THINK_FOR)
                {
                    if (!game.isShowingMessage)
                    {
                        if (!b.parameters.empty())
                        {
                            activeSprite->message = b.parameters[0].asString();
                            activeSprite->isThinking = (b.type == THINK_FOR);
                            if (b.parameters.size() >= 2)
                                game.messageDuration = b.parameters[1].asNumber() * 1000;
                            else
                                game.messageDuration = 2000;
                            game.messageStartTime = SDL_GetTicks();
                            game.isShowingMessage = true;
                        }
                        return;
                    }

                    if (SDL_GetTicks() - game.messageStartTime >= game.messageDuration)
                    {
                        activeSprite->message = "";
                        game.isShowingMessage = false;
                        game.lastExecutedBlock = scriptPC;
                        scriptPC++;
                    }
                    return;
                }
                else
                {
                    if (!b.parameters.empty())
                    {
                        activeSprite->message = b.parameters[0].asString();
                        activeSprite->isThinking = (b.type == THINK);
                        game.isShowingMessage = true;
                        game.messageDuration = 0; // بدون زمان
                    }
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                    safetyCounter++;
                    continue;
                }
            }

            if (b.type == SHOW)
            {
                if (!activeSprite->visible)
                {
                    activeSprite->visible = true;
                    log_info("Sprite shown");
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == HIDE)
            {
                if (activeSprite->visible)
                {
                    activeSprite->visible = false;
                    log_info("Sprite hidden");
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == CHANGE_SIZE)
            {
                if (!b.parameters.empty())
                {
                    double percent = b.parameters[0].asNumber();
                    activeSprite->w = (int)(activeSprite->w * (1 + percent/100));
                    activeSprite->h = (int)(activeSprite->h * (1 + percent/100));

                    if (activeSprite->w < 5) activeSprite->w = 5;
                    if (activeSprite->h < 5) activeSprite->h = 5;

                    // محدودیت بالا
                    int maxSize = 500;
                    if (activeSprite->w > maxSize) activeSprite->w = maxSize;
                    if (activeSprite->h > maxSize) activeSprite->h = maxSize;
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // در تابع update، در حلقه اجرای بلوک‌ها
            if (b.type == SET_COMPARISON)
            {
                // مقدار سمت چپ
                Value leftVal;
                if (!b.leftVar.empty())
                {
                    if (game.variables.count(b.leftVar))
                        leftVal = game.variables[b.leftVar];
                    else
                        leftVal = Value(0.0);
                }
                else if (b.parameters.size() > 0)
                    leftVal = b.parameters[0];
                else
                    leftVal = Value(0.0);

                // مقدار سمت راست
                Value rightVal;
                if (!b.rightVar.empty())
                {
                    if (game.variables.count(b.rightVar))
                        rightVal = game.variables[b.rightVar];
                    else
                        rightVal = Value(0.0);
                }
                else if (b.parameters.size() > 1)
                    rightVal = b.parameters[1];
                else
                    rightVal = Value(0.0);

                double l = leftVal.asNumber();
                double r = rightVal.asNumber();
                bool result = false;

                switch (b.compareOp)
                {
                    case CMP_EQUAL:          result = (l == r); break;
                    case CMP_NOT_EQUAL:      result = (l != r); break;
                    case CMP_LESS:           result = (l < r);  break;
                    case CMP_LESS_OR_EQUAL:  result = (l <= r); break;
                    case CMP_GREATER:        result = (l > r);  break;
                    case CMP_GREATER_OR_EQUAL: result = (l >= r); break;
                }

                // ذخیره نتیجه در متغیر
                if (!b.variableName.empty())
                    game.variables[b.variableName] = Value(result);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == SET_SIZE)
            {
                if (!b.parameters.empty())
                {
                    double percent = b.parameters[0].asNumber();
                    int newSize = (int)(percent);
                    if (newSize < 5) newSize = 5;
                    if (newSize > 500) newSize = 500; // محدودیت بالا
                    activeSprite->w = newSize;
                    activeSprite->h = newSize;
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // ===== بلوک‌های لایه =====
            if (b.type == GO_TO_FRONT_LAYER)
            {
                // پیدا کردن بالاترین لایه
                int maxLayer = 0;
                for (const auto& sprite : game.sprites)
                    if (sprite.layer > maxLayer) maxLayer = sprite.layer;

                activeSprite->layer = maxLayer + 1;
                log_info(("Sprite moved to front layer: " + to_string(activeSprite->layer)).c_str());
                scriptPC++;
                continue;
            }

            if (b.type == GO_TO_BACK_LAYER)
            {
                // پیدا کردن پایین‌ترین لایه
                int minLayer = 0;
                for (const auto& sprite : game.sprites)
                    if (sprite.layer < minLayer) minLayer = sprite.layer;

                activeSprite->layer = minLayer - 1;
                log_info(("Sprite moved to back layer: " + to_string(activeSprite->layer)).c_str());
                scriptPC++;
                continue;
            }

            if (b.type == GO_FORWARD_LAYERS)
            {
                int steps = b.parameters.empty() ? 1 : (int)b.parameters[0].asNumber();
                activeSprite->layer += steps;
                log_info(("Sprite moved forward " + to_string(steps) + " layers").c_str());
                scriptPC++;
                continue;
            }

            if (b.type == GO_BACKWARD_LAYERS)
            {
                int steps = b.parameters.empty() ? 1 : (int)b.parameters[0].asNumber();
                activeSprite->layer -= steps;
                log_info(("Sprite moved backward " + to_string(steps) + " layers").c_str());
                scriptPC++;
                continue;
            }

            // ===== بلوک‌های صدا =====
            if (b.type == PLAY_SOUND || b.type == PLAY_SOUND_UNTIL_DONE)
            {
                if (b.type == PLAY_SOUND)
                {
                    if (game.soundEffect)
                    {
                        int volumeLevel = (game.volume * MIX_MAX_VOLUME) / 100;
                        Mix_VolumeChunk(game.soundEffect, volumeLevel);
                        Mix_PlayChannel(-1, game.soundEffect, 0);
                        log_info("Playing sound");
                    }
                    else
                    {
                        log_error("No sound loaded to play");
                    }
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }
                else
                {
                    if (!game.isWaiting)
                    {
                        if (game.soundEffect)
                        {
                            int volumeLevel = (game.volume * MIX_MAX_VOLUME) / 100;
                            Mix_VolumeChunk(game.soundEffect, volumeLevel);
                            game.soundChannel = Mix_PlayChannel(-1, game.soundEffect, 0);
                            log_info("Playing sound until done");

                            if (game.soundChannel == -1)
                            {
                                log_error(("Error playing sound: " + string(Mix_GetError())).c_str());
                            }
                        }

                        game.waitStartTime = SDL_GetTicks();
                        game.waitDuration = 1000;
                        game.isWaiting = true;
                        return;
                    }

                    if (SDL_GetTicks() - game.waitStartTime >= game.waitDuration)
                    {
                        game.isWaiting = false;
                        game.lastExecutedBlock = scriptPC;
                        scriptPC++;
                    }
                    return;
                }
                continue;
            }

            if (b.type == STOP_ALL_SOUNDS)
            {
                Mix_HaltChannel(-1);
                game.isPlayingSound = false;
                log_info("Stopping all sounds");
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == CHANGE_VOLUME)
            {
                if (!b.parameters.empty())
                {
                    int change = (int)b.parameters[0].asNumber();
                    game.volume += change;
                    if (game.volume < 0) game.volume = 0;
                    if (game.volume > 100) game.volume = 100;
                    log_info(("Volume changed to: " + to_string(game.volume) + "%").c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == SET_VOLUME)
            {
                if (!b.parameters.empty())
                {
                    int newVolume = (int)b.parameters[0].asNumber();
                    if (newVolume < 0) newVolume = 0;
                    if (newVolume > 100) newVolume = 100;
                    game.volume = newVolume;
                    log_info(("Volume set to: " + to_string(game.volume) + "%").c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // ===== بلوک‌های عملگر =====
            if (b.type >= OP_ADD && b.type <= OP_XOR)
            {
                Value result = evaluateOperator(b.type, b.parameters);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                safetyCounter++;
                continue;
            }

            // ===== بلوک‌های متغیر =====
            if (b.type == SET_VARIABLE)
            {
                if (!b.parameters.empty() && !b.variableName.empty())
                {
                    game.variables[b.variableName] = b.parameters[0];
                    log_info(("Variable set: " + b.variableName).c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                safetyCounter++;
                continue;
            }

            if (b.type == CHANGE_VARIABLE)
            {
                if (!b.parameters.empty() && !b.variableName.empty())
                {
                    if (game.variables.find(b.variableName) == game.variables.end())
                    {
                        game.variables[b.variableName] = Value(0.0);
                    }
                    Value current = game.variables[b.variableName];
                    double newVal = current.asNumber() + b.parameters[0].asNumber();
                    game.variables[b.variableName] = Value(newVal);
                    log_info(("Variable changed: " + b.variableName).c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                safetyCounter++;
                continue;
            }

            if (b.type == COSTUME_NUMBER)
            {
                if (!b.variableName.empty() && activeSprite)
                {
                    game.variables[b.variableName] = Value((double)activeSprite->index);
                    log_info(("Costume number set to variable: " + to_string(activeSprite->index)).c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == SPRITE_SIZE)
            {
                if (!b.variableName.empty() && activeSprite)
                {
                    game.variables[b.variableName] = Value((double)activeSprite->w);
                    log_info(("Sprite size set to variable: " + to_string(activeSprite->w)).c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            if (b.type == BACKDROP_NUMBER)
            {
                if (!b.variableName.empty())
                {
                    game.variables[b.variableName] = Value((double)game.currentBackdrop);
                    log_info(("Backdrop number set to variable: " + to_string(game.currentBackdrop)).c_str());
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }

            // ===== بلوک‌های حرکتی =====
            if (b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT ||
                b.type == TURN_RIGHT || b.type == TURN_LEFT || b.type == GOTO_XY || b.type == CHANGE_X ||
                b.type == CHANGE_Y || b.type == SET_X || b.type == SET_Y || b.type == GOTO_RANDOM || b.type == GOTO_MOUSE)
            {
                if (b.type == MOVE_UP || b.type == MOVE_DOWN || b.type == MOVE_LEFT || b.type == MOVE_RIGHT)
                {
                    if (!game.isExecutingBlock)
                    {
                        if (!b.parameters.empty())
                            game.remainingMove = b.parameters[0].asNumber();
                        else
                            game.remainingMove = 5;
                        game.isExecutingBlock = true;
                    }

                    double moveAmount = min(speed, game.remainingMove);

                    switch (b.type)
                    {
                        case MOVE_UP:    activeSprite->y -= moveAmount; break;
                        case MOVE_DOWN:  activeSprite->y += moveAmount; break;
                        case MOVE_LEFT:  activeSprite->x -= moveAmount; break;
                        case MOVE_RIGHT: activeSprite->x += moveAmount; break;
                        default: break;
                    }

                    if (game.penDown)
                    {
                        int stageX = game.stageX + (int)((activeSprite->x + activeSprite->w/2) / game.screenWidth * game.stageW);
                        int stageY = game.stageY + (int)((activeSprite->y + activeSprite->h/2) / game.screenHeight * game.stageH);

                        PenPoint p;
                        p.x = stageX;
                        p.y = stageY;
                        p.r = game.penR;
                        p.g = game.penG;
                        p.b = game.penB;
                        p.a = 255;
                        p.size = game.penSize;
                        game.penPoints.push_back(p);
                    }

                    game.remainingMove -= moveAmount;

                    if (game.remainingMove <= 0)
                    {
                        game.isExecutingBlock = false;
                        game.lastExecutedBlock = scriptPC;
                        scriptPC++;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    if (b.type == TURN_RIGHT)
                    {
                        if (!b.parameters.empty())
                            activeSprite->direction += b.parameters[0].asNumber();
                    }
                    else if (b.type == TURN_LEFT)
                    {
                        if (!b.parameters.empty())
                            activeSprite->direction -= b.parameters[0].asNumber();
                    }
                    else if (b.type == GOTO_XY)
                    {
                        if (b.parameters.size() >= 2)
                        {
                            activeSprite->x = b.parameters[0].asNumber();
                            activeSprite->y = b.parameters[1].asNumber();
                            if (game.penDown)
                            {
                                int stageX = game.stageX + (int)((activeSprite->x + activeSprite->w/2) / game.screenWidth * game.stageW);
                                int stageY = game.stageY + (int)((activeSprite->y + activeSprite->h/2) / game.screenHeight * game.stageH);

                                PenPoint p;
                                p.x = stageX;
                                p.y = stageY;
                                p.r = game.penR; p.g = game.penG; p.b = game.penB; p.a = 255; p.size = game.penSize;
                                game.penPoints.push_back(p);
                            }
                            log_info(("Goto: x=" + to_string(activeSprite->x) + " y=" + to_string(activeSprite->y)).c_str());
                        }
                    }
                    else if (b.type == CHANGE_X)
                    {
                        if (!b.parameters.empty())
                        {
                            activeSprite->x += b.parameters[0].asNumber();
                            if (game.penDown)
                            {
                                int stageX = game.stageX + (int)((activeSprite->x + activeSprite->w/2) / game.screenWidth * game.stageW);
                                int stageY = game.stageY + (int)((activeSprite->y + activeSprite->h/2) / game.screenHeight * game.stageH);

                                PenPoint p;
                                p.x = stageX;
                                p.y = stageY;
                                p.r = game.penR; p.g = game.penG; p.b = game.penB; p.a = 255; p.size = game.penSize;
                                game.penPoints.push_back(p);
                            }
                        }
                    }
                    else if (b.type == CHANGE_Y)
                    {
                        if (!b.parameters.empty())
                        {
                            activeSprite->y += b.parameters[0].asNumber();
                            if (game.penDown)
                            {
                                int stageX = game.stageX + (int)((activeSprite->x + activeSprite->w/2) / game.screenWidth * game.stageW);
                                int stageY = game.stageY + (int)((activeSprite->y + activeSprite->h/2) / game.screenHeight * game.stageH);

                                PenPoint p;
                                p.x = stageX;
                                p.y = stageY;
                                p.r = game.penR; p.g = game.penG; p.b = game.penB; p.a = 255; p.size = game.penSize;
                                game.penPoints.push_back(p);
                            }
                        }
                    }
                    else if (b.type == SET_X)
                    {
                        if (!b.parameters.empty())
                            activeSprite->x = b.parameters[0].asNumber();
                    }
                    else if (b.type == SET_Y)
                    {
                        if (!b.parameters.empty())
                            activeSprite->y = b.parameters[0].asNumber();
                    }
                    else if (b.type == GOTO_RANDOM)
                    {
                        activeSprite->x = rand() % (game.screenWidth - activeSprite->w);
                        activeSprite->y = rand() % (game.screenHeight - activeSprite->h);
                        log_info("Goto random position");
                    }
                    else if (b.type == GOTO_MOUSE)
                    {
                        int categoriesPanelWidth = 180;
                        int examplesPanelX = categoriesPanelWidth + 30;
                        int examplesPanelWidth = 250;
                        int codeAreaX = examplesPanelX + examplesPanelWidth + 20;
                        int codeAreaWidth = 600;
                        int stagePanelX = codeAreaX + codeAreaWidth + 10;
                        int stagePanelWidth = game.screenWidth - stagePanelX - 5;
                        if (stagePanelWidth > 500) stagePanelWidth = 500;

                        int stageX = stagePanelX + 5;
                        int stageY = 70;
                        int stageWidth = stagePanelWidth - 20;
                        int stageHeight = 300;

                        if (game.mouseX >= stageX && game.mouseX <= stageX + stageWidth &&
                            game.mouseY >= stageY && game.mouseY <= stageY + stageHeight)
                        {
                            activeSprite->x = (double)(game.mouseX - stageX) * game.screenWidth / stageWidth;
                            activeSprite->y = (double)(game.mouseY - stageY) * game.screenHeight / stageHeight;
                        }
                        else
                        {
                            activeSprite->x = game.screenWidth / 2 - activeSprite->w / 2;
                            activeSprite->y = game.screenHeight / 2 - activeSprite->h / 2;
                        }

                        log_info("Goto mouse position");
                    }

                    if (activeSprite->x < 0) activeSprite->x = 0;
                    if (activeSprite->y < 0) activeSprite->y = 0;
                    if (activeSprite->x + activeSprite->w > game.screenWidth)
                        activeSprite->x = game.screenWidth - activeSprite->w;
                    if (activeSprite->y + activeSprite->h > game.screenHeight)
                        activeSprite->y = game.screenHeight - activeSprite->h;
                    game.lastExecutedBlock = scriptPC;
                    scriptPC++;
                }

                safetyCounter++;
                continue;
            }

            // ===== بلوک‌های ترسیمی (Pen) =====
            if (b.type == PEN_ERASE_ALL)
            {
                penEraseAll(game);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_STAMP)
            {
                if (activeSprite)
                    penStamp(activeSprite, game, renderer);
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_PEN_DOWN)
            {
                game.penDown = true;
                log_info("Pen down");
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_PEN_UP)
            {
                game.penDown = false;
                log_info("Pen up");
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_SET_COLOR_PARAM)
            {
                if (!b.parameters.empty())
                {
                    double val = b.parameters[0].asNumber();
                    penSetParam(game, b.penParam, val);
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_CHANGE_COLOR_PARAM)
            {
                if (!b.parameters.empty())
                {
                    double delta = b.parameters[0].asNumber();
                    penChangeParam(game, b.penParam, delta);
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_SET_SIZE)
            {
                if (!b.parameters.empty())
                {
                    int newSize = (int)b.parameters[0].asNumber();
                    penSetSize(game, newSize);
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            else if (b.type == PEN_CHANGE_SIZE)
            {
                if (!b.parameters.empty())
                {
                    int delta = (int)b.parameters[0].asNumber();
                    penChangeSize(game, delta);
                }
                game.lastExecutedBlock = scriptPC;
                scriptPC++;
                continue;
            }
            game.lastExecutedBlock = scriptPC;
            scriptPC++;
            safetyCounter++;
        }
    }

    // اجرای اسکریپت‌های فعال

    // پاکسازی اسکریپت‌های غیرفعال
    for (int s = game.scriptActive.size() - 1; s >= 0; s--)
    {
        if (!game.scriptActive[s])
        {
            game.scriptStartIndices.erase(game.scriptStartIndices.begin() + s);
            game.scriptActive.erase(game.scriptActive.begin() + s);
            game.scriptCurrentBlock.erase(game.scriptCurrentBlock.begin() + s);
        }
    }
}