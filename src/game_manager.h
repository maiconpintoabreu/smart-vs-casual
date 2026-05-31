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
#define MAX_CASUAL_LEVEL 1.0f
#define MAX_PARTICLES 1000
#define DEFAULT_MAGE_CD 1
#define DEFAULT_MAGE_ATTACK_CD 1
#define DEFAULT_MAGE_HEALTH 4
#define DEFAULT_MAGE_ANIMATION_CD 0.4f
#define DEFAULT_MAGE_SPEED 10
#define DEFAULT_REGEN 0.1f
#define DEFAULT_ATTACK_CD 1
#define DEFAULT_ATTACK_DURATION 0.4f
#define DEFAULT_HEALTH 50.0f 

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
    LaneType lane;
    int lineProgress;
    int level;
    int hitboxSize;
    int animationIndex;
    float health;
    float damage;
    float attackCD;
    float animationCD;
    bool isBoss;
} Mage;

typedef struct DefenceSlot
{
    Vector2 position;
    Mage *target;
    float damage;
    float attackCD;
    int hitscanSize;
    bool isAlive;
} DefenceSlot;

typedef struct Bullet
{
    Vector2 from;
    Vector2 hit;
    float duration;
} Bullet;

typedef struct Particles {
    Vector2 position;
    Vector2 acceleration;
    Color color;
    float size;
    float maxLifeTime;
    float lifeTime;
    bool isAlive;
} Particles;


// PreRender Variables
RenderTexture target = {0};
static Rectangle sourceRec = {0};
static Rectangle destinationRec = {0};

// UI
const Rectangle startGameRec = {150, SCREEN_HEIGHT*0.5f-(25.0f+40.0f*0.5f), 200, 40};
const Rectangle quitGameRec = {150, SCREEN_HEIGHT*0.5f+(25.0f-40.0f*0.5f), 200, 40};
const Rectangle backRec = {SCREEN_WIDTH - 50.0f, 10.0f, 20.0f, 20.0f};

// Level Design
Texture2D level = {0};

// Global Variables
Vector2 virtualMouse = {0};
Mage mages[MAX_MAGES] = {0};
DefenceSlot defenceSlots[MAX_DEFENCE_SLOTS] = {0};
Bullet bullets[MAX_DEFENCE_SLOTS*2] = {0};
Particles particles[MAX_PARTICLES] = {0};
Texture2D assetTexture = {0};
Music backgroundMusic = {0};
Sound hitSFX = {0};
int mageAmount = 0;
int bulletAmount = 0;
float counterChangeLevel = 0.0f;
int mainLevel = 1;
int coins = 100;
int extraDefenceActivatedAmount = 0;
float mageCD = DEFAULT_MAGE_CD;
float casualLevel = MAX_CASUAL_LEVEL;
float playerHealth = DEFAULT_HEALTH;
bool spawnBossFrame = false;
GameStateType gameState = GameStateMainMenu;

const Vector2 topLanePoints[] = {{429, 39}, {404, 60}, {364, 58}, {298, 41}, {203, 102}, {113, 104}};
const Vector2 midLanePoints[] = {{451, 101}, {356, 130}, {261, 103}, {113, 104}};
const Vector2 bottomLanePoints[] = {{401, 198}, {311, 196}, {258, 199}, {179, 133}, {113, 104}};

const float initTopLine[] = {30, 40};
const float initMidLine[] = {113, 123};
const float initBottomLine[] = {178, 188};

void RestartGame(void)
{
    mageAmount = 0;
    bulletAmount = 0;
    counterChangeLevel = 0.0f;
    extraDefenceActivatedAmount = 0;
    mainLevel = 1;
    coins = 100;
    casualLevel = 1.0f;
    mageCD = DEFAULT_MAGE_CD;
    playerHealth = DEFAULT_HEALTH;
    for (int i=2;i<MAX_DEFENCE_SLOTS;i++)
    {
        defenceSlots[i].isAlive = false;
    }

    for (int i=1; i<MAX_PARTICLES; i++)
    {
        particles[i].isAlive = false;
    }

}

