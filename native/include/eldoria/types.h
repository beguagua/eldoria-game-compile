#ifndef ELDORIA_TYPES_H
#define ELDORIA_TYPES_H

/* Eldoria design reminder: natural fantasy with strong silhouettes, luminous minerals and low-cost 3D rendering. */

#include <stdbool.h>
#include <stdint.h>
#include "raylib.h"

#define ELDORIA_MAX_ENTITIES 96
#define ELDORIA_MAX_RESOURCES 96
#define ELDORIA_MAX_BUILDINGS 128
#define ELDORIA_MAX_INVENTORY 32
#define ELDORIA_MAX_NPCS 12
#define ELDORIA_MAX_PARTICLES 72

#define ELDORIA_PLAYER_RADIUS 0.45f
#define ELDORIA_INTERACT_RANGE 3.1f
#define ELDORIA_ATTACK_RANGE 2.35f

#define ELDORIA_SAVE_MAGIC 0x454C444F
#define ELDORIA_SAVE_VERSION 6

typedef enum EldoriaGameState {
    ELDORIA_STATE_SPLASH = 0,
    ELDORIA_STATE_MENU,
    ELDORIA_STATE_PLAYING,
    ELDORIA_STATE_INVENTORY,
    ELDORIA_STATE_CRAFTING,
    ELDORIA_STATE_ENDING,
    ELDORIA_STATE_CREDITS,
    ELDORIA_STATE_PAUSED,
    ELDORIA_STATE_OPTIONS,
    ELDORIA_STATE_MULTIPLAYER
} EldoriaGameState;

typedef enum EldoriaRealm {
    ELDORIA_REALM_VALLEY = 0,
    ELDORIA_REALM_CAVES,
    ELDORIA_REALM_COBALT,
    ELDORIA_REALM_FURNACE,
    ELDORIA_REALM_COUNT
} EldoriaRealm;

typedef enum EldoriaResourceKind {
    ELDORIA_RESOURCE_WOOD = 0,
    ELDORIA_RESOURCE_STONE,
    ELDORIA_RESOURCE_CRYSTAL,
    ELDORIA_RESOURCE_BERRY,
    ELDORIA_RESOURCE_IRON,
    ELDORIA_RESOURCE_COBALT,
    ELDORIA_RESOURCE_RUBYCORE,
    ELDORIA_RESOURCE_POTION,
    ELDORIA_RESOURCE_PORTAL_KEY,
    ELDORIA_RESOURCE_KIND_COUNT
} EldoriaResourceKind;

typedef enum EldoriaEntityKind {
    ELDORIA_ENTITY_RISCANTE = 0,
    ELDORIA_ENTITY_ESPORO,
    ELDORIA_ENTITY_ESCAVADOR,
    ELDORIA_ENTITY_SENTINELA,
    ELDORIA_ENTITY_BRASA,
    ELDORIA_ENTITY_BOSS_ESCAVADOR,
    ELDORIA_ENTITY_BOSS_COBALTO,
    ELDORIA_ENTITY_BOSS_FORNALHA
} EldoriaEntityKind;

typedef enum EldoriaBuildingKind {
    ELDORIA_BUILDING_FOUNDATION = 0,
    ELDORIA_BUILDING_WALL,
    ELDORIA_BUILDING_PILLAR,
    ELDORIA_BUILDING_LANTERN,
    ELDORIA_BUILDING_PORTAL,
    ELDORIA_BUILDING_WORKBENCH
} EldoriaBuildingKind;

typedef enum EldoriaQuestStage {
    ELDORIA_QUEST_LANTERNS = 0,
    ELDORIA_QUEST_CAVES,
    ELDORIA_QUEST_COBALT,
    ELDORIA_QUEST_FURNACE,
    ELDORIA_QUEST_COMPLETE
} EldoriaQuestStage;

typedef struct EldoriaItemStack {
    EldoriaResourceKind kind;
    int amount;
} EldoriaItemStack;

