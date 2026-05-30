#include <stdlib.h>
#ifdef IS_ANDROID
#include "raymob.h"
#else
#include "raylib.h"
#endif

#include "raymath.h"

#define MIN(a, b) ((a)<(b)? (a) : (b))

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 225
#define MAX_MAGES 1000
#define MAX_DEFENCE_SLOTS 11
#define DEFAULT_MAGE_CD 1
#define DEFAULT_MAGE_ATTACK_CD 1
#define DEFAULT_REGEN 0.1f
#define DEFAULT_ATTACK_CD 1

// Enums

typedef enum GameStateType
{
    GameStateMainMenu, GameStatePlaying, GameStateGameOver, GameStatePause
} GameStateType;

typedef enum LaneType
{
    LaneTop,LaneMid,LaneBottom
} LaneType;

// Structs
typedef struct Mage
{
    Vector2 position;
    Texture2D texture;
    LaneType lane;
    int lineProgress;
    float health;
    int level;
    int hitboxSize;
    float damage;
    float attackCD;
    bool isBoss;
} Mage;

typedef struct DefenceSlot
{
    Vector2 position;
    Texture2D texture;
    Mage *target;
    float damage;
    float attackCD;
    int hitscanSize;
    bool isAlive;
} DefenceSlot;

// PreRender Variables
RenderTexture target = {0};
static Rectangle sourceRec = {0};
static Rectangle destinationRec = {0};

// Level Design
Texture2D level = {0};

// Global Variables
Vector2 virtualMouse = { 0 };
Mage mages[MAX_MAGES] = {0};
DefenceSlot defenceSlots[MAX_DEFENCE_SLOTS] = {0};
int mageAmount = 0;
int totalMageSpawned = 0;
int mainLevel = 1;
int coins = 100;
float mageCD = DEFAULT_MAGE_CD;
float playerHealth = 50.0f;
bool spawnBossFrame = false;
GameStateType gameState = GameStatePlaying;

const Vector2 topLanePoints[] = {{429, 39}, {404, 60}, {364, 58}, {298, 41}, {203, 102}, {113, 104}};
const Vector2 midLanePoints[] = {{451, 101}, {356, 130}, {261, 103}, {113, 104}};
const Vector2 bottomLanePoints[] = {{401, 198}, {311, 196}, {258, 199}, {179, 133}, {113, 104}};

const float initTopLine[] = {30, 40};
const float initMidLine[] = {113, 123};
const float initBottomLine[] = {178, 188};


