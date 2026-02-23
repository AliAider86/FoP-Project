#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <SDL2/SDL_image.h>
#include "engine.h"
#include "operators.h"
#include "logger.h"

using namespace std;

extern bool loadSpriteTexture(Sprite* sprite, SDL_Renderer* renderer, const char* path);

// ==================== توابع Pen ====================

void updatePenColorFromHSV(GameState& game)
{
    // تبدیل HSV به RGB
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

    // محاسبه مختصات استیج در همان لحظه
    int stageX = game.stageX + (int)((sprite->x + sprite->w/2) / game.screenWidth * game.stageW);
    int stageY = game.stageY + (int)((sprite->y + sprite->h/2) / game.screenHeight * game.stageH);

    PenPoint p;
    p.x = stageX;  // ذخیره مختصات استیج
    p.y = stageY;
    p.r = game.penR;
    p.g = game.penG;
    p.b = game.penB;
    p.a = 255;
    p.size = max(5, (sprite->w + sprite->h) / 10); // حداقل 5 پیکسل
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

    // ===== بلوک‌های کنترل (نارنجی) - CAT_CONTROL =====
    startY = 110;

    Block waitBlock(WAIT, CAT_CONTROL);
    waitBlock.eventName = "wait 1 seconds";
    waitBlock.parameters.push_back(Value(1.0));
    waitBlock.x = xPos;
    waitBlock.y = startY;
    waitBlock.width = blockWidth;
    waitBlock.height = blockHeight;
    waitBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(waitBlock);

    Block repeatBlock(REPEAT, CAT_CONTROL);
    repeatBlock.eventName = "repeat 10";
    repeatBlock.repeatCount = 10;
    repeatBlock.x = xPos;
    repeatBlock.y = startY + 45;
    repeatBlock.width = blockWidth;
    repeatBlock.height = blockHeight;
    repeatBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(repeatBlock);

    Block foreverBlock(FOREVER, CAT_CONTROL);
    foreverBlock.eventName = "forever";
    foreverBlock.x = xPos;
    foreverBlock.y = startY + 90;
    foreverBlock.width = blockWidth;
    foreverBlock.height = blockHeight;
    foreverBlock.paletteId = paletteId++;
    game.paletteBlocks.push_back(foreverBlock);

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

    // Set Pen Color (با انتخاب مستقیم - می‌توانیم یک دکمه با پارامتر رنگ داشته باشیم)
    // اما فعلاً یک بلوک ساده با پارامتر عددی برای تنظیم رنگ اصلی (Hue)
    Block setColorBlock(PEN_SET_COLOR_PARAM, CAT_PEN);
    setColorBlock.eventName = "set pen color to 0";
    setColorBlock.parameters.push_back(Value(0.0)); // Hue
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

    // جدا کردن از والد قبلی
    if (child->parent)
    {
        auto& siblings = child->parent->children;
        siblings.erase(remove(siblings.begin(), siblings.end(), child), siblings.end());
    }

    // چسباندن به والد جدید
    child->parent = parent;
    parent->children.push_back(child);

    // تنظیم موقعیت
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

    Backdrop b;
    string path = filePath;
    size_t pos = path.find_last_of("/\\");
    string fileName = (pos != string::npos) ? path.substr(pos + 1) : path;
    size_t dotPos = fileName.find_last_of(".");
    string nameWithoutExt = (dotPos != string::npos) ? fileName.substr(0, dotPos) : fileName;

    b.name = "Custom: " + nameWithoutExt;
    b.filePath = filePath;
    b.isCustom = true;

    SDL_Surface* surface = IMG_Load(filePath);
    if (surface)
    {
        b.texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        game.backdrops.push_back(b);
        game.currentBackdrop = game.backdrops.size() - 1;
        log_info(("Custom backdrop added: " + string(filePath)).c_str());
    }
    else
    {
        log_error(("Failed to load custom backdrop: " + string(filePath) + " - " + string(IMG_GetError())).c_str());
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
    newSprite.imagePath = imagePath ? imagePath : "";
    newSprite.index = game.sprites.size();
    newSprite.isActive = false;

    if (imagePath && renderer && !newSprite.imagePath.empty())
    {
        loadSpriteTexture(&newSprite, renderer, newSprite.imagePath.c_str());
    }

    game.sprites.push_back(newSprite);
    log_info(("Sprite added: " + string(name)).c_str());
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
        file << "Sprite "
             << s.x << " " << s.y << " "
             << s.w << " " << s.h << " "
             << s.visible << " " << s.direction << " "
             << s.name << " " << s.imagePath << " "
             << s.index << " " << s.isActive << "\n";
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
                file << "str " << v.asString() << " ";
            else if (v.type == VALUE_BOOLEAN)
                file << "bool " << v.asBoolean() << " ";
        }
        file << b.repeatCount << " " << b.variableName << " "
             << b.eventName << " " << b.keyCode << " "
             << b.editingMode << " " << b.editingField << " "
             << b.editingBuffer << "\n";
    }

    // Variables
    file << "Variables " << game.variables.size() << "\n";
    for (const auto& var : game.variables)
    {
        file << var.first << " ";
        if (var.second.type == VALUE_NUMBER)
            file << "num " << var.second.asNumber() << "\n";
        else if (var.second.type == VALUE_STRING)
            file << "str " << var.second.asString() << "\n";
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

    for (auto& s : game.sprites)
        if (s.texture) SDL_DestroyTexture(s.texture);
    for (auto& b : game.backdrops)
        if (b.texture) SDL_DestroyTexture(b.texture);

    game.sprites.clear();
    game.program.clear();
    game.variables.clear();
    game.backdrops.clear();
    game.scriptStartIndices.clear();
    game.scriptActive.clear();
    game.scriptCurrentBlock.clear();

    game.editingMode = false;
    game.editingField = -1;
    game.editingBuffer = "";

    string token;
    while (file >> token)
    {
        if (token == "Backdrops")
        {
            int count; file >> count;
            for (int i = 0; i < count; i++)
            {
                Backdrop b; string name;
                file >> name >> b.filePath >> b.isCustom;
                for (char& c : name) if (c == '_') c = ' ';
                b.name = name; b.texture = nullptr;
                game.backdrops.push_back(b);
            }
        }
        else if (token == "CurrentBackdrop")
        {
            file >> game.currentBackdrop;
        }
        else if (token == "Sprites")
        {
            int count; file >> count;
        }
        else if (token == "Sprite")
        {
            Sprite s; string name, imagePath, message;
            int index; bool isActive;

            file >> s.x >> s.y >> s.w >> s.h
                 >> s.visible >> s.direction
                 >> name >> imagePath
                 >> index >> isActive;

            s.name = name; s.imagePath = imagePath;
            s.message = ""; s.isThinking = false;
            s.index = index; s.isActive = isActive;
            s.texture = nullptr;

            game.sprites.push_back(s);
        }
        else if (token == "ActiveSprite")
        {
            file >> game.activeSpriteIndex;
        }
        else if (token == "Blocks")
        {
            int count; file >> count;
            for (int i = 0; i < count; i++)
            {
                Block b; int typeInt, paramCount;
                file >> typeInt >> paramCount;
                b.type = (BlockType)typeInt;

                for (int j = 0; j < paramCount; j++)
                {
                    string valType; file >> valType;
                    if (valType == "num")
                    {
                        double d; file >> d;
                        b.parameters.push_back(Value(d));
                    }
                    else if (valType == "str")
                    {
                        string s; file >> s;
                        b.parameters.push_back(Value(s));
                    }
                    else if (valType == "bool")
                    {
                        bool bVal; file >> bVal;
                        b.parameters.push_back(Value(bVal));
                    }
                }

                file >> b.repeatCount >> b.variableName >> b.eventName >> b.keyCode;
                b.editingMode = false; b.editingField = -1; b.editingBuffer = "";
                game.program.push_back(b);
            }
        }
        else if (token == "Variables")
        {
            int count; file >> count;
            for (int i = 0; i < count; i++)
            {
                string varName, valType; file >> varName >> valType;
                if (valType == "num")
                {
                    double d; file >> d;
                    game.variables[varName] = Value(d);
                }
                else if (valType == "str")
                {
                    string s; file >> s;
                    game.variables[varName] = Value(s);
                }
                else if (valType == "bool")
                {
                    bool b; file >> b;
                    game.variables[varName] = Value(b);
                }
            }
        }
    }

    file.close();
    log_info(("Project loaded from " + filename).c_str());
}