void LoadDefenceSlots(void)
{
    defenceSlots[0] = (DefenceSlot){(Vector2){102, 93}, NULL, 1, DEFAULT_ATTACK_CD, 50, true};
    defenceSlots[1] = (DefenceSlot){(Vector2){104, 115}, NULL, 1, DEFAULT_ATTACK_CD, 56, true};
    defenceSlots[2] = (DefenceSlot){(Vector2){136, 87}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[3] = (DefenceSlot){(Vector2){136, 129}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[4] = (DefenceSlot){(Vector2){195, 86}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[5] = (DefenceSlot){(Vector2){225, 153}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[6] = (DefenceSlot){(Vector2){253, 89}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[7] = (DefenceSlot){(Vector2){291, 215}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[8] = (DefenceSlot){(Vector2){370, 45}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[9] = (DefenceSlot){(Vector2){388, 139}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
    defenceSlots[10] = (DefenceSlot){(Vector2){385,211}, NULL, 1, DEFAULT_ATTACK_CD, 56, false};
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
                extraDefenceActivatedAmount++;
            }
            else
            {
                defenceSlots[i].isAlive = false;
                extraDefenceActivatedAmount--;
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
    assetTexture = LoadTexture("resources/assets.png");

    target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    sourceRec = (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height };

    const float scale = MIN((float)GetScreenWidth()/SCREEN_WIDTH, (float)GetScreenHeight()/SCREEN_HEIGHT);
    destinationRec = (Rectangle){ ((float)GetScreenWidth() - ((float)SCREEN_WIDTH*scale))*0.5f, ((float)GetScreenHeight() - ((float)SCREEN_HEIGHT*scale))*0.5f,
                           (float)SCREEN_WIDTH*scale, (float)SCREEN_HEIGHT*scale };

    // Set defenceSlots
    LoadDefenceSlots();
    InitAudioDevice();
    backgroundMusic = LoadMusicStream("resources/backgroundMusic.wav");
    hitSFX = LoadSound("resources/hit.wav");
    return true;
}

void DrawFrame(float deltaTime)
{
    // Draw
    BeginTextureMode(target);
        ClearBackground(GRAY);
        switch (gameState) {
            case GameStateMainMenu:
                DrawRectangleRounded(startGameRec, 2, 2, DARKGRAY);
                DrawRectangleRounded(quitGameRec, 2, 2, DARKGRAY);
                DrawText("Start Game", (startGameRec.x+startGameRec.width*0.5f)-MeasureTextEx(GetFontDefault(), "Start Game", 20, 2).x*0.5f, startGameRec.y+startGameRec.height*0.5f-10.0f, 20, LIGHTGRAY);
                DrawText("Quit", (quitGameRec.x+quitGameRec.width*0.5f)-MeasureTextEx(GetFontDefault(), "Quit", 20, 2).x*0.5f, quitGameRec.y+quitGameRec.height*0.5f-10.0f, 20, LIGHTGRAY);
                DrawRectangleRoundedLinesEx(startGameRec, 2, 4,2, BLACK);
                DrawRectangleRoundedLinesEx(quitGameRec, 2, 4,2, BLACK);
                DrawText("Smart vs Casual", (SCREEN_WIDTH*0.5f)-MeasureTextEx(GetFontDefault(), "Smart vs Casual", 20, 2).x*0.5f, 20, 20, DARKGRAY);
                DrawText("If you try too hard the game will hit harder", (SCREEN_WIDTH*0.5f)-MeasureTextEx(GetFontDefault(), "If you try too hard the game will hit harder", 10, 1).x*0.5f, 40, 10, Fade(DARKGRAY, 0.8f));
            break;
            case GameStatePlaying:
                DrawTexture(level, 0, 0, WHITE);

                for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
                {
                    if (!defenceSlots[i].isAlive)
                    {
                        casualLevel = Clamp(casualLevel+0.01f*deltaTime, 0.0f, MAX_CASUAL_LEVEL);
                    }
                    DrawTexturePro(assetTexture, 
                        (Rectangle){0,0,16,16},
                        (Rectangle){defenceSlots[i].position.x, defenceSlots[i].position.y, 16.0f, 16.0f}, 
                        (Vector2){8,15}, 0, WHITE);
                }

                for (int i=0;i<mageAmount;i++)
                {
                    const float mageSize = mages[i].isBoss ? 32.0f : 16.0f;
                    DrawTexturePro(assetTexture, 
                        (Rectangle){mages[i].animationIndex*16.0f,16.0f,16.0f,16.0f}, 
                        (Rectangle){mages[i].position.x, mages[i].position.y, mageSize, mageSize}, 
                        (Vector2){8,mageSize-1}, 0, mages[i].isBoss ? RED : WHITE);
                }

                for (int i=0;i<bulletAmount;i++)
                {
                    DrawLineV(Vector2Subtract(bullets[i].from, (Vector2){0,8}), Vector2Subtract(bullets[i].hit, (Vector2){0,8}), Fade(WHITE, bullets[i].duration));
                }
                for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
                {
                    if (defenceSlots[i].isAlive)
                    {
                        DrawTexturePro(assetTexture, 
                            (Rectangle){0,32,16,16},
                            (Rectangle){defenceSlots[i].position.x, defenceSlots[i].position.y-5, 16.0f, 16.0f}, 
                            (Vector2){8,15}, 0, WHITE);
                        DrawCircleV(defenceSlots[i].position, (float)defenceSlots[i].hitscanSize, Fade(GREEN, 0.1f));
                    }
                }

                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (particles[i].isAlive)
                    {
                        Color particleColor = Fade(particles[i].color, particles[i].lifeTime / particles[i].maxLifeTime);
                        DrawCircleV(particles[i].position, particles[i].size, particleColor);
                    }
                }

                DrawRectangle(10, 40, 99*casualLevel, 20, GRAY);
                if (casualLevel < 0.5f) DrawText("HARD", 60-(MeasureTextEx(GetFontDefault(), "HARD", 10, 1).x*0.5f), 45, 10, BLACK);
                if (casualLevel >= 0.5f) DrawText("CASUAL", 60-(MeasureTextEx(GetFontDefault(), "CASUAL", 10, 1).x*0.5f), 45, 10, BLACK);
                DrawRectangleLines(10, 40, 100, 20, BLACK);

                DrawRectangle(10, 20, 99*(playerHealth/(DEFAULT_HEALTH*mainLevel)), 15, GRAY);
                DrawText("HEALTH", 60-(MeasureTextEx(GetFontDefault(), "HEALTH", 10, 1).x*0.5f), 23, 10, BLACK);
                DrawRectangleLines(10, 20, 100, 15, BLACK);
                DrawText(TextFormat("Coins: %i", coins), 10, 10, 10, BLACK);
                const char *levelText = TextFormat("Level -> %i", mainLevel);
                DrawText(levelText, SCREEN_WIDTH*0.5f-(MeasureTextEx(GetFontDefault(), levelText, 10, 1).x*0.5f), 10, 10, BLACK);

                DrawRectangleRoundedLinesEx(backRec, 2, 2, 2, BLACK);
                DrawText("<", backRec.x+6, backRec.y+1, 20, BLACK);
            break;
            case GameStateGameOver:
                DrawText("Restart Game", (startGameRec.x+startGameRec.width*0.5f)-MeasureTextEx(GetFontDefault(), "Restart Game", 20, 2).x*0.5f, startGameRec.y+startGameRec.height*0.5f-10.0f, 20, LIGHTGRAY);
                DrawText("Game Over", 196, SCREEN_HEIGHT*0.5f-10.0f, 20, RED);
            break;
            default:
            break;
        }
    EndTextureMode();

    BeginDrawing();
        ClearBackground(GRAY);

        DrawTexturePro(target.texture, sourceRec, destinationRec, (Vector2){0}, 0.0f, WHITE);
    EndDrawing();
}


bool UpdateDrawFrame(void)
{
    float deltaTime = GetFrameTime();
    if (deltaTime > 1.0f) deltaTime = 0.0f;

    playerHealth = Clamp(playerHealth+DEFAULT_REGEN*deltaTime, 0.0f, DEFAULT_HEALTH*mainLevel);

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
        case GameStateMainMenu:
            if (CheckCollisionPointRec(virtualMouse, startGameRec))
            {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                {
                    RestartGame();
                    gameState = GameStatePlaying;
                }
            }
            if (CheckCollisionPointRec(virtualMouse, quitGameRec))
            {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                {
                    return false;
                }
            }
        break;
        case GameStatePlaying:

        // Input
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            ToogleDefenceSlot(virtualMouse);
        }

        if (CheckCollisionPointRec(virtualMouse, backRec))
        {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                gameState = GameStateMainMenu;
                return true;
            }
        }
        const float mageCDbyCausalLevel = casualLevel > 0.1f ? 1.0f/casualLevel : 11.0f;
        mageCD -= mageCDbyCausalLevel*deltaTime;
        float deviceUsagePercentage = (1-(extraDefenceActivatedAmount/(MAX_DEFENCE_SLOTS-2.0f)));
        casualLevel = Lerp(casualLevel, deviceUsagePercentage, deltaTime);

        if (IsAudioDeviceReady())
        {
            if (IsMusicValid(backgroundMusic) && !IsMusicStreamPlaying(backgroundMusic)) PlayMusicStream(backgroundMusic);
            if (IsMusicValid(backgroundMusic)) UpdateMusicStream(backgroundMusic);
        }
        
        if (mageCD <= 0.0f && mageAmount < MAX_MAGES)
        {
            mageCD = DEFAULT_MAGE_CD*(10.0f/mainLevel);
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
                    laneToSpawn,
                    0,
                    mainLevel, 
                    5,
                    0,
                    DEFAULT_MAGE_HEALTH,
                    DEFAULT_MAGE_HEALTH*mainLevel, 
                    DEFAULT_MAGE_ATTACK_CD,
                    DEFAULT_MAGE_ANIMATION_CD,
                    false
                };
            }
            else
            {
                spawnBossFrame = false;
                mages[mageAmount++] = (Mage){
                    (Vector2){SCREEN_WIDTH, initMidLine[0]+(initMidLine[1]-initMidLine[0])*0.5f}, 
                    LaneMid,
                    0,
                    mainLevel+5, 
                    10,
                    0,
                    DEFAULT_MAGE_HEALTH*3*mainLevel, 
                    DEFAULT_MAGE_HEALTH*3,
                    DEFAULT_MAGE_ATTACK_CD,
                    DEFAULT_MAGE_ANIMATION_CD,
                    true
                };
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
        for (int i=bulletAmount-1;i>=0;i--)
        {
            bullets[i].duration -= deltaTime;
            if (bullets[i].duration)
            {
                bulletAmount--;
                bullets[i] = bullets[bulletAmount];
            }
        }
        for (int i=0;i<MAX_DEFENCE_SLOTS;i++)
        {
            if (defenceSlots[i].isAlive)
            {
                if (defenceSlots[i].attackCD <= 0.0f)
                {
                    if (defenceSlots[i].target != NULL)
                    {
                        defenceSlots[i].target->health -= defenceSlots[i].damage;
                        bullets[bulletAmount] = (Bullet){defenceSlots[i].position, defenceSlots[i].target->position, DEFAULT_ATTACK_DURATION};
                        bulletAmount++;
                        PlaySound(hitSFX);
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

                    if (defenceSlots[i].target != NULL)
                    {
                        if (defenceSlots[i].target->health <= 0.0f)
                        {
                            defenceSlots[i].target = NULL;
                        } else if (!CheckCollisionCircles(defenceSlots[i].position, (float)defenceSlots[i].hitscanSize, defenceSlots[i].target->position, (float)defenceSlots[i].target->hitboxSize))
                        {
                            defenceSlots[i].target = NULL;
                        }
                    }
                }
            }
        }
        for (int i=mageAmount-1;i>=0;i--)
        {
            if (mages[i].health <= 0.0f)
            {
                coins += 2*mages[i].level;

                Vector2 particlesPosition = mages[i].position;

                // Spawn Particles
                int spreadX = 25;
                int startAccelerationY = -100;
                int howManyParticles = 20;

                for (int n = 0;n < MAX_PARTICLES; n++)
                {
                    if (!particles[n].isAlive)
                    {
                        particles[n].isAlive = true;
                        particles[n].maxLifeTime = 0.5f;
                        particles[n].position = particlesPosition;
                        particles[n].size = 3;
                        particles[n].lifeTime = particles[n].maxLifeTime;
                        particles[n].acceleration.x = (float)GetRandomValue(-spreadX, spreadX) ;
                        particles[n].acceleration.y = (float)GetRandomValue(startAccelerationY, 100);
                        particles[n].color = BROWN;
                        howManyParticles -= 1;
                        if (howManyParticles <= 0)
                        {
                            break;
                        }
                    }
                }
                mageAmount--;
                mages[i] = mages[mageAmount];

                continue;
            }
            if (mages[i].animationCD <= 0.0f)
            {
                mages[i].animationCD = DEFAULT_MAGE_ANIMATION_CD;
                mages[i].animationIndex++;
                if (mages[i].animationIndex == 2) mages[i].animationIndex = 0;
            }
            else
            {
                mages[i].animationCD -= deltaTime;
            }
            if (mages[i].position.x <= 120)
            {
                if (mages[i].attackCD <= 0.0f)
                {
                    mages[i].attackCD = DEFAULT_MAGE_ATTACK_CD;
                    playerHealth -= mages[i].damage;
                    PlaySound(hitSFX);
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
                float speed = casualLevel > 0.1f ? (DEFAULT_MAGE_SPEED*mainLevel)/casualLevel : (DEFAULT_MAGE_SPEED*mainLevel)/0.1f;
                mages[i].position = Vector2Add(mages[i].position, Vector2Scale(direction, speed*deltaTime));
            }
        }

        // Check if player is dead
        if (playerHealth <= 0.0f)
        {
            gameState = GameStateGameOver;
            return true;
        }

        // Update Particles
        for (int i = 0; i < MAX_PARTICLES; i++)
        {
            if (particles[i].isAlive)
            {
                particles[i].lifeTime -= deltaTime;
                if (particles[i].lifeTime <=0.0f)
                {
                    particles[i].isAlive = false;
                    continue;
                }

                particles[i].position = Vector2Add(particles[i].position, Vector2Scale(particles[i].acceleration, deltaTime));
            }
        }
        
        counterChangeLevel += deltaTime;
        TraceLog(LOG_INFO, "Test: %3.3f",counterChangeLevel);
        if (counterChangeLevel >= 30*mainLevel)
        {
            counterChangeLevel = 0.0f;
            float percentage = playerHealth/(DEFAULT_HEALTH*mainLevel);
            mainLevel++;
            playerHealth = (DEFAULT_HEALTH*mainLevel)*percentage;
            spawnBossFrame = true;
        }

        break;
        case GameStateGameOver:
            if (CheckCollisionPointRec(virtualMouse, startGameRec))
            {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                {
                    RestartGame();
                    gameState = GameStatePlaying;
                }
            }
        break;
        default:
        break;
    }
    DrawFrame(deltaTime);
    return true;
}

void Destroy(void)
{
    UnloadTexture(target.texture);
    UnloadTexture(level);
    UnloadTexture(assetTexture);
    UnloadMusicStream(backgroundMusic);
    UnloadSound(hitSFX);
    CloseAudioDevice();
}