void LoadDefenceSlots(void)
{
    defenceSlots[0] = (DefenceSlot){(Vector2){102, 93}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 50, true};
    defenceSlots[1] = (DefenceSlot){(Vector2){104, 115}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, true};
    defenceSlots[2] = (DefenceSlot){(Vector2){136, 87}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[3] = (DefenceSlot){(Vector2){136, 129}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[4] = (DefenceSlot){(Vector2){195, 86}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[5] = (DefenceSlot){(Vector2){225, 153}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[6] = (DefenceSlot){(Vector2){253, 89}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[7] = (DefenceSlot){(Vector2){291, 215}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[8] = (DefenceSlot){(Vector2){370, 45}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[9] = (DefenceSlot){(Vector2){388, 139}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[10] = (DefenceSlot){(Vector2){385,211}, {0}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
}

void ToogleDefenceSlot(Vector2 point)
{
    for (int i=2;i<MAX_DEFENCE_SLOTS;i++)
    {
        if (CheckCollisionCircles(point, 10, defenceSlots[i].position, 10))
        {
            if (!defenceSlots[i].isAlive && coins >= 10)
            {
                coins -= 10;
                defenceSlots[i].isAlive = true;
            }
            else
            {
                defenceSlots[i].isAlive = false;
            }
            return;
        }
    }
}

bool Init(void)
{

#if !defined(PLATFORM_WEB)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Smart vs Casual");

    level = LoadTexture("resources/levelnew.png");

    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    sourceRec = (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };

    const float scale = MIN((float)GetScreenWidth()/SCREEN_WIDTH, (float)GetScreenHeight()/SCREEN_HEIGHT);
    destinationRec = (Rectangle){ ((float)GetScreenWidth() - ((float)SCREEN_WIDTH*scale))*0.5f, ((float)GetScreenHeight() - ((float)SCREEN_HEIGHT*scale))*0.5f,
                           (float)SCREEN_WIDTH*scale, (float)SCREEN_HEIGHT*scale };

    // Set defenceSlots
    LoadDefenceSlots();
    return true;
}

bool UpdateDrawFrame(void)
{
    float deltaTime = GetFrameTime();
    if (deltaTime > 1.0f) deltaTime = 0.0f;

    const float scale = MIN((float)GetScreenWidth()/SCREEN_WIDTH, (float)GetScreenHeight()/SCREEN_HEIGHT);

    // Tick
    if (IsWindowResized())
    {
        destinationRec = (Rectangle){ ((float)GetScreenWidth() - ((float)SCREEN_WIDTH*scale))*0.5f, ((float)GetScreenHeight() - ((float)SCREEN_HEIGHT*scale))*0.5f,
                            (float)SCREEN_WIDTH*scale, (float)SCREEN_HEIGHT*scale };
    }

    const Vector2 mouse = GetMousePosition();
    virtualMouse.x = (mouse.x - (GetScreenWidth() - (SCREEN_WIDTH*scale))*0.5f)/scale;
    virtualMouse.y = (mouse.y - (GetScreenHeight() - (SCREEN_HEIGHT*scale))*0.5f)/scale;
    virtualMouse = Vector2Clamp(virtualMouse, (Vector2){ 0, 0 }, (Vector2){SCREEN_WIDTH, SCREEN_HEIGHT });

    switch (gameState) {
        case GameStatePlaying:

        // Input
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            ToogleDefenceSlot(virtualMouse);
        }
        mageCD -= deltaTime;
        
        if (mageCD <= 0.0f)
        {
            mageCD = DEFAULT_MAGE_CD*(10.0f/mainLevel); // TODO: change later to increse difficult by time
            if (!spawnBossFrame)
            {
                LaneType laneToSpawn;
                Vector2 position = {SCREEN_WIDTH, SCREEN_HEIGHT*0.5f};
                switch (GetRandomValue(0, 2)) {
                    case 0:
                        position = (Vector2){SCREEN_WIDTH, GetRandomValue(initTopLine[0], initTopLine[1])};
                        laneToSpawn = LaneTop;
                    break;
                    case 1:
                        position = (Vector2){SCREEN_WIDTH, GetRandomValue(initBottomLine[0], initBottomLine[1])};
                        laneToSpawn = LaneBottom;
                    break;
                    case 2:
                    default:
                        position = (Vector2){SCREEN_WIDTH, GetRandomValue(initMidLine[0], initMidLine[1])};
                        laneToSpawn = LaneMid;
                    break;
                }
                mages[mageAmount++] = (Mage){
                    position, 
                    {0},
                    laneToSpawn,
                    0,
                    10*mainLevel, 
                    mainLevel, 
                    5,
                    10,
                    DEFAULT_MAGE_ATTACK_CD,
                    false
                };
            }
            else
            {
                spawnBossFrame = false;
                mages[mageAmount++] = (Mage){
                    (Vector2){SCREEN_WIDTH, initMidLine[0]+(initMidLine[1]-initMidLine[0])*0.5f}, 
                    {0},
                    LaneMid,
                    0,
                    40*mainLevel, 
                    mainLevel+5, 
                    10,
                    10,
                    DEFAULT_MAGE_ATTACK_CD,
                    true
                };
            }
            totalMageSpawned++;
            if (totalMageSpawned >= 10*mainLevel)
            {
                totalMageSpawned = 0;
                mainLevel++;
                spawnBossFrame = true;
            }
        }

        for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
        {
            if (defenceSlots[i].isAlive && defenceSlots[i].target == NULL)
            {
                for (int j=0;j<mageAmount;j++)
                {
                    if (CheckCollisionCircles(defenceSlots[i].position, (float)defenceSlots[i].hitscanSize, mages[j].position, (float)mages[j].hitboxSize))
                    {
                        defenceSlots[i].target = &mages[j];
                        break;
                    }
                }
            }
        }

        // Attack Targets
        for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
        {
            if (defenceSlots[i].isAlive)
            {
                if (defenceSlots[i].attackCD <= 0.0f)
                {
                    if (defenceSlots[i].target != NULL)
                    {
                        defenceSlots[i].target->health -= defenceSlots[i].damage;
                        if (defenceSlots[i].target->health <= 0.0f)
                        {
                            defenceSlots[i].target = NULL;
                        }
                    }
                    defenceSlots[i].attackCD = DEFAULT_ATTACK_CD;
                }
                else
                {
                    defenceSlots[i].attackCD -= deltaTime;
                }
            }
        }
        for (int i=mageAmount-1;i>=0;i--)
        {
            if (mages[i].health <= 0.0f)
            {
                coins += 2*mages[i].level;
                mages[i] = mages[mageAmount-1];
                mageAmount--;
                continue;
            }
            if (mages[i].position.x <= 120)
            {
                if (mages[i].attackCD <= 0.0f)
                {
                    mages[i].attackCD = DEFAULT_MAGE_ATTACK_CD;
                    playerHealth -= mages[i].damage;
                }
                else
                {
                    mages[i].attackCD -= deltaTime;
                }
            }
            else
            {
                Vector2 direction = {0};
                switch (mages[i].lane) {
                    case LaneTop:
                        if (topLanePoints[mages[i].lineProgress].x+5 > mages[i].position.x) mages[i].lineProgress++;
                        direction = Vector2Normalize(Vector2Subtract(topLanePoints[mages[i].lineProgress], mages[i].position));
                    break;
                    case LaneMid:
                        if (midLanePoints[mages[i].lineProgress].x+5 > mages[i].position.x) mages[i].lineProgress++;
                        direction = Vector2Normalize(Vector2Subtract(midLanePoints[mages[i].lineProgress], mages[i].position));
                    break;
                    case LaneBottom:
                        if (bottomLanePoints[mages[i].lineProgress].x+5 > mages[i].position.x) mages[i].lineProgress++;
                        direction = Vector2Normalize(Vector2Subtract(bottomLanePoints[mages[i].lineProgress], mages[i].position));
                    break;
                }
                mages[i].position = Vector2Add(mages[i].position, Vector2Scale(direction, 10*deltaTime));
            }
        }

        // Check if player is dead
        if (playerHealth <= 0.0f)
        {
            gameState = GameStateGameOver;
        }

        break;
        default:
        break;
    }

    // Draw
    BeginTextureMode(target);
        ClearBackground(WHITE);
        switch (gameState) {
            case GameStatePlaying:
                DrawTexture(level, 0, 0, WHITE);

                for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
                {
                    if (!defenceSlots[i].isAlive)
                    {
                        DrawCircleV(defenceSlots[i].position, 4, GRAY);
                    }
                }
                for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
                {
                    if (defenceSlots[i].isAlive)
                    {
                        DrawCircleV(defenceSlots[i].position, (float)defenceSlots[i].hitscanSize, Fade(GREEN, 0.4f));
                    }
                }

                for (int i=0;i<mageAmount;i++)
                {
                    DrawCircleV(mages[i].position, mages[i].hitboxSize, BLUE);
                }
                DrawText(TextFormat("Coins: %i", coins), 10, 10, 10, GREEN);
                DrawText(TextFormat("Level: %i", mainLevel), 10, 20, 10, GREEN);
                DrawText(TextFormat("Health: %3.1f", playerHealth), 10, 30, 10, GREEN);
            break;
            case GameStateGameOver:
                DrawText("Game Over", 196, SCREEN_HEIGHT*0.5f-10, 20, RED);
            break;
            default:
            break;
        }
    EndTextureMode();

    BeginDrawing();
        ClearBackground(WHITE);

        DrawTexturePro(target.texture, sourceRec, destinationRec, (Vector2){0}, 0.0f, WHITE);
    EndDrawing();
    return true;
}

void Destroy(void)
{
    UnloadTexture(target.texture);
    UnloadTexture(level);
}
