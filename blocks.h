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

    TURN_RIGHT,        // چرخش به راست
    TURN_LEFT,         // چرخش به چپ
    GOTO_XY,           // رفتن به مختصات مشخص
    CHANGE_X,          // تغییر X
    CHANGE_Y,          // تغییر Y
    SET_X,             // تنظیم X
    SET_Y,             // تنظیم Y
    POINT_DIRECTION,   // تنظیم جهت
    GOTO_RANDOM,       // رفتن به موقعیت تصادفی
    GOTO_MOUSE,        // رفتن به موقعیت ماوس

    WHEN_GREEN_FLAG,     // وقتی پرچم سبز زده شد
    WHEN_KEY_PRESSED,    // وقتی کلیدی فشرده شد
    WHEN_SPRITE_CLICKED, // وقتی اسپرایت کلیک شد
    BROADCAST,           // ارسال پیام
    WHEN_BROADCAST,      // وقتی پیام رسید

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

    PEN_CLEAR,        // پاک کردن همه
    PEN_STAMP,        // مهر زدن
    PEN_DOWN,         // قلم پایین
    PEN_UP,           // قلم بالا
    SET_PEN_COLOR,    // تنظیم رنگ قلم
    CHANGE_PEN_COLOR, // تغییر رنگ قلم
    SET_PEN_SIZE,     // تنظیم ضخامت قلم
    CHANGE_PEN_SIZE   // تغییر ضخامت قلم
};

struct Block
{
    BlockType type;
    vector<Value> parameters;
    int repeatCount;
    string variableName;
    string eventName;     // برای BROADCAST و WHEN_BROADCAST
    int keyCode;          // برای WHEN_KEY_PRESSED

    Block() : repeatCount(0), keyCode(0) {}
    Block(BlockType t) : type(t), repeatCount(0), keyCode(0) {}
};

#endif