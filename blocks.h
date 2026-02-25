#ifndef BLOCKS_H
#define BLOCKS_H

#include <vector>
#include <string>
#include "value.h"

using namespace std;

enum CompareOp {
    CMP_EQUAL,
    CMP_NOT_EQUAL,
    CMP_LESS,
    CMP_LESS_OR_EQUAL,
    CMP_GREATER,
    CMP_GREATER_OR_EQUAL,
};

enum BlockType
{
    MOVE_UP,
    COSTUME_NUMBER,
    BACKDROP_NUMBER,
    SPRITE_SIZE,
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
    SET_COMPARISON,

    REPEAT,
    END_REPEAT,
    FOREVER,
    END_FOREVER,
    WAIT,
    IF_THEN,
    IF_THEN_ELSE,
    ELSE,
    END_IF,
    WAIT_UNTIL,
    REPEAT_UNTIL,
    STOP_ALL,
    STOP_THIS_SCRIPT,

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

    SAY,
    SAY_FOR,
    THINK,
    THINK_FOR,
    SHOW,
    HIDE,
    CHANGE_SIZE,
    SET_SIZE,

    SET_VARIABLE,
    CHANGE_VARIABLE,
    SHOW_VARIABLE,
    HIDE_VARIABLE,

    WHEN_GREEN_FLAG,
    WHEN_KEY_PRESSED,
    WHEN_SPRITE_CLICKED,
    WHEN_I_RECEIVE,
    BROADCAST,
    BROADCAST_AND_WAIT,

    PLAY_SOUND,
    PLAY_SOUND_UNTIL_DONE,
    STOP_ALL_SOUNDS,
    CHANGE_VOLUME,
    SET_VOLUME,

    PEN_ERASE_ALL,
    PEN_STAMP,
    PEN_PEN_UP,
    PEN_PEN_DOWN,
    PEN_SET_COLOR,
    PEN_CHANGE_COLOR,
    PEN_SET_COLOR_PARAM,
    PEN_CHANGE_COLOR_PARAM,
    PEN_SET_SIZE,
    PEN_CHANGE_SIZE,
    GO_TO_FRONT_LAYER,
    GO_TO_BACK_LAYER,
    GO_FORWARD_LAYERS,
    GO_BACKWARD_LAYERS,

    SENSOR_TOUCHING_EDGE,
    SENSOR_TOUCHING_MOUSE,
    SENSOR_TOUCHING_COLOR,
    SENSOR_KEY_PRESSED,
    SENSOR_MOUSE_DOWN,
    SENSOR_MOUSE_X,
    SENSOR_MOUSE_Y,
    SENSOR_TIMER,
    SENSOR_RESET_TIMER,
    SENSOR_ASK_AND_WAIT,
    SENSOR_ANSWER,
    SENSOR_DISTANCE_TO_MOUSE,
    SENSOR_DISTANCE_TO_SPRITE,
    SET_VARIABLE_TO_SENSOR,
};

enum BlockCategory
{
    CAT_MOTION,
    CAT_LOOKS,
    CAT_SOUND,
    CAT_EVENTS,
    CAT_CONTROL,
    CAT_SENSING = 5,
    CAT_OPERATORS,
    CAT_VARIABLES,
    CAT_PEN
};

enum SensorType
{
    SENSOR_TYPE_TOUCHING_EDGE,
    SENSOR_TYPE_TOUCHING_MOUSE,
    SENSOR_TYPE_KEY_PRESSED,
    SENSOR_TYPE_MOUSE_DOWN,
    SENSOR_TYPE_MOUSE_X,
    SENSOR_TYPE_MOUSE_Y,
    SENSOR_TYPE_TIMER,
    SENSOR_TYPE_ANSWER,
    SENSOR_TYPE_DISTANCE_TO_MOUSE,
    SENSOR_TYPE_DISTANCE_TO_SPRITE,
};

enum PenColorParam
{
    PEN_PARAM_COLOR,
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
    string messageName;
    int keyCode;
    CompareOp compareOp;
    string leftVar;
    string rightVar;

    int ifTrueJump;
    int ifFalseJump;
    int endIfIndex;

    bool editingMode;
    int editingField;
    string editingBuffer;

    int x, y;
    int width, height;
    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;
    Block* parent;
    vector<Block*> children;
    bool inCodeArea;

    int paletteId;
    PenColorParam penParam;

    SensorType sensorType;
    string sensorParam;
    Uint8 sensorColorR, sensorColorG, sensorColorB;

    Block() :
            type(MOVE_UP),
            category(CAT_MOTION),
            repeatCount(0),
            keyCode(0),
            ifTrueJump(-1),
            ifFalseJump(-1),
            endIfIndex(-1),
            editingMode(false),
            editingField(-1),
            editingBuffer(""),
            x(0), y(0), width(0), height(0),
            isDragging(false),
            dragOffsetX(0), dragOffsetY(0),
            parent(nullptr),
            inCodeArea(false),
            paletteId(-1),
            penParam(PEN_PARAM_COLOR),
            sensorType(SENSOR_TYPE_TOUCHING_EDGE),
            sensorParam(""),
            sensorColorR(0), sensorColorG(0), sensorColorB(0),
            compareOp(CMP_EQUAL),          // <-- مقداردهی
            leftVar(""),
            rightVar("")
    {}

    Block(BlockType t, BlockCategory cat) :
            type(t),
            category(cat),
            repeatCount(0),
            keyCode(0),
            ifTrueJump(-1),
            ifFalseJump(-1),
            endIfIndex(-1),
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
            penParam(PEN_PARAM_COLOR),
            sensorType(SENSOR_TYPE_TOUCHING_EDGE),
            sensorParam(""),
            sensorColorR(0), sensorColorG(0), sensorColorB(0)
    {}
};

#endif