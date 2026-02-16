#ifndef BLOCKS_H
#define BLOCKS_H

enum BlockType
{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    REPEAT,
    END_REPEAT,
    FOREVER,
    END_FOREVER,
    WAIT
};

struct Block
{
    BlockType type;
    double value;
    int repeatCount;
};

#endif
