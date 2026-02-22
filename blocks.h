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

    // رویدادها (زرد) - WITHOUT BROADCAST
    WHEN_GREEN_FLAG,
    WHEN_KEY_PRESSED,
    WHEN_SPRITE_CLICKED,

    // صدا (بنفش)
    PLAY_SOUND,
    PLAY_SOUND_UNTIL_DONE,
    STOP_ALL_SOUNDS,
    CHANGE_VOLUME,
    SET_VOLUME
};

struct Block
{
    BlockType type;
    vector<Value> parameters;
    int repeatCount;
    string variableName;
    string eventName;
    int keyCode;

    bool editingMode;
    int editingField;
    string editingBuffer;

    Block() : repeatCount(0), keyCode(0), editingMode(false), editingField(-1) {}
    Block(BlockType t) : type(t), repeatCount(0), keyCode(0), editingMode(false), editingField(-1) {}
};

#endif