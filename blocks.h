#ifndef BLOCKS_H
#define BLOCKS_H

#include <vector>
#include <string>
#include "value.h"

using namespace std;

enum BlockType
{
    // حرکتی (آبی)
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,

    // کنترلی (نارنجی)
    REPEAT,
    END_REPEAT,
    FOREVER,
    END_FOREVER,
    WAIT,

    // عملگرها (سبز)
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EQUAL,
    OP_LESS_THAN,
    OP_GREATER_THAN,
    OP_NOT,
    OP_OR,
    OP_AND,
    OP_JOIN_STRINGS,
    OP_STRING_LENGTH,
    OP_LETTER_OF,
    OP_ABS,
    OP_SQRT,
    OP_FLOOR,
    OP_CEIL,
    OP_SIN,
    OP_COS,
    OP_MOD,
    OP_XOR,

    // ظاهری (نیلی)
    SAY,
    SAY_FOR,
    THINK,
    THINK_FOR,
    SHOW,
    HIDE,
    CHANGE_SIZE,
    SET_SIZE,

    // متغیرها
    SET_VARIABLE,
    CHANGE_VARIABLE,
    SHOW_VARIABLE,
    HIDE_VARIABLE,

    TURN_RIGHT,
    TURN_LEFT,
    GOTO_XY,
    CHANGE_X,
    CHANGE_Y,
    SET_X,
    SET_Y,
    POINT_DIRECTION,
    GOTO_RANDOM,
    GOTO_MOUSE,

    WHEN_GREEN_FLAG,
    WHEN_KEY_PRESSED,
    WHEN_SPRITE_CLICKED,
    BROADCAST,
    WHEN_BROADCAST,

    TOUCHING_MOUSE,
    TOUCHING_EDGE,
    TOUCHING_SPRITE,
    TOUCHING_COLOR,
    DISTANCE_TO_MOUSE,
    DISTANCE_TO_SPRITE,
    ASK,
    ANSWER,
    KEY_PRESSED,
    MOUSE_DOWN,
    MOUSE_X,
    MOUSE_Y,
    DRAG_MODE,
    TIMER,
    RESET_TIMER,

    PEN_CLEAR,
    PEN_STAMP,
    PEN_DOWN,
    PEN_UP,
    SET_PEN_COLOR,
    CHANGE_PEN_COLOR,
    SET_PEN_SIZE,
    CHANGE_PEN_SIZE
};

struct Block
{
    BlockType type;
    vector<Value> parameters;
    int repeatCount;
    string variableName;
    string eventName;     // برای BROADCAST و WHEN_BROADCAST و ذخیره نام بلوک
    int keyCode;          // برای WHEN_KEY_PRESSED

    // برای ویرایش بلوک - این سه خط رو اضافه کن
    bool editingMode;     // آیا در حال ویرایش هستیم؟
    int editingField;     // کدوم فیلد در حال ویرایشه (0=پارامتر اول، 1=پارامتر دوم، ...)
    string editingBuffer; // متن در حال ویرایش

    Block() : repeatCount(0), keyCode(0), editingMode(false), editingField(-1) {}
    Block(BlockType t) : type(t), repeatCount(0), keyCode(0), editingMode(false), editingField(-1) {}
};

#endif