typedef struct EldoriaPlayer {
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float pitch;
    float health;
    float maxHealth;
    float stamina;
    float maxStamina;
    int level;
    int experience;
    int lanternsRestored;
    int weaponTier;
    int armorTier; /* 0 sem armadura; 1 aurora; 2 ferro; 3 cobalto; 4 rubra. */
    EldoriaItemStack inventory[ELDORIA_MAX_INVENTORY];
    int inventoryCount;
} EldoriaPlayer;

typedef struct EldoriaResourceNode {
    bool active;
    EldoriaResourceKind kind;
    Vector3 position;
    float radius;
    int amount;
} EldoriaResourceNode;

typedef struct EldoriaNPC {
    bool active;
    Vector3 position;
    Vector3 home;
    float routineClock;
    int reputation;
    int dialogueStage;
    char name[48];
    char role[48];
} EldoriaNPC;

typedef struct EldoriaEnemy {
    bool active;
    EldoriaEntityKind kind;
    Vector3 position;
    Vector3 velocity;
    float health;
    float maxHealth;
    float attackCooldown;
    float wanderTime;
    float phaseTimer;
    bool boss;
} EldoriaEnemy;

typedef struct EldoriaBuilding {
    bool active;
    EldoriaBuildingKind kind;
    EldoriaRealm targetRealm;
    Vector3 position;
    Vector3 size;
    Color color;
} EldoriaBuilding;

typedef struct EldoriaQuest {
    EldoriaQuestStage stage;
    bool complete;
    int progress;
    int target;
    char title[96];
    char description[192];
} EldoriaQuest;

typedef struct EldoriaParticle {
    Vector3 position;
    float phase;
    float scale;
} EldoriaParticle;

typedef enum EldoriaPetKind {
    ELDORIA_PET_AURORA_FOX = 0,
    ELDORIA_PET_MOSS_OWL,
    ELDORIA_PET_EMBER_SALAMANDER,
    ELDORIA_PET_COBALT_HARE,
    ELDORIA_PET_KIND_COUNT
} EldoriaPetKind;

typedef struct EldoriaPet {
    bool active;
    EldoriaPetKind kind;
    Vector3 position;
    float mood;
    float followDistance;
} EldoriaPet;

typedef struct EldoriaSettings {
    bool shaderEnabled;
    bool highQuality;
    bool shaderImportPending;
    char shaderPath[256];
} EldoriaSettings;

typedef struct EldoriaWorld {
    uint64_t seed;
    EldoriaRealm realm;
    EldoriaResourceNode resources[ELDORIA_MAX_RESOURCES];
    int resourceCount;
    EldoriaEnemy enemies[ELDORIA_MAX_ENTITIES];
    int enemyCount;
    EldoriaNPC npcs[ELDORIA_MAX_NPCS];
    int npcCount;
    EldoriaBuilding buildings[ELDORIA_MAX_BUILDINGS];
    int buildingCount;
    EldoriaParticle particles[ELDORIA_MAX_PARTICLES];
    float timeOfDay;
    int restoredLanterns;
    bool bossDefeated[ELDORIA_REALM_COUNT];
} EldoriaWorld;

typedef struct EldoriaInput {
    Vector2 move;
    Vector2 look;
    bool sprint;
    bool attackPressed;
    bool interactPressed;
    bool buildPressed;
    bool inventoryPressed;
    bool craftingPressed;
    bool petPressed;
    bool craftOnePressed;
    bool craftTwoPressed;
    bool craftThreePressed;
    bool craftFourPressed;
    bool craftFivePressed;
    bool craftSixPressed;
    bool craftSevenPressed;
    bool craftEightPressed;
    bool savePressed;
    bool quitPressed;
} EldoriaInput;

typedef struct EldoriaGame {
    EldoriaPlayer player;
    EldoriaWorld world;
    EldoriaQuest quest;
    Camera3D camera;
    EldoriaInput input;
    EldoriaGameState state;
    int menuSelection;
    float introTimer;
    bool showHelp;
    bool cursorCaptured;
    bool discardNextMouseDelta;
    float messageTimer;
    char message[192];
    bool primeUnlocked;
    float endingTimer;
    int creditsPage;
    /* Novos dados ficam no fim para permitir a migração do save 0.1.4. */
    EldoriaPet pet;
    EldoriaSettings settings;
} EldoriaGame;

#endif
