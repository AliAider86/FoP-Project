#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <stack>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include "blocks.h"
#include "value.h"

using namespace std;

struct PenPoint
{
    int x, y;
    Uint8 r, g, b, a;
    int size;
};

struct QueuedMessage
{
    string messageName;
    int senderScriptIndex;
    Uint32 broadcastTime;
};

struct IfStackFrame
{
    int scriptIndex;
    int endIfIndex;
    bool inElseBranch;
};

struct Backdrop
{
    SDL_Texture* texture;
    string name;
    string filePath;
    bool isCustom;
};

struct Button
{
    int x, y, w, h;
    bool isPressed;
};

struct Sprite
{
    // فیلدهای قبلی
    double x, y;
    int w, h;
    bool visible;
    double direction;
    string message;
    string name;
    bool isThinking;
    SDL_Texture* texture;
    string imagePath;
    int index;
    bool isActive;

    // فیلدهای جدید برای costume
    vector<string> costumes;    // لیست مسیر فایل‌های costume
    int currentCostume;

    Sprite() :
            x(0), y(0), w(50), h(50), visible(true), direction(0),
            message(""), name(""), isThinking(false), texture(nullptr),
            imagePath(""), index(0), isActive(false),
            currentCostume(0)  // مقداردهی اولیه
    {}
};

struct GameState
{
    vector<Sprite> sprites;
    int activeSpriteIndex;
    vector<Block> program;
    vector<Block> paletteBlocks;
    int currentBlockIndex;
    bool isRunningCode;
    bool stepMode;
    double remainingMove;
    bool isExecutingBlock;
    int screenWidth;
    int screenHeight;

    Button runButton;
    Button pauseButton;
    Button stepButton;
    Button resetButton;
    Button saveButton;
    Button loadButton;
    Button addSpriteBtn;
    Button deleteSpriteBtn;
    Button prevSpriteBtn;
    Button nextSpriteBtn;
    Button uploadBackdropBtn;
    Button prevBackdropBtn;
    Button nextBackdropBtn;
    bool isPaused;

    Button moveCategoryBtn;
    Button looksCategoryBtn;
    Button soundCategoryBtn;
    Button eventsCategoryBtn;
    Button controlCategoryBtn;
    Button sensingCategoryBtn;
    Button operatorsCategoryBtn;
    Button variablesCategoryBtn;
    Button penCategoryBtn;
    int currentCategory;

    vector<int> repeatCountStack;
    vector<int> repeatStartStack;
    stack<IfStackFrame> ifStack;
    bool conditionResult;

    Uint32 waitStartTime;
    Uint32 waitDuration;
    bool isWaiting;
    Uint32 messageStartTime;
    Uint32 messageDuration;
    bool isShowingMessage;

    unordered_map<string, Value> variables;
    bool greenFlagPressed;
    Uint8 pressedKeys[SDL_NUM_SCANCODES];
    Uint8 pressedThisFrame[SDL_NUM_SCANCODES];
    bool spriteClicked;
    int mouseX, mouseY;
    bool mousePressed;

    vector<int> scriptStartIndices;
    vector<bool> scriptActive;
    vector<int> scriptCurrentBlock;

    queue<QueuedMessage> messageQueue;
    unordered_map<string, vector<int>> messageHandlers;
    bool waitingForBroadcast;
    int waitingScriptIndex;
    Uint32 broadcastWaitStartTime;

    string askQuestion;
    string answer;
    bool waitingForAnswer;
    Uint32 timerStartTime;
    bool dragMode;

    Block selectedBlock;
    bool placingBlock;
    bool editingMode;
    int editingField;
    string editingBuffer;
    bool showSpriteName;
    bool showVariables;  // در کنار showSpriteName

    int volume;
    bool isPlayingSound;
    Mix_Chunk* soundEffect;
    int soundChannel;

    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;
    int clickedSpriteIndex;
    SDL_Texture* logoTexture;
    vector<Backdrop> backdrops;
    int currentBackdrop;

    Block* draggedBlock;
    bool isDraggingBlock;
    int dragBlockStartX, dragBlockStartY;
    int dragBlockOffsetX, dragBlockOffsetY;
    Block* hoverBlock;
    int codeAreaX, codeAreaY;
    int codeAreaWidth, codeAreaHeight;
    int categoriesPanelX, categoriesPanelY;
    int categoriesPanelWidth, categoriesPanelHeight;
    int examplesPanelX, examplesPanelY;
    int examplesPanelWidth, examplesPanelHeight;
    Block* draggedBlockOriginalParent;
    int draggedBlockOriginalIndex;
    int trashX, trashY, trashW, trashH;

    bool penDown;
    int penSize;
    Uint8 penR, penG, penB, penA;
    double penHue;
    double penSaturation;
    double penBrightness;
    vector<PenPoint> penPoints;
    int stageX, stageY, stageW, stageH;
    int lastExecutedBlock;

    string currentQuestion;
    string defaultBackdropFolder;

