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

    // رویدادها (زرد)
    WHEN_GREEN_FLAG,
    WHEN_KEY_PRESSED,
    WHEN_SPRITE_CLICKED,

    // صدا (بنفش)
    PLAY_SOUND,
    PLAY_SOUND_UNTIL_DONE,
    STOP_ALL_SOUNDS,
    CHANGE_VOLUME,
    SET_VOLUME,

    // ===== بلوک‌های ترسیمی (Pen) - رنگ سبز پررنگ =====
    PEN_ERASE_ALL,
    PEN_STAMP,
    PEN_PEN_UP,
    PEN_PEN_DOWN,
    PEN_SET_COLOR,          // تنظیم رنگ با انتخاب مستقیم
    PEN_CHANGE_COLOR,       // تغییر رنگ با مقدار
    PEN_SET_COLOR_PARAM,    // تنظیم یکی از پارامترهای رنگ (رنگ اصلی، روشنایی، اشباع)
    PEN_CHANGE_COLOR_PARAM, // تغییر پارامتر رنگ
    PEN_SET_SIZE,           // تنظیم ضخامت قلم
    PEN_CHANGE_SIZE,        // تغییر ضخامت
};

enum BlockCategory
{
    CAT_MOTION,      // 0
    CAT_LOOKS,       // 1
    CAT_SOUND,       // 2
    CAT_EVENTS,      // 3
    CAT_CONTROL,     // 4
    CAT_SENSING,     // 5
    CAT_OPERATORS,   // 6
    CAT_VARIABLES,   // 7
    CAT_PEN          // 8  (دسته‌بندی جدید برای Pen)
};

// برای تنظیم پارامترهای رنگ می‌توانیم از enum استفاده کنیم
enum PenColorParam
{
    PEN_PARAM_COLOR,      // hue
    PEN_PARAM_SATURATION,
    PEN_PARAM_BRIGHTNESS
};

struct Block
{
    BlockType type;
    BlockCategory category;
    vector<Value> parameters;
    int repeatCount;
    string variableName;
    string eventName;
    int keyCode;

    bool editingMode;
    int editingField;
    string editingBuffer;

    // فیلدهای Drag & Drop
    int x, y;
    int width, height;
    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;
    Block* parent;
    vector<Block*> children;
    bool inCodeArea;

    int paletteId;

    // فیلدهای اضافی برای Pen (مثلاً برای تعیین پارامتر رنگ)
    PenColorParam penParam;

    Block() :
            type(MOVE_UP),
            category(CAT_MOTION),
            repeatCount(0),
            keyCode(0),
            editingMode(false),
            editingField(-1),
            editingBuffer(""),
            x(0),
            y(0),
            width(0),
            height(0),
            isDragging(false),
            dragOffsetX(0),
            dragOffsetY(0),
            parent(nullptr),
            inCodeArea(false),
            paletteId(-1),
            penParam(PEN_PARAM_COLOR)
    {}

    Block(BlockType t, BlockCategory cat) :
            type(t),
            category(cat),
            repeatCount(0),
            keyCode(0),
            editingMode(false),
            editingField(-1),
            editingBuffer(""),
            x(0),
            y(0),
            width(0),
            height(0),
            isDragging(false),
            dragOffsetX(0),
            dragOffsetY(0),
            parent(nullptr),
            inCodeArea(false),
            paletteId(-1),
            penParam(PEN_PARAM_COLOR)
    {}
};

#endif