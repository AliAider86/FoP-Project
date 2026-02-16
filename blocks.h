#ifndef BLOCKS_H
#define BLOCKS_H

enum BlockType
{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    REPEAT
};

struct Block
{
    BlockType type;
    int value;
    int repeatCount;
};

#endif