    GameState() :
            activeSpriteIndex(-1),
            defaultBackdropFolder("backdrops/"),
            lastExecutedBlock(-1),
            showVariables(true),
            isPaused(false),
            currentBlockIndex(0),
            isRunningCode(false),
            stepMode(false),
            remainingMove(0),
            isExecutingBlock(false),
            screenWidth(0),
            screenHeight(0),
            currentCategory(0),
            conditionResult(false),
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
            waitingForBroadcast(false),
            waitingScriptIndex(-1),
            broadcastWaitStartTime(0),
            waitingForAnswer(false),
            timerStartTime(0),
            dragMode(false),
            placingBlock(false),
            editingMode(false),
            editingField(-1),
            editingBuffer(""),
            showSpriteName(true),
            volume(100),
            isPlayingSound(false),
            soundEffect(nullptr),
            soundChannel(-1),
            isDragging(false),
            dragOffsetX(0),
            dragOffsetY(0),
            clickedSpriteIndex(-1),
            logoTexture(nullptr),
            currentBackdrop(0),
            draggedBlock(nullptr),
            isDraggingBlock(false),
            dragBlockStartX(0),
            dragBlockStartY(0),
            dragBlockOffsetX(0),
            dragBlockOffsetY(0),
            hoverBlock(nullptr),
            codeAreaX(0),
            codeAreaY(0),
            codeAreaWidth(0),
            codeAreaHeight(0),
            categoriesPanelX(0),
            categoriesPanelY(0),
            categoriesPanelWidth(0),
            categoriesPanelHeight(0),
            examplesPanelX(0),
            examplesPanelY(0),
            examplesPanelWidth(0),
            examplesPanelHeight(0),
            draggedBlockOriginalParent(nullptr),
            draggedBlockOriginalIndex(-1),
            trashX(0),
            trashY(0),
            trashW(60),
            trashH(60),
            penDown(false),
            penSize(1),
            penR(0),
            penG(0),
            penB(0),
            penA(255),
            penHue(0),
            penSaturation(100),
            penBrightness(100),
            stageX(0),
            stageY(0),
            stageW(0),
            stageH(0),
            currentQuestion("")
    {
        memset(pressedKeys, 0, SDL_NUM_SCANCODES);
        memset(pressedThisFrame, 0, SDL_NUM_SCANCODES);

        runButton = {0,0,0,0,0};
        pauseButton = {0,0,0,0,0};
        stepButton = {0,0,0,0,0};
        resetButton = {0,0,0,0,0};
        saveButton = {0,0,0,0,0};
        loadButton = {0,0,0,0,0};
        addSpriteBtn = {0,0,0,0,0};
        deleteSpriteBtn = {0,0,0,0,0};
        prevSpriteBtn = {0,0,0,0,0};
        nextSpriteBtn = {0,0,0,0,0};
        uploadBackdropBtn = {0,0,0,0,0};
        prevBackdropBtn = {0,0,0,0,0};
        nextBackdropBtn = {0,0,0,0,0};
        moveCategoryBtn = {0,0,0,0,0};
        looksCategoryBtn = {0,0,0,0,0};
        soundCategoryBtn = {0,0,0,0,0};
        eventsCategoryBtn = {0,0,0,0,0};
        controlCategoryBtn = {0,0,0,0,0};
        sensingCategoryBtn = {0,0,0,0,0};
        operatorsCategoryBtn = {0,0,0,0,0};
        variablesCategoryBtn = {0,0,0,0,0};
        penCategoryBtn = {0,0,0,0,0};
    }
};

void update(GameState& game, SDL_Renderer* renderer);
void saveProject(const GameState& game, const string& filename);
void loadProject(GameState& game, const string& filename);
void reloadAllTextures(GameState& game, SDL_Renderer* renderer);
Sprite* getActiveSprite(GameState& game);
void setActiveSprite(GameState& game, int index);
void addSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath);
void removeSprite(GameState& game, int index);
void initPaletteBlocks(GameState& game);
Block* getBlockAtPosition(GameState& game, int x, int y);
void snapBlockToParent(Block* child, Block* parent);
void detachBlock(Block* block);

void penEraseAll(GameState& game);
void penStamp(Sprite* sprite, GameState& game, SDL_Renderer* renderer);
void penSetSize(GameState& game, int size);
void penChangeSize(GameState& game, int delta);
void penSetParam(GameState& game, PenColorParam param, double value);
void penChangeParam(GameState& game, PenColorParam param, double delta);
void updatePenColorFromHSV(GameState& game);

void broadcastMessage(GameState& game, const string& messageName, int senderScriptIndex);
void broadcastMessageAndWait(GameState& game, const string& messageName, int senderScriptIndex);
void processMessageQueue(GameState& game);

void resetTimer(GameState& game);
double getTimerValue(GameState& game);
bool isKeyPressed(GameState& game, int keyCode);
bool isMouseDown(GameState& game);
int getMouseX(GameState& game);
int getMouseY(GameState& game);
bool isTouchingEdge(Sprite* sprite, GameState& game);
bool isTouchingMouse(Sprite* sprite, GameState& game);
double getDistanceToMouse(Sprite* sprite, GameState& game);
void askQuestion(GameState& game, const string& question);

void preprocessControlBlocks(GameState& game);
Value evaluateCondition(Block& b, GameState& game);

#endif