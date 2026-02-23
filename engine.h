#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include "blocks.h"
#include "value.h"

using namespace std;

// ساختار برای ذخیره یک نقطه از خط رسم شده
struct PenPoint
{
    int x, y;
    Uint8 r, g, b, a;
    int size;
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
};

struct GameState
{
    // اسپرایت‌ها
    vector<Sprite> sprites;
    int activeSpriteIndex;

    // بلوک‌ها
    vector<Block> program;
    vector<Block> paletteBlocks;
    int currentBlockIndex;

    // وضعیت اجرا
    bool isRunningCode;
    bool stepMode;
    double remainingMove;
    bool isExecutingBlock;

    // ابعاد صفحه
    int screenWidth;
    int screenHeight;

    // دکمه‌های اصلی
    Button runButton;
    Button pauseButton;
    Button stepButton;
    Button resetButton;
    Button saveButton;
    Button loadButton;

    // دکمه‌های مدیریت اسپرایت
    Button addSpriteBtn;
    Button deleteSpriteBtn;
    Button prevSpriteBtn;
    Button nextSpriteBtn;

    // دکمه‌های پس‌زمینه
    Button uploadBackdropBtn;
    Button prevBackdropBtn;
    Button nextBackdropBtn;

    // دکمه‌های دسته‌بندی
    Button moveCategoryBtn;
    Button looksCategoryBtn;
    Button soundCategoryBtn;
    Button eventsCategoryBtn;
    Button controlCategoryBtn;
    Button sensingCategoryBtn;
    Button operatorsCategoryBtn;
    Button variablesCategoryBtn;
    Button penCategoryBtn;  // دکمه جدید برای Pen

    int currentCategory;

    // پشته‌ها برای حلقه‌ها
    vector<int> repeatCountStack;
    vector<int> repeatStartStack;

    // زمان‌بندی
    Uint32 waitStartTime;
    Uint32 waitDuration;
    bool isWaiting;

    Uint32 messageStartTime;
    Uint32 messageDuration;
    bool isShowingMessage;

    // متغیرها
    unordered_map<string, Value> variables;

    // رویدادها
    bool greenFlagPressed;
    Uint8 pressedKeys[SDL_NUM_SCANCODES];
    Uint8 pressedThisFrame[SDL_NUM_SCANCODES];
    bool spriteClicked;
    int mouseX, mouseY;
    bool mousePressed;

    // اسکریپت‌ها
    vector<int> scriptStartIndices;
    vector<bool> scriptActive;
    vector<int> scriptCurrentBlock;

    // حسگرها
    string askQuestion;
    string answer;
    bool waitingForAnswer;
    Uint32 timerStartTime;
    bool dragMode;

    // بلوک انتخاب شده
    Block selectedBlock;
    bool placingBlock;

    // ویرایش
    bool editingMode;
    int editingField;
    string editingBuffer;
    bool showSpriteName;

    // صدا
    int volume;
    bool isPlayingSound;
    Mix_Chunk* soundEffect;
    int soundChannel;

    // درگ اسپرایت
    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;
    int clickedSpriteIndex;

    // لوگو
    SDL_Texture* logoTexture;

    // پس‌زمینه‌ها
    vector<Backdrop> backdrops;
    int currentBackdrop;

    // ===== فیلدهای جدید برای Drag & Drop بلوک =====
    Block* draggedBlock;
    bool isDraggingBlock;
    int dragBlockStartX, dragBlockStartY;
    int dragBlockOffsetX, dragBlockOffsetY;
    Block* hoverBlock;
    int codeAreaX, codeAreaY;
    int codeAreaWidth, codeAreaHeight;

    // ===== فیلدهای جدید برای تشخیص کلیک روی پنل‌ها =====
    int categoriesPanelX, categoriesPanelY;
    int categoriesPanelWidth, categoriesPanelHeight;
    int examplesPanelX, examplesPanelY;
    int examplesPanelWidth, examplesPanelHeight;

    // ===== فیلدهای جدید برای بازگردانی بلوک در صورت رها شدن خارج از Code Area =====
    Block* draggedBlockOriginalParent;
    int draggedBlockOriginalIndex;

    // ===== فیلدهای جدید برای منطقه حذف (Trash) =====
    int trashX, trashY, trashW, trashH;

    // ===== فیلدهای مربوط به Pen =====
    bool penDown;                 // آیا قلم پایین است؟
    int penSize;                  // ضخامت قلم (پیکسل)
    Uint8 penR, penG, penB, penA; // رنگ قلم (RGB)
    double penHue;                // مقدار hue برای تغییرات (0-360)
    double penSaturation;         // اشباع (0-100)
    double penBrightness;         // روشنایی (0-100)
    vector<PenPoint> penPoints;   // نقاط رسم شده
    int stageX, stageY, stageW, stageH;

    // سازنده پیش‌فرض
    GameState() :
            activeSpriteIndex(-1),
            currentBlockIndex(0),
            isRunningCode(false),
            stepMode(false),
            remainingMove(0),
            isExecutingBlock(false),
            screenWidth(0),
            screenHeight(0),
            currentCategory(0),
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
            // مقادیر پیش‌فرض Pen
            penDown(false),
            penSize(1),
            penR(255),
            penG(0),
            penB(0),
            penA(255),
            penHue(0),
            penSaturation(100),
            penBrightness(100)
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
        penCategoryBtn = {0,0,0,0,0};  // مقداردهی
    }
};

// توابع اصلی
void update(GameState& game, SDL_Renderer* renderer);
void saveProject(const GameState& game, const string& filename);
void loadProject(GameState& game, const string& filename);
void reloadAllTextures(GameState& game, SDL_Renderer* renderer);

// توابع مدیریت اسپرایت
Sprite* getActiveSprite(GameState& game);
void setActiveSprite(GameState& game, int index);
void addSprite(GameState& game, SDL_Renderer* renderer, const char* name, const char* imagePath);
void removeSprite(GameState& game, int index);

// توابع جدید برای Drag & Drop بلوک
void initPaletteBlocks(GameState& game);
Block* getBlockAtPosition(GameState& game, int x, int y);
void snapBlockToParent(Block* child, Block* parent);
void detachBlock(Block* block);

// توابع Pen
void penEraseAll(GameState& game);
void penStamp(Sprite* sprite, GameState& game, SDL_Renderer* renderer);
void penSetColor(GameState& game, Uint8 r, Uint8 g, Uint8 b);
void penSetSize(GameState& game, int size);
void penChangeSize(GameState& game, int delta);
void penSetParam(GameState& game, PenColorParam param, double value);
void penChangeParam(GameState& game, PenColorParam param, double delta);
void updatePenColorFromHSV(GameState& game); // تبدیل HSV به RGB

#endif