void update(GameState& game, SDL_Renderer* renderer)
{
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

    // رویداد Green Flag
    if (game.greenFlagPressed)
    {
        log_info("Green flag pressed - starting program");
        game.greenFlagPressed = false;

        game.scriptStartIndices.clear();
        game.scriptActive.clear();
        game.scriptCurrentBlock.clear();

        game.scriptStartIndices.push_back(0);
        game.scriptActive.push_back(true);
        game.scriptCurrentBlock.push_back(0);
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

    // اجرای اسکریپت‌های فعال
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

        // ===== بلوک‌های کنترلی =====
        if (b.type == REPEAT)
        {
            game.repeatCountStack.push_back(b.repeatCount);
            game.repeatStartStack.push_back(scriptPC + 1);
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
                    scriptPC++;
                }
            }
            safetyCounter++;
            continue;
        }

        if (b.type == FOREVER)
        {
            game.repeatStartStack.push_back(scriptPC + 1);
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
                scriptPC++;
            }
            return;
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

                        string msgType = activeSprite->isThinking ? "THINK" : "SAY";
                        log_info((msgType + ": " + activeSprite->message).c_str());
                    }
                    return;
                }

                if (SDL_GetTicks() - game.messageStartTime >= game.messageDuration)
                {
                    activeSprite->message = "";
                    game.isShowingMessage = false;
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
                    string msgType = activeSprite->isThinking ? "THINK" : "SAY";
                    log_info((msgType + ": " + activeSprite->message).c_str());
                }
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
            }
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
                activeSprite->w = newSize;
                activeSprite->h = newSize;
            }
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
            scriptPC++;
            continue;
        }

        // ===== بلوک‌های عملگر =====
        if (b.type >= OP_ADD && b.type <= OP_XOR)
        {
            Value result = evaluateOperator(b.type, b.parameters);
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
            scriptPC++;
            safetyCounter++;
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

                // اگر pen پایین است، نقطه رسم کن
                if (game.penDown)
                {
                    PenPoint p;
                    p.x = (int)(activeSprite->x + activeSprite->w/2);
                    p.y = (int)(activeSprite->y + activeSprite->h/2);
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
                            PenPoint p;
                            p.x = (int)(activeSprite->x + activeSprite->w/2);
                            p.y = (int)(activeSprite->y + activeSprite->h/2);
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
                            PenPoint p;
                            p.x = (int)(activeSprite->x + activeSprite->w/2);
                            p.y = (int)(activeSprite->y + activeSprite->h/2);
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
                            PenPoint p;
                            p.x = (int)(activeSprite->x + activeSprite->w/2);
                            p.y = (int)(activeSprite->y + activeSprite->h/2);
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

                // محدودیت مرزها
                if (activeSprite->x < 0) activeSprite->x = 0;
                if (activeSprite->y < 0) activeSprite->y = 0;
                if (activeSprite->x + activeSprite->w > game.screenWidth)
                    activeSprite->x = game.screenWidth - activeSprite->w;
                if (activeSprite->y + activeSprite->h > game.screenHeight)
                    activeSprite->y = game.screenHeight - activeSprite->h;

                scriptPC++;
            }

            safetyCounter++;
            continue;
        }

        // ===== بلوک‌های ترسیمی (Pen) =====
        if (b.type == PEN_ERASE_ALL)
        {
            penEraseAll(game);
            scriptPC++;
            continue;
        }
        else if (b.type == PEN_STAMP)
        {
            if (activeSprite)
                penStamp(activeSprite, game, renderer);
            scriptPC++;
            continue;
        }
        else if (b.type == PEN_PEN_DOWN)
        {
            game.penDown = true;
            log_info("Pen down");
            scriptPC++;
            continue;
        }
        else if (b.type == PEN_PEN_UP)
        {
            game.penDown = false;
            log_info("Pen up");
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
            scriptPC++;
            continue;
        }

        scriptPC++;
        safetyCounter++;
    }

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