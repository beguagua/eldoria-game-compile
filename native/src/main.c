/* Eldoria design reminder: natural fantasy, amber/cyan mineral energy, readable low-poly silhouettes and low-cost 3D atmosphere. */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "raylib.h"
#include "raymath.h"
#include "eldoria/config.h"
#include "eldoria/types.h"
#include "eldoria/net.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif
#if defined(PLATFORM_ANDROID)
#include <unistd.h>
#include <jni.h>
#include <android_native_app_glue.h>
extern struct android_app *GetAndroidApp(void);
#endif

static const Color MOSS = { 73, 103, 71, 255 };
static const Color DEEP_MOSS = { 37, 61, 49, 255 };
static const Color AMBER = { 235, 169, 74, 255 };
static const Color AURORA = { 93, 183, 190, 255 };
static const Color CLAY = { 171, 91, 64, 255 };
static const Color IVORY = { 245, 237, 211, 255 };
static const Color COBALT = { 62, 116, 222, 255 };
static const Color RUBY = { 224, 79, 47, 255 };
static const char *SAVE_PATH = "eldoria_save.dat";

typedef struct EldoriaAudio {
    bool ready;
    Sound attack;
    Sound hit;
    Sound pickup;
    Sound craft;
    Sound portal;
    Sound hurt;
    Sound victory;
    Sound endingVoice;
    Music endingTheme;
} EldoriaAudio;
static EldoriaAudio gAudio = {0};

typedef struct EldoriaVisuals {
    Texture2D texGrass;
    Texture2D texStone;
    Texture2D texWood;
    Texture2D texAurora;
    Texture2D texPetAtlas;
    Texture2D texMaterialTiles[8];
    Texture2D texFaces[20];
    Model modelBeveledCube;
} EldoriaVisuals;
static EldoriaVisuals gVisuals = {0};
static bool gVisualAssetsReady = false;
static Shader gWorldShader = {0};
static bool gWorldShaderReady = false;
static float gShaderTime = 0.0f;
static EldoriaNetwork gNetwork = {0};
static char gNetworkAddress[64] = "127.0.0.1";
static char gNetworkPort[8] = "27815";
static int gNetworkField = 0;
static bool gShaderPickerPending = false;
static const char *gShaderPickerResult = "eldoria_imported_shader.fs";


/* Eldoria mobile controls: touch regions stay separate from gameplay physics so desktop input remains unchanged. */
typedef struct EldoriaTouchState {
    bool nowAny;
    bool previousAny;
    bool nowAttack;
    bool previousAttack;
    bool nowInteract;
    bool previousInteract;
    bool nowBuild;
    bool previousBuild;
    bool nowInventory;
    bool previousInventory;
    bool nowCrafting;
    bool previousCrafting;
    bool nowPet;
    bool previousPet;
    bool nowSprint;
    bool previousSprint;
    Vector2 move;
    Vector2 lookDelta;
} EldoriaTouchState;

static EldoriaTouchState gTouch = {0};

static bool LoadGame(EldoriaGame *g);
static bool SaveGame(const EldoriaGame *g);

static Sound LoadSoundOptional(const char *path)
{
#if defined(PLATFORM_ANDROID)
    // FileExists() usa access(), que não enxerga arquivos somente leitura dentro
    // do APK. LoadSound() passa pelo android_fopen/AAssetManager corretamente.
    return LoadSound(path);
#else
    return FileExists(path) ? LoadSound(path) : (Sound){0};
#endif
}

static Texture2D LoadTextureOptional(const char *path, Color fallbackA, Color fallbackB)
{
    Texture2D texture = {0};
#if defined(PLATFORM_ANDROID)
    // Assets somente leitura dentro do APK não aparecem em access()/FileExists().
    // LoadTexture() usa o android_fopen/AAssetManager diretamente.
    texture = LoadTexture(path);
#else
    if (FileExists(path)) texture = LoadTexture(path);
#endif
    if (!IsTextureValid(texture)) {
        // Nunca deixe o material receber texture.id == 0: no OpenGL ES isso
        // costuma resultar em uma cena inteira preta. O fallback também mantém
        // o jogo jogável quando um pacote de assets está incompleto.
        Image checker = GenImageChecked(64, 64, 8, 8, fallbackA, fallbackB);
        texture = LoadTextureFromImage(checker);
        UnloadImage(checker);
        TraceLog(LOG_WARNING, "ELDORIA: textura ausente, usando fallback para %s", path);
    }
    return texture;
}

static void LoadVisualAssets(void)
{
    gVisuals.texGrass = LoadTextureOptional("assets/textures/grass_stylized.png", (Color){70, 132, 77, 255}, (Color){42, 92, 57, 255});
    gVisuals.texStone = LoadTextureOptional("assets/textures/stone_stylized.png", (Color){112, 108, 102, 255}, (Color){69, 68, 70, 255});
    gVisuals.texWood = LoadTextureOptional("assets/textures/wood_stylized.png", (Color){145, 92, 55, 255}, (Color){85, 49, 35, 255});
    gVisuals.texAurora = LoadTextureOptional("assets/textures/ore_aurora.png", (Color){62, 142, 170, 255}, (Color){178, 92, 206, 255});
    gVisuals.texPetAtlas = LoadTextureOptional("assets/textures/pet_atlas_015.png", (Color){25, 35, 40, 255}, (Color){12, 18, 23, 255});
    Image materialAtlas = LoadImage("assets/textures/material_atlas_015.png");
    if (materialAtlas.data != NULL && materialAtlas.width >= 4 && materialAtlas.height >= 2) {
        int tileW = materialAtlas.width / 4;
        int tileH = materialAtlas.height / 2;
        for (int i = 0; i < 8; ++i) {
            Rectangle rec = { (float)((i % 4) * tileW), (float)((i / 4) * tileH), (float)tileW, (float)tileH };
            Image tile = ImageFromImage(materialAtlas, rec);
            if (tile.data != NULL) {
                ImageFlipVertical(&tile);
                gVisuals.texMaterialTiles[i] = LoadTextureFromImage(tile);
            }
            UnloadImage(tile);
        }
        UnloadImage(materialAtlas);
    }

    /* O atlas é 5 x 4. Cada recorte é carregado separadamente para impedir que
       o quadro inteiro do atlas apareça na cabeça dos NPCs. */
    Image atlas = LoadImage("assets/textures/character_faces_015.png");
    if (atlas.data != NULL && atlas.width >= 5 && atlas.height >= 4) {
        const int atlasCols = 5;
        const int atlasRows = 4;
        const int faceW = atlas.width / atlasCols;
        const int faceH = atlas.height / atlasRows;
        for (int i = 0; i < 20; i++) {
            int col = i % atlasCols;
            int row = i / atlasCols;
            Rectangle rec = { (float)(col * faceW), (float)(row * faceH), (float)faceW, (float)faceH };
            Image faceImg = ImageFromImage(atlas, rec);
            if (faceImg.data != NULL && faceImg.width > 0 && faceImg.height > 0) {
                // O UV do cubo 3D usa a origem vertical oposta à ordem de linhas
                // do PNG. Virar cada célula, e não o atlas inteiro, preserva a
                // posição de cada NPC e deixa o rosto na orientação correta.
                ImageFlipVertical(&faceImg);
                gVisuals.texFaces[i] = LoadTextureFromImage(faceImg);
            }
            UnloadImage(faceImg);
        }
        UnloadImage(atlas);
    }

    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    if (cube.vertexCount > 0) {
        gVisuals.modelBeveledCube = LoadModelFromMesh(cube);
    }
    gVisualAssetsReady = gVisuals.modelBeveledCube.meshCount > 0 &&
        gVisuals.modelBeveledCube.materialCount > 0 &&
        gVisuals.modelBeveledCube.materials != NULL &&
        gVisuals.modelBeveledCube.materials[0].maps != NULL;
}

static void LoadWorldShader(const EldoriaSettings *settings)
{
    const char *fragmentPath = NULL;
    const char *vertexPath = NULL;
#if defined(PLATFORM_ANDROID)
    fragmentPath = settings->shaderPath[0] != '\0' ? settings->shaderPath : "assets/shaders/eldoria_realistic_gles.fs";
    vertexPath = "assets/shaders/eldoria_realistic_gles.vs";
#else
    fragmentPath = settings->shaderPath[0] != '\0' ? settings->shaderPath : "assets/shaders/eldoria_realistic.fs";
    vertexPath = "assets/shaders/eldoria_realistic.vs";
#endif
    bool packageAvailable = settings->shaderPath[0] == '\0' || FileExists(fragmentPath);
    if (settings->shaderEnabled && packageAvailable) {
        gWorldShader = LoadShader(vertexPath, fragmentPath);
        gWorldShaderReady = IsShaderValid(gWorldShader);
        if (!gWorldShaderReady) TraceLog(LOG_WARNING, "ELDORIA: shader inválido, usando iluminação compatível.");
    }
}

static void UnloadWorldShader(void)
{
    if (gWorldShaderReady) UnloadShader(gWorldShader);
    gWorldShader = (Shader){0};
    gWorldShaderReady = false;
}

static void UnloadVisualAssets(void)
{
    if (IsTextureValid(gVisuals.texGrass)) UnloadTexture(gVisuals.texGrass);
    if (IsTextureValid(gVisuals.texStone)) UnloadTexture(gVisuals.texStone);
    if (IsTextureValid(gVisuals.texWood)) UnloadTexture(gVisuals.texWood);
    if (IsTextureValid(gVisuals.texAurora)) UnloadTexture(gVisuals.texAurora);
    if (IsTextureValid(gVisuals.texPetAtlas)) UnloadTexture(gVisuals.texPetAtlas);
    for (int i = 0; i < 8; ++i) if (IsTextureValid(gVisuals.texMaterialTiles[i])) UnloadTexture(gVisuals.texMaterialTiles[i]);
    for (int i = 0; i < 20; i++) if (IsTextureValid(gVisuals.texFaces[i])) UnloadTexture(gVisuals.texFaces[i]);
    if (gVisuals.modelBeveledCube.meshCount > 0) UnloadModel(gVisuals.modelBeveledCube);
    gVisuals = (EldoriaVisuals){0};
    gVisualAssetsReady = false;
}

static void LoadAudioAssets(void)
{
#if defined(PLATFORM_ANDROID) && !defined(ELDORIA_ENABLE_ANDROID_AUDIO)
    /* Alguns aparelhos encerram NativeActivity quando o backend de áudio não
       consegue abrir OpenSL ES no primeiro frame. O jogo permanece jogável e
       pode ser recompilado com ELDORIA_ENABLE_ANDROID_AUDIO para habilitar áudio. */
    TraceLog(LOG_WARNING, "ELDORIA: áudio Android desabilitado no modo seguro");
    return;
#endif

    InitAudioDevice();
    gAudio.ready = IsAudioDeviceReady();
    if (!gAudio.ready) return;
    gAudio.attack = LoadSoundOptional("assets/audio/sfx_attack.wav");
    gAudio.hit = LoadSoundOptional("assets/audio/sfx_hit.wav");
    gAudio.pickup = LoadSoundOptional("assets/audio/sfx_pickup.wav");
    gAudio.craft = LoadSoundOptional("assets/audio/sfx_craft.wav");
    gAudio.portal = LoadSoundOptional("assets/audio/sfx_portal.wav");
    gAudio.hurt = LoadSoundOptional("assets/audio/sfx_hurt.wav");
    gAudio.victory = LoadSoundOptional("assets/audio/sfx_victory.wav");
    gAudio.endingVoice = LoadSoundOptional("assets/audio/eldoria_ending_voice.wav");
    if (FileExists("assets/audio/eldoria_ending_theme.wav")) {
        gAudio.endingTheme = LoadMusicStream("assets/audio/eldoria_ending_theme.wav");
        SetMusicVolume(gAudio.endingTheme, 0.52f);
    }
}

static void PlaySfx(Sound sound)
{
    if (gAudio.ready && IsSoundValid(sound)) PlaySound(sound);
}

static void UnloadAudioAssets(void)
{
    if (!gAudio.ready) return;
    UnloadSound(gAudio.attack); UnloadSound(gAudio.hit); UnloadSound(gAudio.pickup);
    UnloadSound(gAudio.craft); UnloadSound(gAudio.portal); UnloadSound(gAudio.hurt);
    UnloadSound(gAudio.victory); UnloadSound(gAudio.endingVoice);
    UnloadMusicStream(gAudio.endingTheme);
    CloseAudioDevice();
    gAudio = (EldoriaAudio){0};
}

static float ClampFloat(float v, float min, float max) { return v < min ? min : (v > max ? max : v); }

static float Hash01(int x, int z, uint32_t seed)
{
    uint32_t h = seed ^ ((uint32_t)x * 374761393u) ^ ((uint32_t)z * 668265263u);
    h = (h ^ (h >> 13u)) * 1274126177u;
    return (float)((h ^ (h >> 16u)) & 0xffffu) / 65535.0f;
}

static float TerrainHeight(float x, float z)
{
    return sinf(x * 0.055f) * 0.55f + cosf(z * 0.045f) * 0.4f + sinf((x + z) * 0.14f) * 0.16f;
}

static float RealmGround(EldoriaRealm realm, float x, float z)
{
    if (realm == ELDORIA_REALM_CAVES) return -2.1f + sinf(x * 0.12f) * 0.15f + cosf(z * 0.11f) * 0.12f;
    if (realm == ELDORIA_REALM_COBALT) return 1.4f + sinf(x * 0.1f + z * 0.12f) * 0.25f;
    if (realm == ELDORIA_REALM_FURNACE) return -0.4f + sinf(x * 0.16f) * 0.28f + cosf(z * 0.1f) * 0.18f;
    return TerrainHeight(x, z);
}

static const char *RealmName(EldoriaRealm realm)
{
    static const char *names[] = { "Vale das Lanternas", "Grutas de Basalto", "Fenda de Cobalto", "Fornalha Rubra" };
    return names[realm];
}

static Color RealmAccent(EldoriaRealm realm)
{
    static const Color colors[] = { {93,183,190,255}, {190,128,79,255}, {62,116,222,255}, {224,79,47,255} };
    return colors[realm];
}

static float ArmorDefense(int tier)
{
    static const float values[] = { 0.0f, 0.14f, 0.28f, 0.42f, 0.58f };
    if (tier < 0) tier = 0;
    if (tier > 4) tier = 4;
    return values[tier];
}
static const char *ArmorName(int tier)
{
    static const char *names[] = { "Nenhuma", "Couraça de Aurora", "Armadura de Ferro Vivo", "Égide de Cobalto", "Placa da Fornalha Rubra" };
    if (tier < 0) tier = 0;
    if (tier > 4) tier = 4;
    return names[tier];
}
static Color ArmorColor(int tier)
{
    static const Color colors[] = { {82,92,90,255}, {93,183,190,255}, {174,155,129,255}, {62,116,222,255}, {224,79,47,255} };
    if (tier < 0) tier = 0;
    if (tier > 4) tier = 4;
    return colors[tier];
}
static void SetMessage(EldoriaGame *game, const char *text)
{
    snprintf(game->message, sizeof(game->message), "%s", text);
    game->messageTimer = 4.5f;
}

static void DefaultSettings(EldoriaSettings *settings)
{
    memset(settings, 0, sizeof(*settings));
    settings->shaderEnabled = false;
    settings->highQuality = false;
}

static void SaveSettings(const EldoriaSettings *settings)
{
    FILE *file = fopen("eldoria_settings.cfg", "wb");
    if (file == NULL) return;
    fprintf(file, "shader=%d\nquality=%d\npath=%s\n", settings->shaderEnabled ? 1 : 0, settings->highQuality ? 1 : 0, settings->shaderPath);
    fclose(file);
}

static void LoadSettings(EldoriaSettings *settings)
{
    DefaultSettings(settings);
    FILE *file = fopen("eldoria_settings.cfg", "rb");
    if (file == NULL) return;
    char line[320];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "shader=1", 8) == 0) settings->shaderEnabled = true;
        if (strncmp(line, "quality=1", 9) == 0) settings->highQuality = true;
        if (strncmp(line, "path=", 5) == 0) {
            snprintf(settings->shaderPath, sizeof(settings->shaderPath), "%s", line + 5);
            settings->shaderPath[strcspn(settings->shaderPath, "\r\n")] = '\0';
        }
    }
    fclose(file);
}

static const char *PetName(EldoriaPetKind kind)
{
    static const char *names[] = { "Raposa de Aurora", "Coruja de Musgo", "Salamandra Brasa", "Lebre de Cobalto" };
    if (kind < 0 || kind >= ELDORIA_PET_KIND_COUNT) return "Companheiro";
    return names[kind];
}

static Color PetColor(EldoriaPetKind kind)
{
    static const Color colors[] = { {73, 205, 218, 255}, {151, 174, 76, 255}, {232, 93, 46, 255}, {78, 132, 232, 255} };
    if (kind < 0 || kind >= ELDORIA_PET_KIND_COUNT) return AURORA;
    return colors[kind];
}

static void SetPet(EldoriaGame *game, EldoriaPetKind kind)
{
    game->pet.active = true;
    game->pet.kind = kind;
    game->pet.position = Vector3Add(game->player.position, (Vector3){-1.2f, 0.45f, -1.2f});
    game->pet.followDistance = 1.35f;
    game->pet.mood = 0.0f;
    SetMessage(game, TextFormat("Companheiro: %s", PetName(kind)));
}

static void AddItem(EldoriaPlayer *player, EldoriaResourceKind kind, int amount)
{
    for (int i = 0; i < player->inventoryCount; ++i) if (player->inventory[i].kind == kind) { player->inventory[i].amount += amount; return; }
    if (player->inventoryCount < ELDORIA_MAX_INVENTORY) player->inventory[player->inventoryCount++] = (EldoriaItemStack){ kind, amount };
}

static int CountItem(const EldoriaPlayer *player, EldoriaResourceKind kind)
{
    for (int i = 0; i < player->inventoryCount; ++i) if (player->inventory[i].kind == kind) return player->inventory[i].amount;
    return 0;
}

static bool HasItems(const EldoriaPlayer *p, EldoriaResourceKind a, int aa, EldoriaResourceKind b, int bb)
{
    return CountItem(p, a) >= aa && CountItem(p, b) >= bb;
}

static bool ConsumeItem(EldoriaPlayer *player, EldoriaResourceKind kind, int amount)
{
    for (int i = 0; i < player->inventoryCount; ++i) {
        if (player->inventory[i].kind == kind && player->inventory[i].amount >= amount) { player->inventory[i].amount -= amount; return true; }
    }
    return false;
}

static void SetQuest(EldoriaGame *game, EldoriaQuestStage stage)
{
    EldoriaQuest *q = &game->quest;
    q->stage = stage; q->complete = false; q->progress = 0;
    switch (stage) {
        case ELDORIA_QUEST_LANTERNS:
            q->target = 3; snprintf(q->title, sizeof(q->title), "As Lanternas do Vale"); snprintf(q->description, sizeof(q->description), "Colete cristais de Aurora e reacenda a lanterna antiga."); break;
        case ELDORIA_QUEST_CAVES:
            q->target = 1; snprintf(q->title, sizeof(q->title), "Ecos Sob a Pedra"); snprintf(q->description, sizeof(q->description), "Entre nas Grutas de Basalto e vença o Escavador de Cinzas."); break;
        case ELDORIA_QUEST_COBALT:
            q->target = 1; snprintf(q->title, sizeof(q->title), "A Chave de Cobalto"); snprintf(q->description, sizeof(q->description), "Forje uma chave de portal e enfrente o Vigia de Cobalto."); break;
        case ELDORIA_QUEST_FURNACE:
            q->target = 1; snprintf(q->title, sizeof(q->title), "Coração da Fornalha"); snprintf(q->description, sizeof(q->description), "Atravesse a Fornalha Rubra e derrote o Arconte."); break;
        default:
            q->target = 1; q->complete = true; snprintf(q->title, sizeof(q->title), "Ecos das Três Fendas"); snprintf(q->description, sizeof(q->description), "O equilíbrio de Eldoria foi restaurado."); break;
    }
}

static void AddResource(EldoriaWorld *world, EldoriaResourceKind kind, Vector3 pos, int amount)
{
    if (world->resourceCount >= ELDORIA_MAX_RESOURCES) return;
    EldoriaResourceNode *n = &world->resources[world->resourceCount++];
    *n = (EldoriaResourceNode){ true, kind, pos, 0.42f, amount };
    n->position.y = RealmGround(world->realm, pos.x, pos.z) + 0.45f;
}

static float RealmThreat(EldoriaRealm realm)
{
    if (realm == ELDORIA_REALM_CAVES) return 1.28f;
    if (realm == ELDORIA_REALM_COBALT) return 1.62f;
    if (realm == ELDORIA_REALM_FURNACE) return 2.05f;
    return 1.0f;
}

static void AddEnemy(EldoriaWorld *world, EldoriaEntityKind kind, Vector3 pos, float hp, bool boss)
{
    if (world->enemyCount >= ELDORIA_MAX_ENTITIES) return;
    float threat = RealmThreat(world->realm);
    if (boss) threat += 0.22f;
    hp *= threat;
    EldoriaEnemy *e = &world->enemies[world->enemyCount++];
    *e = (EldoriaEnemy){ true, kind, pos, {0, 0, 0}, hp, hp, 0.4f, 0.0f, 0.0f, boss };
    e->position.y = RealmGround(world->realm, pos.x, pos.z) + (boss ? 1.1f : 0.6f);
}

static void AddPortal(EldoriaWorld *world, Vector3 pos, EldoriaRealm target, Color color)
{
    if (world->buildingCount >= ELDORIA_MAX_BUILDINGS) return;
    EldoriaBuilding *b = &world->buildings[world->buildingCount++];
    *b = (EldoriaBuilding){ true, ELDORIA_BUILDING_PORTAL, target, pos, {1.5f, 2.7f, 0.38f}, color };
    b->position.y = RealmGround(world->realm, pos.x, pos.z) + 1.35f;
}

static void PopulateRealm(EldoriaGame *game, EldoriaRealm realm)
{
    EldoriaWorld *w = &game->world;
    w->realm = realm; w->resourceCount = 0; w->enemyCount = 0; w->buildingCount = 0; w->npcCount = 0;
    memset(w->resources, 0, sizeof(w->resources)); memset(w->enemies, 0, sizeof(w->enemies)); memset(w->buildings, 0, sizeof(w->buildings));
    for (int i = 0; i < ELDORIA_MAX_PARTICLES; ++i) w->particles[i] = (EldoriaParticle){ {(float)((i * 17) % 49 - 24), 0.3f + (float)(i % 7) * 0.23f, (float)((i * 29) % 49 - 24)}, (float)i * 0.61f, 0.5f + (float)(i % 5) * 0.12f };

    if (realm == ELDORIA_REALM_VALLEY) {
        for (int x = -24; x <= 24; x += 6) for (int z = -18; z <= 30; z += 6) {
            float r = Hash01(x, z, (uint32_t)w->seed);
            AddResource(w, r > 0.78f ? ELDORIA_RESOURCE_CRYSTAL : (r > 0.45f ? ELDORIA_RESOURCE_WOOD : ELDORIA_RESOURCE_STONE), (Vector3){x + (r - .5f) * 2.0f,0,z}, 1 + (int)(r * 3));
        }
        AddEnemy(w, ELDORIA_ENTITY_RISCANTE, (Vector3){5,0,10}, 58, false); AddEnemy(w, ELDORIA_ENTITY_ESPORO, (Vector3){-10,0,15}, 48, false); AddEnemy(w, ELDORIA_ENTITY_RISCANTE, (Vector3){-6,0,22}, 54, false);
        w->buildings[w->buildingCount++] = (EldoriaBuilding){ true, ELDORIA_BUILDING_LANTERN, ELDORIA_REALM_VALLEY, {0, RealmGround(realm,0,7)+.9f,7}, {.45f,1.8f,.45f}, AMBER };
        w->buildings[w->buildingCount++] = (EldoriaBuilding){ true, ELDORIA_BUILDING_WORKBENCH, ELDORIA_REALM_VALLEY, {3, RealmGround(realm,3,1)+.5f,1}, {1.2f,1.0f,.7f}, CLAY };
        AddPortal(w, (Vector3){-14,0,20}, ELDORIA_REALM_CAVES, (Color){190,128,79,255});
        w->npcs[0] = (EldoriaNPC){ true, {2.5f,0,5}, {2.5f,0,5}, 0, 0, 0, "Maela", "guardiã das lanternas" };
        w->npcs[0].position.y = RealmGround(realm, 2.5f, 5) + .9f; w->npcs[0].home.y = w->npcs[0].position.y;
        w->npcs[1] = (EldoriaNPC){ true, {-2.8f,0,5.8f}, {-2.8f,0,5.8f}, 0, 0, 0, "Bruma", "cartógrafa das fendas" };
        w->npcs[2] = (EldoriaNPC){ true, {5.2f,0,6.8f}, {5.2f,0,6.8f}, 0, 0, 0, "Orin", "artífice do Vale" };
        w->npcs[3] = (EldoriaNPC){ true, {-5.5f,0,8.2f}, {-5.5f,0,8.2f}, 0, 0, 0, "Tessa", "guardiã das sementes" };
        for (int i = 1; i < 4; ++i) { w->npcs[i].position.y = RealmGround(realm, w->npcs[i].position.x, w->npcs[i].position.z) + .9f; w->npcs[i].home.y = w->npcs[i].position.y; }
        w->npcCount = 4;
        if (w->bossDefeated[ELDORIA_REALM_CAVES]) AddPortal(w, (Vector3){14,0,20}, ELDORIA_REALM_COBALT, COBALT);
        if (w->bossDefeated[ELDORIA_REALM_COBALT]) AddPortal(w, (Vector3){-4,0,29}, ELDORIA_REALM_FURNACE, RUBY);
    } else if (realm == ELDORIA_REALM_CAVES) {
        for (int i = 0; i < 32; ++i) AddResource(w, (i % 3 == 0) ? ELDORIA_RESOURCE_IRON : ELDORIA_RESOURCE_STONE, (Vector3){(float)((i*13)%47-23),0,(float)((i*19)%45-22)}, 2);
        AddEnemy(w, ELDORIA_ENTITY_ESCAVADOR, (Vector3){8,0,8}, 92, false); AddEnemy(w, ELDORIA_ENTITY_ESPORO, (Vector3){-8,0,12}, 60, false); AddEnemy(w, ELDORIA_ENTITY_ESCAVADOR, (Vector3){-13,0,18}, 82, false);
        if (!w->bossDefeated[realm]) AddEnemy(w, ELDORIA_ENTITY_BOSS_ESCAVADOR, (Vector3){0,0,17}, 280, true);
        AddPortal(w, (Vector3){0,0,-20}, ELDORIA_REALM_VALLEY, AURORA);
    } else if (realm == ELDORIA_REALM_COBALT) {
        for (int i = 0; i < 34; ++i) AddResource(w, ELDORIA_RESOURCE_COBALT, (Vector3){(float)((i*11)%49-24),0,(float)((i*17)%49-24)}, 1 + i % 2);
        AddEnemy(w, ELDORIA_ENTITY_SENTINELA, (Vector3){9,0,7}, 105, false); AddEnemy(w, ELDORIA_ENTITY_SENTINELA, (Vector3){-10,0,12}, 105, false); AddEnemy(w, ELDORIA_ENTITY_SENTINELA, (Vector3){12,0,20}, 92, false);
        if (!w->bossDefeated[realm]) AddEnemy(w, ELDORIA_ENTITY_BOSS_COBALTO, (Vector3){0,0,18}, 350, true);
        AddPortal(w, (Vector3){0,0,-20}, ELDORIA_REALM_VALLEY, AURORA);
    } else {
        for (int i = 0; i < 28; ++i) AddResource(w, ELDORIA_RESOURCE_RUBYCORE, (Vector3){(float)((i*7)%43-21),0,(float)((i*23)%43-21)}, 1);
        AddEnemy(w, ELDORIA_ENTITY_BRASA, (Vector3){8,0,8}, 132, false); AddEnemy(w, ELDORIA_ENTITY_BRASA, (Vector3){-10,0,9}, 132, false); AddEnemy(w, ELDORIA_ENTITY_BRASA, (Vector3){13,0,18}, 118, false);
        if (!w->bossDefeated[realm]) AddEnemy(w, ELDORIA_ENTITY_BOSS_FORNALHA, (Vector3){0,0,17}, 430, true);
        AddPortal(w, (Vector3){0,0,-20}, ELDORIA_REALM_VALLEY, AURORA);
    }
}

static void InitGame(EldoriaGame *game)
{
    memset(game, 0, sizeof(*game)); game->world.seed = ELDORIA_WORLD_SEED; game->world.timeOfDay = .32f;
    game->player.position = (Vector3){0, TerrainHeight(0,0)+1, 0}; game->player.health = game->player.maxHealth = 100; game->player.stamina = game->player.maxStamina = 100; game->player.level = 1; game->player.armorTier = 0;
    AddItem(&game->player, ELDORIA_RESOURCE_WOOD, 6); AddItem(&game->player, ELDORIA_RESOURCE_STONE, 4); AddItem(&game->player, ELDORIA_RESOURCE_BERRY, 2);
    PopulateRealm(game, ELDORIA_REALM_VALLEY); SetQuest(game, ELDORIA_QUEST_LANTERNS); SetPet(game, ELDORIA_PET_AURORA_FOX); LoadSettings(&game->settings); game->state = ELDORIA_STATE_PLAYING;
}

static Rectangle TouchAttackRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 164), (float)(h - 190), 108, 108}; }
static Rectangle TouchInteractRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 270), (float)(h - 130), 68, 68}; }
static Rectangle TouchBuildRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 330), (float)(h - 220), 68, 68}; }
static Rectangle TouchInventoryRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 160), (float)(h - 300), 68, 68}; }
static Rectangle TouchCraftingRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 250), (float)(h - 300), 68, 68}; }
static Rectangle TouchPetRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 420), (float)(h - 126), 68, 68}; }
static Rectangle TouchSprintRect(void) { int w = GetScreenWidth(), h = GetScreenHeight(); return (Rectangle){(float)(w - 340), (float)(h - 126), 68, 68}; }

static bool TouchPointIn(Rectangle rect, Vector2 point) { return CheckCollisionPointRec(point, rect); }

static bool TouchAnyPointIn(Rectangle rect)
{
    for (int i = 0; i < GetTouchPointCount(); ++i) if (TouchPointIn(rect, GetTouchPosition(i))) return true;
    return false;
}

static void UpdateTouchFrame(void)
{
    gTouch.previousAny = gTouch.nowAny;
    gTouch.previousAttack = gTouch.nowAttack;
    gTouch.previousInteract = gTouch.nowInteract;
    gTouch.previousBuild = gTouch.nowBuild;
    gTouch.previousInventory = gTouch.nowInventory;
    gTouch.previousCrafting = gTouch.nowCrafting;
    gTouch.previousPet = gTouch.nowPet;
    gTouch.previousSprint = gTouch.nowSprint;
    gTouch.nowAny = GetTouchPointCount() > 0;
    gTouch.nowAttack = TouchAnyPointIn(TouchAttackRect());
    gTouch.nowInteract = TouchAnyPointIn(TouchInteractRect());
    gTouch.nowBuild = TouchAnyPointIn(TouchBuildRect());
    gTouch.nowInventory = TouchAnyPointIn(TouchInventoryRect());
    gTouch.nowCrafting = TouchAnyPointIn(TouchCraftingRect());
    gTouch.nowPet = TouchAnyPointIn(TouchPetRect());
    gTouch.nowSprint = TouchAnyPointIn(TouchSprintRect());

    gTouch.move = (Vector2){0, 0};
    int width = GetScreenWidth(), height = GetScreenHeight();
    Vector2 moveCenter = {(float)(96), (float)(height - 112)};
    Rectangle moveRegion = {0, (float)(height - 238), (float)(width / 2), 238};
    for (int i = 0; i < GetTouchPointCount(); ++i) {
        Vector2 point = GetTouchPosition(i);
        if (TouchPointIn(moveRegion, point)) {
            Vector2 delta = Vector2Subtract(point, moveCenter);
            if (Vector2Length(delta) > 72.0f) delta = Vector2Scale(Vector2Normalize(delta), 72.0f);
            gTouch.move = (Vector2){delta.x / 72.0f, -delta.y / 72.0f};
            break;
        }
    }

    gTouch.lookDelta = (Vector2){0, 0};
    Rectangle lookRegion = { (float)(width / 2), 0, (float)(width / 2), (float)(height - 238) };
    static int lookTouchId = -1;
    static Vector2 previousLookPoint = {0, 0};
    bool foundLook = false;
    for (int i = 0; i < GetTouchPointCount(); ++i) {
        int id = GetTouchPointId(i);
        Vector2 point = GetTouchPosition(i);
        if (id == lookTouchId && TouchPointIn(lookRegion, point)) {
            gTouch.lookDelta = Vector2Subtract(point, previousLookPoint);
            previousLookPoint = point;
            foundLook = true;
            break;
        }
    }
    if (!foundLook) {
        for (int i = 0; i < GetTouchPointCount(); ++i) {
            Vector2 point = GetTouchPosition(i);
            if (TouchPointIn(lookRegion, point) && !TouchAnyPointIn(TouchAttackRect())) {
                lookTouchId = GetTouchPointId(i);
                previousLookPoint = point;
                foundLook = true;
                break;
            }
        }
    }
    if (!foundLook) lookTouchId = -1;
}

static EldoriaInput ReadInput(bool readMouse)
{
    EldoriaInput in = {0};
    /* W avança na direção que a câmera olha; S recua. */
    in.move = (Vector2){(float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)), (float)(IsKeyDown(KEY_W) - IsKeyDown(KEY_S))};
    in.move = Vector2Add(in.move, gTouch.move);
    in.look = readMouse ? GetMouseDelta() : (Vector2){0, 0};
    in.look = Vector2Add(in.look, gTouch.lookDelta);
    in.sprint = IsKeyDown(KEY_LEFT_SHIFT) || gTouch.nowSprint;
    in.attackPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_F) || (gTouch.nowAttack && !gTouch.previousAttack);
    in.interactPressed = IsKeyPressed(KEY_E) || (gTouch.nowInteract && !gTouch.previousInteract);
    in.buildPressed = IsKeyPressed(KEY_B) || (gTouch.nowBuild && !gTouch.previousBuild);
    in.inventoryPressed = IsKeyPressed(KEY_TAB) || (gTouch.nowInventory && !gTouch.previousInventory);
    in.craftingPressed = IsKeyPressed(KEY_C) || (gTouch.nowCrafting && !gTouch.previousCrafting);
    in.petPressed = IsKeyPressed(KEY_P) || (gTouch.nowPet && !gTouch.previousPet);
    in.craftOnePressed = IsKeyPressed(KEY_ONE); in.craftTwoPressed = IsKeyPressed(KEY_TWO); in.craftThreePressed = IsKeyPressed(KEY_THREE); in.craftFourPressed = IsKeyPressed(KEY_FOUR); in.craftFivePressed = IsKeyPressed(KEY_FIVE); in.craftSixPressed = IsKeyPressed(KEY_SIX); in.craftSevenPressed = IsKeyPressed(KEY_SEVEN); in.craftEightPressed = IsKeyPressed(KEY_EIGHT);
    in.savePressed = IsKeyPressed(KEY_F5); in.quitPressed = IsKeyPressed(KEY_ESCAPE); return in;
}

static bool TouchTapped(void) { return gTouch.nowAny && !gTouch.previousAny; }

static void UpdateCursorCapture(EldoriaGame *game)
{
    bool wantsCapture = game->state == ELDORIA_STATE_PLAYING;
    if (wantsCapture && !game->cursorCaptured) {
        DisableCursor();
        SetMousePosition(GetScreenWidth()/2, GetScreenHeight()/2);
        game->cursorCaptured = true;
        game->discardNextMouseDelta = true;
    } else if (!wantsCapture && game->cursorCaptured) {
        EnableCursor();
        game->cursorCaptured = false;
        game->discardNextMouseDelta = true;
    }
}

static void UpdateFirstPersonLook(EldoriaGame *game)
{
    EldoriaPlayer *p = &game->player;
    Vector2 mouse = game->input.look;
    const float sensitivity = 0.00215f;

    /* Ignora deltas anormais causados pela captura/liberação do cursor. */
    mouse.x = ClampFloat(mouse.x, -90.0f, 90.0f);
    mouse.y = ClampFloat(mouse.y, -90.0f, 90.0f);

    /* Mouse à direita gira a visão para a direita; para cima olha para cima. */
    p->yaw += mouse.x * sensitivity;
    p->pitch = ClampFloat(p->pitch - mouse.y * sensitivity, -1.15f, 1.15f);

    /* Evita perda de precisão depois de longas sessões de jogo. */
    if (p->yaw > PI) p->yaw -= 2.0f * PI;
    if (p->yaw < -PI) p->yaw += 2.0f * PI;
}

static void UpdatePlayer(EldoriaGame *game, float dt)
{
    EldoriaPlayer *p = &game->player;
    if (game->primeUnlocked) { p->health = p->maxHealth; p->stamina = p->maxStamina; }
    Vector2 m = game->input.move; if (Vector2Length(m) > 1) m = Vector2Normalize(m);
    float speed = game->input.sprint && p->stamina > 0 ? 5.6f : 3.5f; p->stamina = ClampFloat(p->stamina + ((game->input.sprint && Vector2Length(m)>0) ? -18.f : 24.f) * dt, 0, p->maxStamina);
    Vector3 forward = {sinf(p->yaw),0,cosf(p->yaw)}, right = {forward.z,0,-forward.x}; p->velocity = Vector3Scale(Vector3Add(Vector3Scale(right,m.x),Vector3Scale(forward,m.y)), speed); p->position = Vector3Add(p->position, Vector3Scale(p->velocity,dt));
    p->position.x = ClampFloat(p->position.x,-28,28); p->position.z = ClampFloat(p->position.z,-25,31); p->position.y = RealmGround(game->world.realm,p->position.x,p->position.z)+1;
}

static void EnterRealm(EldoriaGame *game, EldoriaRealm realm)
{
    if (realm == ELDORIA_REALM_COBALT && !game->world.bossDefeated[ELDORIA_REALM_CAVES]) { SetMessage(game, "A Fenda de Cobalto permanece selada pelo Escavador."); return; }
    if (realm == ELDORIA_REALM_FURNACE && !game->world.bossDefeated[ELDORIA_REALM_COBALT]) { SetMessage(game, "A Fornalha exige o eco do Vigia de Cobalto."); return; }
    if (realm != ELDORIA_REALM_VALLEY && CountItem(&game->player, ELDORIA_RESOURCE_PORTAL_KEY) < 1 && realm != ELDORIA_REALM_CAVES) { SetMessage(game, "Forje uma Chave de Portal na bancada primeiro."); return; }
    PopulateRealm(game, realm); game->player.position = (Vector3){0, RealmGround(realm,0,-16)+1,-16}; SetMessage(game, TextFormat("Você atravessou para %s.", RealmName(realm)));
}

static void TryInteract(EldoriaGame *game)
{
    EldoriaWorld *w=&game->world; EldoriaPlayer *p=&game->player;
    for(int i=0;i<w->resourceCount;i++) { EldoriaResourceNode *n=&w->resources[i]; if(n->active&&Vector3Distance(p->position,n->position)<ELDORIA_INTERACT_RANGE) { AddItem(p,n->kind,n->amount); n->active=false; PlaySfx(gAudio.pickup); if(n->kind==ELDORIA_RESOURCE_CRYSTAL&&game->quest.stage==ELDORIA_QUEST_LANTERNS&&!game->quest.complete) { game->quest.progress += n->amount; if(game->quest.progress>=game->quest.target){game->quest.complete=true;SetMessage(game,"A Aurora respondeu. Volte à lanterna antiga.");} else SetMessage(game,"Cristal de Aurora coletado."); } else SetMessage(game,"Recurso coletado."); return; } }
    for(int i=0;i<w->buildingCount;i++) { EldoriaBuilding *b=&w->buildings[i]; if(b->active&&Vector3Distance(p->position,b->position)<ELDORIA_INTERACT_RANGE) { if(b->kind==ELDORIA_BUILDING_PORTAL){PlaySfx(gAudio.portal);EnterRealm(game,b->targetRealm);return;} if(b->kind==ELDORIA_BUILDING_WORKBENCH){game->state=ELDORIA_STATE_CRAFTING;SetMessage(game,"Bancada aberta. Escolha uma receita.");return;} if(b->kind==ELDORIA_BUILDING_LANTERN){ if(game->quest.stage==ELDORIA_QUEST_LANTERNS&&game->quest.complete&&!w->restoredLanterns){w->restoredLanterns=1;p->lanternsRestored=1;SetQuest(game,ELDORIA_QUEST_CAVES);SetMessage(game,"Lanterna restaurada. A passagem das Grutas foi revelada.");} else SetMessage(game,"A lanterna reflete as jornadas concluídas.");return;} } }
    for(int i=0;i<w->npcCount;i++) if(w->npcs[i].active&&Vector3Distance(p->position,w->npcs[i].position)<ELDORIA_INTERACT_RANGE){ if(game->quest.stage==ELDORIA_QUEST_CAVES)SetMessage(game,"Maela: O basalto guarda um coração que não dorme."); else if(game->quest.stage==ELDORIA_QUEST_COBALT)SetMessage(game,"Maela: A chave abre a fenda, mas não apaga o medo."); else SetMessage(game,"Maela: Eldoria cresce onde alguém escolhe permanecer."); return; }
}

static void TryBuild(EldoriaGame *game)
{
    if(game->world.realm!=ELDORIA_REALM_VALLEY){SetMessage(game,"A energia das dimensões impede construção aqui.");return;} if(game->world.buildingCount>=ELDORIA_MAX_BUILDINGS){return;} Vector3 f={sinf(game->player.yaw),0,cosf(game->player.yaw)},p=Vector3Add(game->player.position,Vector3Scale(f,2.2f)); p.y=RealmGround(game->world.realm,p.x,p.z)+.75f;
    if(HasItems(&game->player,ELDORIA_RESOURCE_WOOD,2,ELDORIA_RESOURCE_STONE,1)){ConsumeItem(&game->player,ELDORIA_RESOURCE_WOOD,2);ConsumeItem(&game->player,ELDORIA_RESOURCE_STONE,1);game->world.buildings[game->world.buildingCount++]=(EldoriaBuilding){true,ELDORIA_BUILDING_WALL,ELDORIA_REALM_VALLEY,p,{1.6f,1.5f,.35f},CLAY};SetMessage(game,"Estrutura construída.");}else SetMessage(game,"Você precisa de 2 madeiras e 1 pedra.");
}

static void BeginEnding(EldoriaGame *game)
{
    game->state = ELDORIA_STATE_ENDING;
    game->endingTimer = 0.0f;
    game->creditsPage = 0; /* 0 = música e aplausos; 1 = fala e legendas. */
    EnableCursor();
    if (gAudio.ready) {
        PlaySfx(gAudio.victory);
        /* A fala é iniciada somente depois do primeiro movimento musical. */
        if (IsMusicValid(gAudio.endingTheme)) PlayMusicStream(gAudio.endingTheme);
    }
    SetMessage(game, "O Coração da Fornalha silenciou.");
}

static void UnlockPrimeMode(EldoriaGame *game)
{
    game->primeUnlocked = true;
    game->player.maxHealth = 999999.0f;
    game->player.health = game->player.maxHealth;
    game->player.maxStamina = 999999.0f;
    game->player.stamina = game->player.maxStamina;
    for (int kind = 0; kind < ELDORIA_RESOURCE_KIND_COUNT; ++kind) AddItem(&game->player, (EldoriaResourceKind)kind, 999999);
    SaveGame(game);
}

static void RewardBoss(EldoriaGame *game, EldoriaEntityKind kind)
{
    EldoriaPlayer *p=&game->player; game->world.bossDefeated[game->world.realm]=true; p->experience+=220;
    if(kind==ELDORIA_ENTITY_BOSS_ESCAVADOR){AddItem(p,ELDORIA_RESOURCE_IRON,8);SetQuest(game,ELDORIA_QUEST_COBALT);SetMessage(game,"Escavador vencido. A Fenda de Cobalto despertou.");}
    else if(kind==ELDORIA_ENTITY_BOSS_COBALTO){AddItem(p,ELDORIA_RESOURCE_COBALT,8);SetQuest(game,ELDORIA_QUEST_FURNACE);SetMessage(game,"Vigia vencido. A Fornalha Rubra foi liberada.");}
    else {AddItem(p,ELDORIA_RESOURCE_RUBYCORE,3);SetQuest(game,ELDORIA_QUEST_COMPLETE);BeginEnding(game);}
}

static void TryAttack(EldoriaGame *game)
{
    PlaySfx(gAudio.attack);
    Vector3 facing={sinf(game->player.yaw),0,cosf(game->player.yaw)}; float damage=game->primeUnlocked?999999.f:(13.f+game->player.weaponTier*7.f);
    for(int i=0;i<game->world.enemyCount;i++){EldoriaEnemy *e=&game->world.enemies[i];if(!e->active)continue;Vector3 to=Vector3Subtract(e->position,game->player.position);float d=Vector3Length(to);if(d<ELDORIA_ATTACK_RANGE + (e->boss ? .4f : 0.0f)&&Vector3DotProduct(Vector3Normalize(to),facing)>.1f){e->health-=damage;if(e->health<=0){e->active=false;PlaySfx(gAudio.hit);if(e->boss)RewardBoss(game,e->kind);else{game->player.experience+=30;AddItem(&game->player,ELDORIA_RESOURCE_BERRY,1);SetMessage(game,"Inimigo derrotado.");}}else SetMessage(game,e->boss?"O golpe rachou a couraça do boss.":"Golpe certeiro.");return;}}
    SetMessage(game,"O golpe encontra apenas o ar.");
}

static void UpdateEnemies(EldoriaGame *game,float dt)
{
    for(int i=0;i<game->world.enemyCount;i++){EldoriaEnemy *e=&game->world.enemies[i];if(!e->active)continue;Vector3 to=Vector3Subtract(game->player.position,e->position);float d=Vector3Length(to);float speed=(e->boss?1.72f:1.22f)*RealmThreat(game->world.realm); if(e->kind==ELDORIA_ENTITY_SENTINELA)speed=1.7f*RealmThreat(game->world.realm); if(e->kind==ELDORIA_ENTITY_BRASA)speed=2.f*RealmThreat(game->world.realm);e->attackCooldown-=dt;e->phaseTimer+=dt;if(d<10.f)e->velocity=Vector3Scale(Vector3Normalize(to),speed);else {e->velocity=(Vector3){sinf(e->phaseTimer*.7f)*.4f,0,cosf(e->phaseTimer*.5f)*.4f};}if(d<(e->boss?1.8f:1.3f)&&e->attackCooldown<=0){float hit=e->boss?(17.f*RealmThreat(game->world.realm)):(9.f*RealmThreat(game->world.realm));if(e->kind==ELDORIA_ENTITY_BRASA)hit*=1.18f;if(!game->primeUnlocked)hit*=1.0f-ArmorDefense(game->player.armorTier);game->player.health=ClampFloat(game->player.health-hit,0,game->player.maxHealth);PlaySfx(gAudio.hurt);e->attackCooldown = e->boss ? .78f : 1.12f;SetMessage(game,e->boss?"A arena estremece sob o ataque do boss.":"A criatura acertou você.");}e->position=Vector3Add(e->position,Vector3Scale(e->velocity,dt));e->position.y=RealmGround(game->world.realm,e->position.x,e->position.z)+(e->boss?1.1f:.6f);}
}

static void UpdateNPCs(EldoriaGame *game,float dt){for(int i=0;i<game->world.npcCount;i++){EldoriaNPC *n=&game->world.npcs[i];n->routineClock+=dt;n->position.x=n->home.x+cosf(n->routineClock*.35f)*.8f;n->position.z=n->home.z+sinf(n->routineClock*.35f)*.8f;n->position.y=RealmGround(game->world.realm,n->position.x,n->position.z)+.9f;}}

static void UpdatePet(EldoriaGame *game, float dt)
{
    if (!game->pet.active) return;
    game->pet.mood += dt;
    Vector3 forward = { sinf(game->player.yaw), 0.0f, cosf(game->player.yaw) };
    Vector3 desired = Vector3Add(game->player.position, Vector3Scale(forward, -game->pet.followDistance));
    desired.x += cosf(game->pet.mood * 2.2f) * 0.38f;
    desired.z += sinf(game->pet.mood * 2.2f) * 0.38f;
    desired.y = RealmGround(game->world.realm, desired.x, desired.z) + 0.42f;
    Vector3 delta = Vector3Subtract(desired, game->pet.position);
    float distance = Vector3Length(delta);
    if (distance > 0.08f) {
        float speed = distance > 3.5f ? 7.0f : 3.8f;
        game->pet.position = Vector3Add(game->pet.position, Vector3Scale(delta, ClampFloat(speed * dt / distance, 0.0f, 1.0f)));
    }
    game->pet.position.y = RealmGround(game->world.realm, game->pet.position.x, game->pet.position.z) + 0.42f + sinf(game->pet.mood * 4.0f) * 0.06f;
}

static void TryCraft(EldoriaGame *g,int recipe)
{
    EldoriaPlayer *p=&g->player;
    if(recipe==1){if(HasItems(p,ELDORIA_RESOURCE_WOOD,2,ELDORIA_RESOURCE_BERRY,1)){ConsumeItem(p,ELDORIA_RESOURCE_WOOD,2);ConsumeItem(p,ELDORIA_RESOURCE_BERRY,1);AddItem(p,ELDORIA_RESOURCE_POTION,1);PlaySfx(gAudio.craft);SetMessage(g,"Poção de Aurora criada.");}else SetMessage(g,"Receita: 2 madeiras e 1 baga.");}
    if(recipe==2){if(HasItems(p,ELDORIA_RESOURCE_CRYSTAL,3,ELDORIA_RESOURCE_IRON,2)){ConsumeItem(p,ELDORIA_RESOURCE_CRYSTAL,3);ConsumeItem(p,ELDORIA_RESOURCE_IRON,2);p->weaponTier=1;PlaySfx(gAudio.craft);SetMessage(g,"Espada de Aurora forjada. Dano aumentado.");}else SetMessage(g,"Receita: 3 cristais e 2 Ferros Vivos.");}
    if(recipe==3){if(HasItems(p,ELDORIA_RESOURCE_IRON,3,ELDORIA_RESOURCE_CRYSTAL,2)){ConsumeItem(p,ELDORIA_RESOURCE_IRON,3);ConsumeItem(p,ELDORIA_RESOURCE_CRYSTAL,2);AddItem(p,ELDORIA_RESOURCE_PORTAL_KEY,1);PlaySfx(gAudio.craft);SetMessage(g,"Chave de Portal forjada.");}else SetMessage(g,"Receita: 3 Ferros Vivos e 2 cristais.");}
    if(recipe==4){if(HasItems(p,ELDORIA_RESOURCE_COBALT,4,ELDORIA_RESOURCE_RUBYCORE,1)){ConsumeItem(p,ELDORIA_RESOURCE_COBALT,4);ConsumeItem(p,ELDORIA_RESOURCE_RUBYCORE,1);p->weaponTier=2;PlaySfx(gAudio.craft);SetMessage(g,"Lâmina Rubra forjada. Dano elevado.");}else SetMessage(g,"Receita: 4 Cobaltos Astrais e 1 Núcleo Rubro.");}
    if(recipe==5){if(HasItems(p,ELDORIA_RESOURCE_CRYSTAL,4,ELDORIA_RESOURCE_WOOD,2)){ConsumeItem(p,ELDORIA_RESOURCE_CRYSTAL,4);ConsumeItem(p,ELDORIA_RESOURCE_WOOD,2);p->armorTier=1;PlaySfx(gAudio.craft);SetMessage(g,"Couraça de Aurora equipada. Defesa 14%.");}else SetMessage(g,"Receita: 4 cristais e 2 madeiras.");}
    if(recipe==6){if(HasItems(p,ELDORIA_RESOURCE_IRON,5,ELDORIA_RESOURCE_STONE,2)){ConsumeItem(p,ELDORIA_RESOURCE_IRON,5);ConsumeItem(p,ELDORIA_RESOURCE_STONE,2);p->armorTier=2;PlaySfx(gAudio.craft);SetMessage(g,"Armadura de Ferro Vivo equipada. Defesa 28%.");}else SetMessage(g,"Receita: 5 Ferros Vivos e 2 pedras.");}
    if(recipe==7){if(HasItems(p,ELDORIA_RESOURCE_COBALT,6,ELDORIA_RESOURCE_IRON,3)){ConsumeItem(p,ELDORIA_RESOURCE_COBALT,6);ConsumeItem(p,ELDORIA_RESOURCE_IRON,3);p->armorTier=3;PlaySfx(gAudio.craft);SetMessage(g,"Égide de Cobalto equipada. Defesa 42%.");}else SetMessage(g,"Receita: 6 Cobaltos Astrais e 3 Ferros Vivos.");}
    if(recipe==8){if(HasItems(p,ELDORIA_RESOURCE_RUBYCORE,6,ELDORIA_RESOURCE_COBALT,3)){ConsumeItem(p,ELDORIA_RESOURCE_RUBYCORE,6);ConsumeItem(p,ELDORIA_RESOURCE_COBALT,3);p->armorTier=4;PlaySfx(gAudio.craft);SetMessage(g,"Placa da Fornalha Rubra equipada. Defesa 58%.");}else SetMessage(g,"Receita: 6 Núcleos Rubros e 3 Cobaltos Astrais.");}
}

static void UpdateGameCamera(EldoriaGame *g)
{
    EldoriaPlayer *p = &g->player;
    float cosPitch = cosf(p->pitch);
    Vector3 look = { sinf(p->yaw) * cosPitch, sinf(p->pitch), cosf(p->yaw) * cosPitch };
    Vector3 eye = Vector3Add(p->position, (Vector3){0, 0.82f, 0});
    g->camera.position = eye;
    g->camera.target = Vector3Add(eye, Vector3Scale(look, 8.0f));
    g->camera.up = (Vector3){0,1,0};
    g->camera.fovy = 68.0f;
    g->camera.projection = CAMERA_PERSPECTIVE;
}

static void SetDiffuseTexture(Texture2D tex)
{
    if (!gVisualAssetsReady || !IsTextureValid(tex)) return;
    gVisuals.modelBeveledCube.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;
}

static void DrawBeveledCube(Vector3 pos, Vector3 size, Color tint, Texture2D tex)
{
    if (!gVisualAssetsReady) {
        DrawCubeV(pos, size, tint);
        return;
    }
    SetDiffuseTexture(tex);
    DrawModelEx(gVisuals.modelBeveledCube, pos, (Vector3){0,1,0}, 0.0f, size, tint);
}
static void DrawCharacter(Vector3 pos, float yaw, Color bodyColor, int faceIndex, float scale)
{
    // Corpo cúbico
    Vector3 bodyPos = Vector3Add(pos, (Vector3){0, 0.4f*scale, 0});
    if (!gVisualAssetsReady) {
        DrawCubeV(bodyPos, (Vector3){0.6f*scale, 0.8f*scale, 0.4f*scale}, bodyColor);
    } else {
        SetDiffuseTexture(gVisuals.texWood);
        DrawModelEx(gVisuals.modelBeveledCube, bodyPos, (Vector3){0,1,0}, yaw * RAD2DEG, (Vector3){0.6f*scale, 0.8f*scale, 0.4f*scale}, bodyColor);
    }
    
    // Cabeça cúbica com face
    Vector3 headPos = Vector3Add(pos, (Vector3){0, 1.1f*scale, 0});
    if (!gVisualAssetsReady) {
        DrawCubeV(headPos, (Vector3){0.5f*scale, 0.5f*scale, 0.5f*scale}, WHITE);
    } else {
        if (faceIndex >= 0 && faceIndex < 20 && IsTextureValid(gVisuals.texFaces[faceIndex])) {
            SetDiffuseTexture(gVisuals.texFaces[faceIndex]);
        } else {
            SetDiffuseTexture(gVisuals.texWood);
        }
        DrawModelEx(gVisuals.modelBeveledCube, headPos, (Vector3){0,1,0}, yaw * RAD2DEG, (Vector3){0.5f*scale, 0.5f*scale, 0.5f*scale}, WHITE);
    }
    
    // Pernas simples
    if (!gVisualAssetsReady) {
        DrawCubeV(Vector3Add(pos, (Vector3){-0.15f*scale, 0.1f*scale, 0}), (Vector3){0.2f*scale, 0.2f*scale, 0.2f*scale}, bodyColor);
        DrawCubeV(Vector3Add(pos, (Vector3){0.15f*scale, 0.1f*scale, 0}), (Vector3){0.2f*scale, 0.2f*scale, 0.2f*scale}, bodyColor);
    } else {
        SetDiffuseTexture(gVisuals.texStone);
        DrawModelEx(gVisuals.modelBeveledCube, Vector3Add(pos, (Vector3){-0.15f*scale, 0.1f*scale, 0}), (Vector3){0,1,0}, yaw * RAD2DEG, (Vector3){0.2f*scale, 0.2f*scale, 0.2f*scale}, bodyColor);
        DrawModelEx(gVisuals.modelBeveledCube, Vector3Add(pos, (Vector3){0.15f*scale, 0.1f*scale, 0}), (Vector3){0,1,0}, yaw * RAD2DEG, (Vector3){0.2f*scale, 0.2f*scale, 0.2f*scale}, bodyColor);
    }
}

static void DrawRealmTerrain(const EldoriaWorld *w)
{
    Color ground = MOSS;
    Texture2D tex = gVisuals.texGrass;
    if (w->realm == ELDORIA_REALM_CAVES) { ground = (Color){62, 53, 50, 255}; tex = IsTextureValid(gVisuals.texMaterialTiles[2]) ? gVisuals.texMaterialTiles[2] : gVisuals.texStone; }
    else if (w->realm == ELDORIA_REALM_COBALT) { ground = (Color){35, 52, 93, 255}; tex = IsTextureValid(gVisuals.texMaterialTiles[4]) ? gVisuals.texMaterialTiles[4] : gVisuals.texAurora; }
    else if (w->realm == ELDORIA_REALM_FURNACE) { ground = (Color){80, 42, 34, 255}; tex = IsTextureValid(gVisuals.texMaterialTiles[5]) ? gVisuals.texMaterialTiles[5] : gVisuals.texStone; }
    else if (IsTextureValid(gVisuals.texMaterialTiles[0])) tex = gVisuals.texMaterialTiles[0];
    
    // Desenha o plano de chão com textura estilizada
    DrawBeveledCube((Vector3){0, RealmGround(w->realm,0,0)-0.2f, 3}, (Vector3){70, 0.4f, 68}, ground, tex);

    if(w->realm==ELDORIA_REALM_VALLEY){
        for(int x=-28;x<=28;x+=4)for(int z=-24;z<=34;z+=4){
            float y=TerrainHeight(x,z);
            if(fabsf(y)>.25f) DrawBeveledCube((Vector3){x,y-.22f,z}, (Vector3){4,.45f+fabsf(y),4}, DEEP_MOSS, gVisuals.texStone);
        }
    }
    if(w->realm==ELDORIA_REALM_CAVES){
        for(int i=0;i<24;i++){
            float a=i*0.74f;
            Vector3 p={cosf(a)*25,RealmGround(w->realm,0,0)+3,sinf(a)*25};
            DrawBeveledCube(p, (Vector3){1.8f, 2.4f, 1.8f}, (Color){50,45,46,255}, gVisuals.texStone);
        }
    }
}
static void DrawVegetation(const EldoriaWorld *w, bool highQuality)
{
    if(w->realm!=ELDORIA_REALM_VALLEY)return;
    int vegetationCount = highQuality ? 54 : 34;
    for(int i=0;i<vegetationCount;i++){
        int x=(i*17)%51-25,z=(i*23)%59-20;
        if(fabsf((float)x)<3&&z<10)continue;
        float y=TerrainHeight(x,z),s=.85f+Hash01(x,z,99u)*.65f;
        // Árvores "fofas" quadradas
        DrawBeveledCube((Vector3){(float)x, y+1.05f*s, (float)z}, (Vector3){0.4f*s, 2.1f*s, 0.4f*s}, CLAY, gVisuals.texWood);
        DrawBeveledCube((Vector3){(float)x, y+2.25f*s, (float)z}, (Vector3){1.5f*s, 1.5f*s, 1.5f*s}, (Color){63,113,78,255}, gVisuals.texGrass);
    }
}


static void DrawResource(const EldoriaResourceNode *n)
{
    DrawCircle3D(Vector3Add(n->position,(Vector3){0,-.42f,0}), .48f, (Vector3){1,0,0}, 90, ColorAlpha(BLACK, .22f));
    Texture2D tex = gVisuals.texStone;
    Color c = WHITE;
    if(n->kind==ELDORIA_RESOURCE_WOOD) { tex = gVisuals.texWood; }
    else if(n->kind==ELDORIA_RESOURCE_STONE) { tex = gVisuals.texStone; }
    else if(n->kind==ELDORIA_RESOURCE_IRON) { c = LIGHTGRAY; }
    else if(n->kind==ELDORIA_RESOURCE_COBALT) { c = COBALT; tex = gVisuals.texAurora; }
    else if(n->kind==ELDORIA_RESOURCE_RUBYCORE) { c = RUBY; tex = gVisuals.texAurora; }
    else if(n->kind==ELDORIA_RESOURCE_CRYSTAL) { c = AURORA; tex = gVisuals.texAurora; }
    
    DrawBeveledCube(n->position, (Vector3){0.6f, 0.6f, 0.6f}, c, tex);
}

static void DrawEnemy(const EldoriaEnemy *e)
{
    float s = e->boss ? 1.7f : 1.0f;
    Color body = CLAY;
    if(e->kind==ELDORIA_ENTITY_ESCAVADOR||e->kind==ELDORIA_ENTITY_BOSS_ESCAVADOR) body=DEEP_MOSS;
    if(e->kind==ELDORIA_ENTITY_SENTINELA||e->kind==ELDORIA_ENTITY_BOSS_COBALTO) body=COBALT;
    if(e->kind==ELDORIA_ENTITY_BRASA||e->kind==ELDORIA_ENTITY_BOSS_FORNALHA) body=RUBY;
    
    DrawCharacter(e->position, 0.0f, body, 0, s);
}


static void DrawCelebrationScene(const EldoriaGame *g)
{
    Camera3D celebration = {0};
    celebration.position = (Vector3){0, 8.4f, 18.5f};
    celebration.target = (Vector3){0, 1.4f, 4.8f};
    celebration.up = (Vector3){0, 1, 0};
    celebration.fovy = 48.0f;
    celebration.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(celebration);
    DrawPlane((Vector3){0, 0, 5}, (Vector2){30, 24}, (Color){31, 76, 67, 255});
    DrawCircle3D((Vector3){0, .04f, 5}, 4.7f, (Vector3){0,1,0}, 0, ColorAlpha(AMBER, .18f));
    // Pedestal arredondado
    DrawBeveledCube((Vector3){0, .35f, 5}, (Vector3){2.3f, 0.7f, 2.3f}, (Color){42, 48, 48, 255}, gVisuals.texStone);
    DrawBeveledCube((Vector3){0, 1.35f, 5}, (Vector3){0.8f, 0.8f, 0.8f}, ColorAlpha(AMBER, .9f), gVisuals.texAurora);
    for (int i = 0; i < g->world.npcCount; ++i) {
        float a = (float)i * 1.57f + .32f;
        Vector3 p = { cosf(a) * (3.0f + (float)(i % 2) * .65f), .0f, 5.0f + sinf(a) * 2.0f };
        p.y = 0.0f; // DrawCharacter handles height
        Color robe = (i % 2 == 0) ? AURORA : (i == 1 ? COBALT : CLAY);
        float wave = sinf(g->endingTimer * 7.0f + (float)i) * .28f;
        DrawCircle3D(Vector3Add(p,(Vector3){0, 0.04f, 0}), .48f, (Vector3){1,0,0}, 90, ColorAlpha(BLACK, .26f));
        DrawCharacter(p, 0.0f, robe, i % 4, 1.0f);
        // Braços aplaudindo (linhas de energia)
        Vector3 headPos = Vector3Add(p, (Vector3){0, 1.1f, 0});
        DrawLine3D(Vector3Add(headPos,(Vector3){-.28f,-.2f,0}), Vector3Add(headPos,(Vector3){-.72f,0.2f + wave,0.3f}), AMBER);
        DrawLine3D(Vector3Add(headPos,(Vector3){.28f,-.2f,0}), Vector3Add(headPos,(Vector3){.72f,0.2f - wave,0.3f}), AMBER);
    }
    EndMode3D();
}

static void DrawAtmosphere(const EldoriaGame *g)
{
    int w = GetScreenWidth(), h = GetScreenHeight();
    Color accent = RealmAccent(g->world.realm);
    DrawRectangle(0, 0, w, 70, ColorAlpha(accent, .055f));
    DrawRectangle(0, h - 90, w, 90, ColorAlpha(BLACK, .12f));
    DrawCircle(w - 92, 92, 34, ColorAlpha(AMBER, .18f));
    DrawCircle(w - 92, 92, 22, ColorAlpha(IVORY, .12f));
}

static void DrawPet(const EldoriaPet *pet)
{
    if (!pet->active) return;
    Color color = PetColor(pet->kind);
    float bob = sinf(pet->mood * 4.0f) * 0.06f;
    Vector3 p = Vector3Add(pet->position, (Vector3){0, bob, 0});
    DrawCircle3D(Vector3Add(p, (Vector3){0, -0.36f, 0}), 0.48f, (Vector3){1, 0, 0}, 90, ColorAlpha(BLACK, 0.25f));
    if (pet->kind == ELDORIA_PET_AURORA_FOX) {
        DrawBeveledCube(Vector3Add(p, (Vector3){0, .35f, 0}), (Vector3){.72f, .66f, .58f}, color, gVisuals.texAurora);
        DrawBeveledCube(Vector3Add(p, (Vector3){0, .78f, .02f}), (Vector3){.58f, .52f, .52f}, ColorAlpha(IVORY, .95f), gVisuals.texAurora);
        DrawBeveledCube(Vector3Add(p, (Vector3){-.25f, 1.15f, 0}), (Vector3){.18f, .45f, .18f}, color, gVisuals.texAurora);
        DrawBeveledCube(Vector3Add(p, (Vector3){.25f, 1.15f, 0}), (Vector3){.18f, .45f, .18f}, color, gVisuals.texAurora);
        DrawSphere(Vector3Add(p, (Vector3){0, .26f, -.48f}), .25f, ColorAlpha(color, .8f));
    } else if (pet->kind == ELDORIA_PET_MOSS_OWL) {
        DrawSphere(Vector3Add(p, (Vector3){0, .54f, 0}), .52f, color);
        DrawSphere(Vector3Add(p, (Vector3){-.2f, .65f, -.43f}), .18f, IVORY);
        DrawSphere(Vector3Add(p, (Vector3){.2f, .65f, -.43f}), .18f, IVORY);
        DrawSphere(Vector3Add(p, (Vector3){-.2f, .65f, -.57f}), .06f, BLACK);
        DrawSphere(Vector3Add(p, (Vector3){.2f, .65f, -.57f}), .06f, BLACK);
        DrawBeveledCube(Vector3Add(p, (Vector3){0, .18f, -.47f}), (Vector3){.16f, .16f, .16f}, AMBER, gVisuals.texWood);
    } else if (pet->kind == ELDORIA_PET_EMBER_SALAMANDER) {
        DrawBeveledCube(Vector3Add(p, (Vector3){0, .32f, 0}), (Vector3){.95f, .42f, .48f}, color, gVisuals.texAurora);
        for (int i = -2; i <= 2; ++i) DrawBeveledCube(Vector3Add(p, (Vector3){i * .17f, .66f, .04f}), (Vector3){.12f, .32f, .14f}, AMBER, gVisuals.texAurora);
        DrawSphere(Vector3Add(p, (Vector3){0, .38f, -.52f}), .18f, RUBY);
    } else {
        DrawBeveledCube(Vector3Add(p, (Vector3){0, .48f, 0}), (Vector3){.60f, .86f, .54f}, color, gVisuals.texAurora);
        DrawBeveledCube(Vector3Add(p, (Vector3){-.25f, 1.0f, 0}), (Vector3){.18f, .45f, .18f}, color, gVisuals.texAurora);
        DrawBeveledCube(Vector3Add(p, (Vector3){.25f, 1.0f, 0}), (Vector3){.18f, .45f, .18f}, color, gVisuals.texAurora);
        DrawBeveledCube(Vector3Add(p, (Vector3){0, .10f, -.36f}), (Vector3){.48f, .30f, .28f}, IVORY, gVisuals.texStone);
    }
}

static void DrawRemotePlayer(const EldoriaRemotePlayer *remote, const Camera3D *camera)
{
    if (remote == NULL || !remote->active) return;
    DrawCharacter(remote->position, remote->yaw, (Color){173, 121, 222, 255}, 7, 1.0f);
    Vector2 label = GetWorldToScreen(Vector3Add(remote->position, (Vector3){0, 1.55f, 0}), *camera);
    DrawRectangle((int)label.x - 42, (int)label.y - 3, 84, 20, ColorAlpha((Color){12, 21, 28, 255}, .86f));
    DrawText("JOGADOR", (int)label.x - 34, (int)label.y, 10, ColorAlpha(IVORY, .95f));
}

static void DrawWorld(const EldoriaGame *g)
{
    const EldoriaWorld *w=&g->world;DrawRealmTerrain(w);DrawVegetation(w, g->settings.highQuality);
    for(int i=0;i<w->resourceCount;i++)if(w->resources[i].active)DrawResource(&w->resources[i]);
    for(int i=0;i<w->buildingCount;i++){
        const EldoriaBuilding *b=&w->buildings[i];
        if(!b->active)continue;
        if(b->kind==ELDORIA_BUILDING_PORTAL){
            Vector3 core=Vector3Add(b->position,(Vector3){0,.2f,0});
            DrawBeveledCube(b->position, (Vector3){1.6f, 2.7f, 1.6f}, (Color){30,44,49,255}, gVisuals.texStone);
            DrawCircle3D(core,1.05f,(Vector3){1,0,0},90,b->color);
            DrawCircle3D(core,.72f,(Vector3){1,0,0},90,ColorAlpha(b->color,.6f));
            DrawBeveledCube(core, (Vector3){0.8f, 0.8f, 0.8f}, ColorAlpha(b->color,.7f), gVisuals.texAurora);
        } else {
            DrawBeveledCube(b->position, b->size, b->color, gVisuals.texWood);
            if(b->kind==ELDORIA_BUILDING_LANTERN)
                DrawBeveledCube(Vector3Add(b->position,(Vector3){0,1.15f,0}), (Vector3){0.4f, 0.4f, 0.4f}, w->restoredLanterns?AMBER:DARKGRAY, gVisuals.texAurora);
        }
    }
    for(int i=0;i<w->npcCount;i++){
        const EldoriaNPC*n=&w->npcs[i];
        Color npcColor = (i % 2 == 0) ? AURORA : (i == 1 ? COBALT : CLAY);
        DrawCharacter(n->position, 0.0f, npcColor, i % 4, 1.0f);
        Vector2 l=GetWorldToScreen(Vector3Add(n->position,(Vector3){0,1.35f,0}),g->camera);
        DrawText(n->name,(int)l.x-MeasureText(n->name,12)/2,(int)l.y,12,IVORY);
    }
    DrawPet(&g->pet);
    DrawRemotePlayer(EldoriaNetRemote(&gNetwork), &g->camera);
    for(int i=0;i<w->enemyCount;i++)if(w->enemies[i].active)DrawEnemy(&w->enemies[i]);
    for(int i=0;i<ELDORIA_MAX_PARTICLES;i++){const EldoriaParticle*p=&w->particles[i];float bob=sinf(w->timeOfDay*16+p->phase)*.35f;DrawSphere((Vector3){p->position.x,RealmGround(w->realm,p->position.x,p->position.z)+p->position.y+bob,p->position.z},.035f*p->scale,ColorAlpha(RealmAccent(w->realm),.55f));}
}

static void DrawTouchControls(void)
{
#if defined(PLATFORM_ANDROID)
    bool enabled = true;
#else
    bool enabled = GetTouchPointCount() > 0;
#endif
    if (!enabled) return;
    int w = GetScreenWidth(), h = GetScreenHeight();
    Vector2 moveCenter = {96, (float)(h - 112)};
    DrawCircleV(moveCenter, 78, ColorAlpha((Color){7, 17, 23, 255}, 0.48f));
    DrawCircleLines((int)moveCenter.x, (int)moveCenter.y, 78, ColorAlpha(AURORA, 0.58f));
    Vector2 knob = Vector2Add(moveCenter, Vector2Scale(gTouch.move, 48.0f));
    DrawCircleV(knob, 30, ColorAlpha(AURORA, 0.65f));
    DrawText("MOVER", (int)moveCenter.x - 29, (int)moveCenter.y + 88, 11, ColorAlpha(IVORY, 0.78f));

    Rectangle attack = TouchAttackRect(), interact = TouchInteractRect(), build = TouchBuildRect(), inventory = TouchInventoryRect(), crafting = TouchCraftingRect(), pet = TouchPetRect(), sprint = TouchSprintRect();
    DrawRectangleRounded(attack, 0.35f, 12, ColorAlpha(RUBY, 0.76f)); DrawText("ATQ", (int)attack.x + 30, (int)attack.y + 43, 18, IVORY);
    DrawRectangleRounded(interact, 0.35f, 12, ColorAlpha(AMBER, 0.70f)); DrawText("E", (int)interact.x + 28, (int)interact.y + 22, 22, IVORY); DrawText("USAR", (int)interact.x + 12, (int)interact.y + 45, 10, IVORY);
    DrawRectangleRounded(build, 0.35f, 12, ColorAlpha(CLAY, 0.72f)); DrawText("B", (int)build.x + 28, (int)build.y + 20, 22, IVORY); DrawText("BUILD", (int)build.x + 9, (int)build.y + 44, 10, IVORY);
    DrawRectangleRounded(inventory, 0.35f, 12, ColorAlpha(DEEP_MOSS, 0.88f)); DrawText("TAB", (int)inventory.x + 12, (int)inventory.y + 27, 14, IVORY);
    DrawRectangleRounded(crafting, 0.35f, 12, ColorAlpha(AURORA, 0.68f)); DrawText("C", (int)crafting.x + 28, (int)crafting.y + 20, 22, IVORY); DrawText("CRAFT", (int)crafting.x + 8, (int)crafting.y + 44, 10, IVORY);
    DrawRectangleRounded(pet, 0.35f, 12, ColorAlpha(PetColor(ELDORIA_PET_AURORA_FOX), 0.72f)); DrawText("P", (int)pet.x + 28, (int)pet.y + 20, 22, IVORY); DrawText("PET", (int)pet.x + 17, (int)pet.y + 44, 10, IVORY);
    DrawRectangleRounded(sprint, 0.35f, 12, ColorAlpha(MOSS, 0.78f)); DrawText("RUN", (int)sprint.x + 16, (int)sprint.y + 25, 14, IVORY);
    DrawText("ARRASTE O LADO DIREITO PARA OLHAR", w/2 - 132, h - 24, 11, ColorAlpha(IVORY, 0.58f));
}

static void DrawHud(const EldoriaGame *g)
{
    int w=GetScreenWidth(),h=GetScreenHeight();Color accent=RealmAccent(g->world.realm);DrawRectangle(18,18,340,124,(Color){15,27,31,222});DrawText("ELDORIA",32,28,22,IVORY);DrawText(RealmName(g->world.realm),32,54,12,accent);DrawRectangle(32,77,225,12,(Color){54,68,68,255});DrawRectangle(32,77,(int)(225*g->player.health/g->player.maxHealth),12,CLAY);DrawText(TextFormat("Vida %d/%d  •  Nv. %d  •  Lâmina %d",(int)g->player.health,(int)g->player.maxHealth,g->player.level,g->player.weaponTier),32,94,11,IVORY);DrawText(TextFormat("Armadura: %s  •  Defesa %d%%",ArmorName(g->player.armorTier),(int)(ArmorDefense(g->player.armorTier)*100.0f)),32,111,10,ArmorColor(g->player.armorTier)); if(g->primeUnlocked) DrawText("MODO PRIME",32,128,10,AMBER);
    DrawRectangle(w-316,18,298,132,(Color){15,27,31,222});DrawText("CONTROLES",w-296,30,14,AMBER);DrawText("WASD mover • mouse olhar",w-296,53,11,IVORY);DrawText("F / clique atacar • E interagir",w-296,70,11,IVORY);DrawText("B construir • C crafting • TAB inventário",w-296,87,11,IVORY);DrawText("F5 salvar • portais levam às dimensões",w-296,104,11,IVORY);DrawText(TextFormat("Ferro %d  Cobalto %d  Núcleo %d",CountItem(&g->player,ELDORIA_RESOURCE_IRON),CountItem(&g->player,ELDORIA_RESOURCE_COBALT),CountItem(&g->player,ELDORIA_RESOURCE_RUBYCORE)),w-296,127,11,accent); DrawText(TextFormat("Pet: %s  •  P trocar",PetName(g->pet.kind)),w-296,145,10,PetColor(g->pet.kind));
    DrawRectangle(18,h-124,440,102,(Color){15,27,31,222});DrawText("QUESTA PRINCIPAL",32,h-110,12,AMBER);DrawText(g->quest.title,32,h-91,16,IVORY);DrawText(g->quest.description,32,h-69,11,accent);DrawText(g->quest.complete?"Objetivo concluído.":TextFormat("Progresso: %d/%d",g->quest.progress,g->quest.target),32,h-46,12,IVORY);
    if(g->messageTimer>0){int tw=MeasureText(g->message,16);DrawRectangle((w-tw)/2-18,h-66,tw+36,34,(Color){15,27,31,235});DrawText(g->message,(w-tw)/2,h-56,16,IVORY);}DrawText(TextFormat("%s • Seed %llu",ELDORIA_PLATFORM_NAME,(unsigned long long)g->world.seed),18,h-21,10,(Color){210,215,190,190});
}

#if defined(PLATFORM_ANDROID)
static void RequestAndroidShaderPicker(void)
{
    struct android_app *app = GetAndroidApp();
    if (app == NULL || app->activity == NULL || app->activity->vm == NULL) return;
    JNIEnv *env = NULL;
    JavaVM *vm = app->activity->vm;
    if ((*vm)->AttachCurrentThread(vm, &env, NULL) != 0 || env == NULL) return;
    jclass activityClass = (*env)->GetObjectClass(env, app->activity->clazz);
    if (activityClass != NULL) {
        jmethodID openPicker = (*env)->GetMethodID(env, activityClass, "openShaderPicker", "()V");
        if (openPicker != NULL) (*env)->CallVoidMethod(env, app->activity->clazz, openPicker);
        (*env)->DeleteLocalRef(env, activityClass);
    }
    (*vm)->DetachCurrentThread(vm);
}
#endif

static void CyclePet(EldoriaGame *game)
{
    EldoriaPetKind next = (EldoriaPetKind)((game->pet.kind + 1) % ELDORIA_PET_KIND_COUNT);
    SetPet(game, next);
}

static void DrawOptions(const EldoriaGame *g)
{
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangleGradientV(0, 0, w, h, (Color){19, 52, 62, 255}, (Color){9, 18, 27, 255});
    DrawRectangle(90, 62, w - 180, h - 124, ColorAlpha((Color){9, 18, 24, 255}, .94f));
    DrawRectangleLines(90, 62, w - 180, h - 124, ColorAlpha(AURORA, .70f));
    DrawText("OPÇÕES", 132, 98, 32, IVORY);
    DrawText("Preferências locais da versão 0.1.5", 134, 138, 14, AURORA);
    const char *shaderState = g->settings.shaderEnabled ? "ATIVADO" : "DESATIVADO";
    const char *qualityState = g->settings.highQuality ? "ALTO" : "COMPATÍVEL";
    DrawText(TextFormat("[1] Shader realista: %s", shaderState), 142, 214, 22, g->settings.shaderEnabled ? AMBER : IVORY);
    DrawText(TextFormat("[2] Qualidade visual: %s", qualityState), 142, 264, 22, g->settings.highQuality ? AMBER : IVORY);
    DrawText("[3] Importar pacote de shaders (.fs/.glsl)", 142, 314, 22, AURORA);
    DrawText(g->settings.shaderPath[0] ? g->settings.shaderPath : "Nenhum pacote externo selecionado", 142, 350, 13, ColorAlpha(IVORY, .68f));
    DrawText("O pacote é opcional: sem ele o jogo usa a iluminação compatível.", 142, 406, 14, ColorAlpha(IVORY, .76f));
    DrawText("ENTER/ESPAÇO confirma • 1/2 alternam • 3 abre o gerenciador", 142, h - 132, 14, AMBER);
    DrawText("ESC retorna", 142, h - 100, 14, ColorAlpha(IVORY, .70f));
    DrawText(EldoriaNetStatus(&gNetwork), 142, h - 72, 13, ColorAlpha(AURORA, .82f));
}

static void ApplyShaderSettings(EldoriaGame *game)
{
    UnloadWorldShader();
    LoadWorldShader(&game->settings);
    SaveSettings(&game->settings);
    SetMessage(game, game->settings.shaderEnabled && gWorldShaderReady ? "Shader realista ativado." : "Shader compatível em uso.");
}

static void BeginShaderPick(void)
{
    gShaderPickerPending = true;
    gShaderPickerResult = "eldoria_imported_shader.fs";
#if defined(PLATFORM_ANDROID)
    remove("eldoria_imported_shader.fs");
    RequestAndroidShaderPicker();
#else
    FILE *picker = popen("zenity --file-selection --title='Importar pacote de shaders' --file-filter='Shaders | *.fs *.glsl' 2>/dev/null", "r");
    if (picker != NULL) {
        static char selected[256];
        if (fgets(selected, sizeof(selected), picker) != NULL) {
            selected[strcspn(selected, "\r\n")] = '\0';
            if (selected[0] != '\0') gShaderPickerResult = selected;
        }
        pclose(picker);
    }
#endif
}

static void UpdateOptions(EldoriaGame *game)
{
    if (IsKeyPressed(KEY_ESCAPE)) { game->state = ELDORIA_STATE_MENU; return; }
    if (IsKeyPressed(KEY_ONE)) { game->settings.shaderEnabled = !game->settings.shaderEnabled; ApplyShaderSettings(game); }
    if (IsKeyPressed(KEY_TWO)) { game->settings.highQuality = !game->settings.highQuality; SaveSettings(&game->settings); SetMessage(game, game->settings.highQuality ? "Qualidade alta ativada." : "Perfil compatível ativado."); }
    if (IsKeyPressed(KEY_THREE)) BeginShaderPick();
#if defined(PLATFORM_ANDROID)
    if (gShaderPickerPending && FileExists("eldoria_imported_shader.fs")) {
        snprintf(game->settings.shaderPath, sizeof(game->settings.shaderPath), "%s", "eldoria_imported_shader.fs");
        game->settings.shaderEnabled = true;
        gShaderPickerPending = false;
        ApplyShaderSettings(game);
    }
#else
    if (gShaderPickerPending && strcmp(gShaderPickerResult, "eldoria_imported_shader.fs") != 0) {
        snprintf(game->settings.shaderPath, sizeof(game->settings.shaderPath), "%s", gShaderPickerResult);
        game->settings.shaderEnabled = true;
        gShaderPickerPending = false;
        ApplyShaderSettings(game);
    }
#endif
    if (TouchTapped()) {
        Vector2 point = GetTouchPosition(0);
        if (CheckCollisionPointRec(point, (Rectangle){128, 190, 560, 48})) { game->settings.shaderEnabled = !game->settings.shaderEnabled; ApplyShaderSettings(game); }
        else if (CheckCollisionPointRec(point, (Rectangle){128, 248, 560, 48})) { game->settings.highQuality = !game->settings.highQuality; SaveSettings(&game->settings); SetMessage(game, game->settings.highQuality ? "Qualidade alta ativada." : "Perfil compatível ativado."); }
        else if (CheckCollisionPointRec(point, (Rectangle){128, 298, 610, 64})) BeginShaderPick();
        else if (point.y > GetScreenHeight() - 160) game->state = ELDORIA_STATE_MENU;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) game->state = ELDORIA_STATE_MENU;
}

static int NetworkPortValue(void)
{
    long value = strtol(gNetworkPort, NULL, 10);
    if (value < 1024) value = 1024;
    if (value > 65535) value = 65535;
    return (int)value;
}

static void UpdateNetworkText(char *text, size_t capacity, bool allowDots)
{
    int character = GetCharPressed();
    while (character > 0) {
        bool valid = character >= '0' && character <= '9';
        if (allowDots && character == '.') valid = true;
        if (valid && strlen(text) + 1 < capacity) {
            size_t length = strlen(text);
            text[length] = (char)character;
            text[length + 1] = '\0';
        }
        character = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        size_t length = strlen(text);
        if (length > 0) text[length - 1] = '\0';
    }
}

static void BeginHost(EldoriaGame *game)
{
    if (EldoriaNetHost(&gNetwork, NetworkPortValue())) {
        game->state = ELDORIA_STATE_PLAYING;
        game->cursorCaptured = false;
        SetMessage(game, "Servidor criado. Compartilhe seu IP e a porta.");
    } else SetMessage(game, EldoriaNetStatus(&gNetwork));
}

static void BeginJoin(EldoriaGame *game)
{
    if (EldoriaNetJoin(&gNetwork, gNetworkAddress, NetworkPortValue())) {
        game->state = ELDORIA_STATE_PLAYING;
        game->cursorCaptured = false;
        SetMessage(game, "Conectado. O outro jogador aparecerá na cena.");
    } else SetMessage(game, EldoriaNetStatus(&gNetwork));
}

static void DrawMultiplayer(const EldoriaGame *g)
{
    (void)g;
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangleGradientV(0, 0, w, h, (Color){29, 48, 72, 255}, (Color){9, 18, 27, 255});
    DrawRectangle(86, 48, w - 172, h - 96, ColorAlpha((Color){9, 18, 24, 255}, .96f));
    DrawRectangleLines(86, 48, w - 172, h - 96, ColorAlpha(COBALT, .72f));
    DrawText("MULTIPLAYER ONLINE", 126, 84, 30, IVORY);
    DrawText("Conexão direta: o dispositivo do host mantém a sessão.", 128, 124, 14, AURORA);
    DrawText("Endereço IPv4 do host", 128, 190, 14, ColorAlpha(IVORY, .76f));
    DrawRectangle(128, 216, 310, 44, ColorAlpha((Color){30, 46, 62, 255}, .92f));
    DrawRectangleLines(128, 216, 310, 44, gNetworkField == 0 ? AMBER : ColorAlpha(AURORA, .55f));
    DrawText(gNetworkAddress, 144, 229, 20, IVORY);
    DrawText("Porta", 128, 286, 14, ColorAlpha(IVORY, .76f));
    DrawRectangle(128, 312, 170, 44, ColorAlpha((Color){30, 46, 62, 255}, .92f));
    DrawRectangleLines(128, 312, 170, 44, gNetworkField == 1 ? AMBER : ColorAlpha(AURORA, .55f));
    DrawText(gNetworkPort, 144, 325, 20, IVORY);
    DrawRectangleRounded((Rectangle){128, 390, 250, 48}, .2f, 8, ColorAlpha(DEEP_MOSS, .92f));
    DrawText("H  HOSPEDAR", 154, 404, 18, IVORY);
    DrawRectangleRounded((Rectangle){398, 390, 250, 48}, .2f, 8, ColorAlpha(COBALT, .92f));
    DrawText("J  CONECTAR", 424, 404, 18, IVORY);
    DrawText("No host: encaminhe a porta escolhida no roteador ou use a mesma rede.", 128, 486, 13, ColorAlpha(IVORY, .74f));
    DrawText("Em outra rede, o convidado deve usar o IP público do host.", 128, 510, 13, ColorAlpha(IVORY, .74f));
    DrawText(EldoriaNetStatus(&gNetwork), 128, h - 114, 15, AMBER);
    DrawText("TAB alterna campo • ENTER hospeda • J conecta • ESC retorna", 128, h - 78, 14, ColorAlpha(IVORY, .72f));
}

static void UpdateMultiplayer(EldoriaGame *game)
{
    if (IsKeyPressed(KEY_ESCAPE)) { EldoriaNetStop(&gNetwork); game->state = ELDORIA_STATE_MENU; return; }
    if (IsKeyPressed(KEY_TAB)) gNetworkField = (gNetworkField + 1) % 2;
    UpdateNetworkText(gNetworkField == 0 ? gNetworkAddress : gNetworkPort, gNetworkField == 0 ? sizeof(gNetworkAddress) : sizeof(gNetworkPort), gNetworkField == 0);
    if (IsKeyPressed(KEY_H)) BeginHost(game);
    if (IsKeyPressed(KEY_J)) BeginJoin(game);
    if (IsKeyPressed(KEY_ENTER)) BeginHost(game);
    if (TouchTapped()) {
        Vector2 point = GetTouchPosition(0);
        if (CheckCollisionPointRec(point, (Rectangle){128, 216, 310, 44})) gNetworkField = 0;
        else if (CheckCollisionPointRec(point, (Rectangle){128, 312, 170, 44})) gNetworkField = 1;
        else if (CheckCollisionPointRec(point, (Rectangle){128, 390, 250, 48})) BeginHost(game);
        else if (CheckCollisionPointRec(point, (Rectangle){398, 390, 250, 48})) BeginJoin(game);
    }
}

static void DrawCrafting(const EldoriaGame *g)
{
    int w=GetScreenWidth(),h=GetScreenHeight();DrawRectangle(150,90,w-300,h-180,(Color){13,24,29,246});DrawText("BANCADA DE CRAFTING",190,130,28,IVORY);DrawText("Pressione 1–8 para criar • C para fechar",190,168,14,AURORA);DrawText("1  Poção de Aurora       2 madeira + 1 baga",190,220,18,AMBER);DrawText("2  Espada de Aurora      3 cristal + 2 Ferro Vivo",190,260,18,AMBER);DrawText("3  Chave de Portal       3 Ferro Vivo + 2 cristal",190,300,18,AMBER);DrawText("4  Lâmina Rubra          4 Cobalto Astral + 1 Núcleo Rubro",190,340,18,AMBER);DrawText("5  Couraça de Aurora     4 cristal + 2 madeira",190,382,16,ArmorColor(1));DrawText("6  Armadura de Ferro     5 ferro + 2 pedra",190,414,16,ArmorColor(2));DrawText("7  Égide de Cobalto      6 cobalto + 3 ferro",190,446,16,ArmorColor(3));DrawText("8  Placa Rubra           6 núcleo + 3 cobalto",190,478,16,ArmorColor(4));DrawText(TextFormat("Madeira %d  Pedra %d  Cristal %d  Ferro %d  Cobalto %d  Núcleo %d",CountItem(&g->player,ELDORIA_RESOURCE_WOOD),CountItem(&g->player,ELDORIA_RESOURCE_STONE),CountItem(&g->player,ELDORIA_RESOURCE_CRYSTAL),CountItem(&g->player,ELDORIA_RESOURCE_IRON),CountItem(&g->player,ELDORIA_RESOURCE_COBALT),CountItem(&g->player,ELDORIA_RESOURCE_RUBYCORE)),190,h-145,13,IVORY);
}

static void DrawRaycastSplash(float time)
{
    int w = GetScreenWidth(), h = GetScreenHeight();
    float alpha = ClampFloat(time * 1.4f, 0.0f, 1.0f) * ClampFloat((4.2f - time) * 1.4f, 0.0f, 1.0f);
    Color glow = ColorAlpha(AURORA, alpha);
    DrawRectangleGradientV(0, 0, w, h, (Color){10,20,27,255}, (Color){27,53,60,255});
    DrawCircle(w/2, h/2 - 38, 78.0f + sinf(time * 3.0f) * 4.0f, ColorAlpha(AURORA, alpha * 0.14f));
    DrawCircleLines(w/2, h/2 - 38, 46.0f, glow);
    int rw = MeasureText("RAYCAST", 50);
    DrawText("RAYCAST", (w-rw)/2, h/2 + 30, 50, ColorAlpha(IVORY, alpha));
    int cw = MeasureText("CORP", 18);
    DrawText("CORP", (w-cw)/2, h/2 + 88, 18, glow);
    DrawText("APRESENTA", w/2 - MeasureText("APRESENTA", 13)/2, h/2 + 124, 13, ColorAlpha(AMBER, alpha));
}

static const char *EndingSubtitle(float timer)
{
    if (timer < 5.0f) return "Você conseguiu. As três fendas se fecharam, e Eldoria respira novamente.";
    if (timer < 11.0f) return "Quando a noite parecia não ter fim, você guardou uma pequena luz.";
    if (timer < 18.0f) return "Quando o medo chamou seu nome, você respondeu caminhando.";
    if (timer < 25.0f) return "Cada pedra, cada queda, cada recomeço trouxe você até aqui.";
    if (timer < 31.0f) return "Eldoria não termina quando o último inimigo cai.";
    return "Ela continua no brilho que você deixou pelo caminho.";
}

static void DrawEnding(const EldoriaGame *g)
{
    int w = GetScreenWidth(), h = GetScreenHeight();
    float pulse = 0.5f + 0.5f * sinf(g->endingTimer * 2.2f);
    DrawRectangleGradientV(0, 0, w, h, (Color){23, 67, 72, 255}, (Color){8, 17, 25, 255});
    if (g->creditsPage == 0) DrawCelebrationScene(g);
    DrawRectangle(0, 0, w, h, ColorAlpha((Color){5, 13, 19, 255}, g->creditsPage == 0 ? .18f : .54f));
    if (g->creditsPage == 0) {
        DrawCircle(w/2, h/2 - 85, 110.0f + pulse * 12.0f, ColorAlpha(AURORA, 0.10f + pulse * 0.06f));
        DrawCircleLines(w/2, h/2 - 85, 72.0f + pulse * 5.0f, ColorAlpha(AMBER, 0.72f));
        DrawText("ELDORIA FOI RESTAURADA", w/2 - MeasureText("ELDORIA FOI RESTAURADA", 32)/2, 70, 32, IVORY);
        DrawText("Os povos de Eldoria celebram a sua vitória.", w/2 - MeasureText("Os povos de Eldoria celebram a sua vitória.", 20)/2, 118, 20, AURORA);
        DrawText("A música termina antes que a voz da jornada desperte.", w/2 - MeasureText("A música termina antes que a voz da jornada desperte.", 15)/2, h - 82, 15, ColorAlpha(IVORY, .84f));
    } else {
        const char *subtitle = EndingSubtitle(g->endingTimer);
        DrawText("A VOZ DE ELDORIA", w/2 - MeasureText("A VOZ DE ELDORIA", 27)/2, 76, 27, AMBER);
        DrawRectangle(105, h - 164, w - 210, 92, ColorAlpha((Color){7, 17, 23, 255}, .92f));
        DrawRectangleLines(105, h - 164, w - 210, 92, ColorAlpha(AURORA, .68f));
        DrawText(subtitle, w/2 - MeasureText(subtitle, 17)/2, h - 128, 17, IVORY);
        DrawText("A narração continua...", w/2 - MeasureText("A narração continua...", 13)/2, h - 92, 13, AURORA);
    }
}

static void DrawCredits(const EldoriaGame *g)
{
    int w = GetScreenWidth(), h = GetScreenHeight();
    float fade = ClampFloat(g->endingTimer / 2.0f, 0.0f, 1.0f);
    Color text = ColorAlpha(IVORY, fade);
    DrawRectangleGradientV(0, 0, w, h, (Color){9, 18, 27, 255}, (Color){26, 63, 65, 255});
    DrawText("CRÉDITOS DE ELDORIA", w/2 - MeasureText("CRÉDITOS DE ELDORIA", 28)/2, 55, 28, ColorAlpha(AMBER, fade));
    DrawText("Uma jornada criada para quem continuou caminhando.", w/2 - MeasureText("Uma jornada criada para quem continuou caminhando.", 16)/2, 102, 16, ColorAlpha(AURORA, fade));
    DrawText("Raycast Corp", w/2 - MeasureText("Raycast Corp", 22)/2, 160, 22, text);
    DrawText("Eldoria — Ecos das Três Fendas", w/2 - MeasureText("Eldoria — Ecos das Três Fendas", 17)/2, 196, 17, text);
    DrawText("Programação, mundo e sistemas RPG", w/2 - MeasureText("Programação, mundo e sistemas RPG", 15)/2, 248, 15, ColorAlpha(IVORY, fade * .78f));
    DrawText("Áudio original e vozes da jornada", w/2 - MeasureText("Áudio original e vozes da jornada", 15)/2, 274, 15, ColorAlpha(IVORY, fade * .78f));
    DrawText("Para você, que não desistiu.", w/2 - MeasureText("Para você, que não desistiu.", 19)/2, 330, 19, ColorAlpha(AMBER, fade));
    DrawText("Quando a noite parecia não ter fim, você guardou uma pequena luz.", w/2 - MeasureText("Quando a noite parecia não ter fim, você guardou uma pequena luz.", 14)/2, 388, 14, text);
    DrawText("Cada queda virou caminho; cada medo, uma ponte para o amanhã.", w/2 - MeasureText("Cada queda virou caminho; cada medo, uma ponte para o amanhã.", 14)/2, 414, 14, text);
    DrawText("E agora Eldoria respira porque você escolheu permanecer.", w/2 - MeasureText("E agora Eldoria respira porque você escolheu permanecer.", 14)/2, 440, 14, text);
    if (g->endingTimer > 8.0f) {
        DrawText("O MODO PRIME está sendo desbloqueado.", w/2 - MeasureText("O MODO PRIME está sendo desbloqueado.", 20)/2, 505, 20, ColorAlpha(AMBER, fade));
        DrawText("Vida infinita. Itens infinitos. Um mundo sem limites.", w/2 - MeasureText("Vida infinita. Itens infinitos. Um mundo sem limites.", 15)/2, 538, 15, ColorAlpha(AURORA, fade));
    }
    DrawText("ENTER para voltar à aventura", w/2 - MeasureText("ENTER para voltar à aventura", 14)/2, h - 48, 14, ColorAlpha(IVORY, .70f));
}

static void UpdateEnding(EldoriaGame *game, float dt)
{
    game->endingTimer += dt;
    if (game->creditsPage == 0) {
        if (gAudio.ready && IsMusicValid(gAudio.endingTheme)) UpdateMusicStream(gAudio.endingTheme);
        /* Dezesseis segundos de música e aplausos; só então entra a narração. */
        if (game->endingTimer >= 16.0f || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (gAudio.ready && IsMusicValid(gAudio.endingTheme)) StopMusicStream(gAudio.endingTheme);
            game->creditsPage = 1;
            game->endingTimer = 0.0f;
            if (gAudio.ready && IsSoundValid(gAudio.endingVoice)) PlaySound(gAudio.endingVoice);
        }
    } else {
        /* A legenda acompanha blocos de tempo da fala gerada para a cena final. */
        if (game->endingTimer >= 36.0f || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (gAudio.ready && IsSoundValid(gAudio.endingVoice)) StopSound(gAudio.endingVoice);
            if (gAudio.ready && IsMusicValid(gAudio.endingTheme)) PlayMusicStream(gAudio.endingTheme);
            game->state = ELDORIA_STATE_CREDITS;
            game->endingTimer = 0.0f;
        }
    }
}

static void UpdateCredits(EldoriaGame *game, float dt)
{
    game->endingTimer += dt;
    if (gAudio.ready && IsMusicValid(gAudio.endingTheme)) UpdateMusicStream(gAudio.endingTheme);
    if (game->endingTimer >= 8.0f && !game->primeUnlocked) UnlockPrimeMode(game);
    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) && game->endingTimer >= 3.0f) {
        if (!game->primeUnlocked) UnlockPrimeMode(game);
        game->state = ELDORIA_STATE_PLAYING;
        game->endingTimer = 0.0f;
        UpdateCursorCapture(game);
        SetMessage(game, "Modo Prime desbloqueado. Eldoria agora é sua para sempre.");
    }
}

static void DrawMainMenu(const EldoriaGame *g)
{
    static const char *items[] = { "INICIAR JORNADA", "CONTINUAR", "MULTIPLAYER ONLINE", "OPÇÕES", "CONTROLES", "SAIR" };
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangleGradientV(0, 0, w, h, (Color){24,64,70,255}, (Color){10,21,29,255});
    for (int i = 0; i < 20; ++i) DrawCircle((i * 173) % w, 55 + (i * 97) % h, 1.5f, ColorAlpha(AURORA, 0.22f));
    DrawText("ELDORIA", 86, 106, 64, IVORY);
    DrawText("ECOS DAS TRÊS FENDAS  •  v0.1.5", 91, 178, 18, AURORA);
    DrawText("RAYCAST CORP", 91, 210, 14, AMBER);
    for (int i = 0; i < 6; ++i) {
        int y = 270 + i * 48;
        bool selected = g->menuSelection == i;
        if (selected) DrawRectangle(84, y - 8, 470, 42, ColorAlpha(AURORA, 0.22f));
        DrawText(selected ? ">" : " ", 96, y, 24, selected ? AMBER : IVORY);
        DrawText(items[i], 130, y, 24, selected ? AMBER : IVORY);
    }
    DrawText("Setas ou W/S para selecionar • ENTER para confirmar", 90, h - 82, 14, ColorAlpha(IVORY, 0.78f));
    DrawText("primeira pessoa: WASD move, mouse olha", 90, h - 55, 12, ColorAlpha(AURORA, 0.62f));
}

static void UpdateMainMenu(EldoriaGame *g)
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) g->menuSelection = (g->menuSelection + 1) % 6;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) g->menuSelection = (g->menuSelection + 5) % 6;
    if (TouchTapped()) {
        Vector2 point = GetTouchPosition(0);
        if (point.x >= 70 && point.x <= 560 && point.y >= 238 && point.y <= 570) {
            g->menuSelection = (int)((point.y - 238) / 48.0f);
            if (g->menuSelection < 0) g->menuSelection = 0;
            if (g->menuSelection > 5) g->menuSelection = 5;
        }
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || TouchTapped()) {
        if (g->menuSelection == 0) { InitGame(g); g->state = ELDORIA_STATE_PLAYING; }
        else if (g->menuSelection == 1) { InitGame(g); if (!LoadGame(g)) SetMessage(g, "Nenhum jogo salvo foi encontrado."); UnloadWorldShader(); LoadWorldShader(&g->settings); g->state = ELDORIA_STATE_PLAYING; }
        else if (g->menuSelection == 2) { g->state = ELDORIA_STATE_MULTIPLAYER; gNetworkField = 0; }
        else if (g->menuSelection == 3) g->state = ELDORIA_STATE_OPTIONS;
        else if (g->menuSelection == 4) SetMessage(g, "Primeira pessoa: WASD move, mouse olha, F ataca e E interage.");
        else g->input.quitPressed = true;
    }
}

static bool SaveGame(const EldoriaGame *g)
{
    FILE *file = fopen(SAVE_PATH, "wb");
    if (file == NULL) return false;
    uint32_t magic = ELDORIA_SAVE_MAGIC;
    uint32_t version = ELDORIA_SAVE_VERSION;
    bool ok = fwrite(&magic, sizeof magic, 1, file) == 1 && fwrite(&version, sizeof version, 1, file) == 1 && fwrite(g, sizeof *g, 1, file) == 1;
    fclose(file);
    return ok;
}

static bool LoadGame(EldoriaGame *g)
{
    FILE *file = fopen(SAVE_PATH, "rb");
    if (file == NULL) return false;
    uint32_t magic = 0, version = 0;
    bool ok = fread(&magic, sizeof magic, 1, file) == 1 && fread(&version, sizeof version, 1, file) == 1 && magic == ELDORIA_SAVE_MAGIC;
    if (ok && version == ELDORIA_SAVE_VERSION) {
        ok = fread(g, sizeof *g, 1, file) == 1;
    } else if (ok && version == 5) {
        /* O prefixo do save 0.1.4 termina antes dos campos de pets/opções. */
        memset(g, 0, sizeof(*g));
        ok = fread(g, offsetof(EldoriaGame, pet), 1, file) == 1;
        if (ok) {
            SetPet(g, ELDORIA_PET_AURORA_FOX);
            LoadSettings(&g->settings);
        }
    } else {
        ok = false;
    }
    fclose(file);
    if (ok) SetMessage(g, version == 5 ? "Jogo 0.1.4 migrado para 0.1.5." : "Jogo carregado.");
    return ok;
}

int main(void)
{
#if defined(PLATFORM_ANDROID)
    // No Android, o diretório de trabalho inicial pode ser restrito.
    // Mudamos para o internalDataPath para permitir a criação do arquivo de save.
    struct android_app *app = GetAndroidApp();
    if (app != NULL && app->activity != NULL && app->activity->internalDataPath != NULL) {
        chdir(app->activity->internalDataPath);
        TraceLog(LOG_INFO, "ELDORIA: CWD alterado para %s", app->activity->internalDataPath);
    }
#endif
    TraceLog(LOG_INFO, "ELDORIA: Iniciando main...");
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    TraceLog(LOG_INFO, "ELDORIA: Inicializando janela %dx%d...", ELDORIA_DEFAULT_WIDTH, ELDORIA_DEFAULT_HEIGHT);
    InitWindow(ELDORIA_DEFAULT_WIDTH, ELDORIA_DEFAULT_HEIGHT, ELDORIA_TITLE);
    TraceLog(LOG_INFO, "ELDORIA: Janela inicializada.");
    SetTargetFPS(ELDORIA_TARGET_FPS);
    TraceLog(LOG_INFO, "ELDORIA: Carregando assets de áudio...");
    LoadAudioAssets();
    TraceLog(LOG_INFO, "ELDORIA: Carregando assets visuais...");
    LoadVisualAssets();
    TraceLog(LOG_INFO, "ELDORIA: Assets carregados.");
    EldoriaGame g;
    InitGame(&g);
    EldoriaNetInit(&gNetwork);
    LoadWorldShader(&g.settings);
    g.state = ELDORIA_STATE_SPLASH;
    g.introTimer = 0.0f;
    float acc = 0.0f;

    while (!WindowShouldClose() && !g.input.quitPressed) {
        float dt = ClampFloat(GetFrameTime(), 0.0f, 0.1f);
        UpdateTouchFrame();
        BeginDrawing();
        ClearBackground((Color){10, 20, 27, 255});

        if (g.state == ELDORIA_STATE_SPLASH) {
            EnableCursor();
            g.introTimer += dt;
            DrawRaycastSplash(g.introTimer);
            if (g.introTimer >= 4.2f || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || TouchTapped()) g.state = ELDORIA_STATE_MENU;
        } else if (g.state == ELDORIA_STATE_MENU) {
            EnableCursor();
            UpdateMainMenu(&g);
            DrawMainMenu(&g);
        } else if (g.state == ELDORIA_STATE_ENDING) {
            EnableCursor();
            UpdateEnding(&g, dt);
            DrawEnding(&g);
        } else if (g.state == ELDORIA_STATE_CREDITS) {
            EnableCursor();
            UpdateCredits(&g, dt);
            DrawCredits(&g);
        } else if (g.state == ELDORIA_STATE_OPTIONS) {
            EnableCursor();
            UpdateOptions(&g);
            DrawOptions(&g);
        } else if (g.state == ELDORIA_STATE_MULTIPLAYER) {
            EnableCursor();
            UpdateMultiplayer(&g);
            DrawMultiplayer(&g);
        } else {
            UpdateCursorCapture(&g);
            acc += dt;
            g.input = ReadInput(g.cursorCaptured && !g.discardNextMouseDelta);
            g.discardNextMouseDelta = false;
            if (g.state == ELDORIA_STATE_PLAYING) UpdateFirstPersonLook(&g);
            if (g.input.inventoryPressed) g.state = g.state == ELDORIA_STATE_INVENTORY ? ELDORIA_STATE_PLAYING : ELDORIA_STATE_INVENTORY;
            if (g.input.craftingPressed) g.state = g.state == ELDORIA_STATE_CRAFTING ? ELDORIA_STATE_PLAYING : ELDORIA_STATE_CRAFTING;
            if (g.input.petPressed && g.state == ELDORIA_STATE_PLAYING) CyclePet(&g);
            if (g.input.savePressed) SetMessage(&g, SaveGame(&g) ? "Jogo salvo." : "Não foi possível salvar.");
            if (g.state == ELDORIA_STATE_CRAFTING) {
                if (g.input.craftOnePressed) TryCraft(&g, 1);
                if (g.input.craftTwoPressed) TryCraft(&g, 2);
                if (g.input.craftThreePressed) TryCraft(&g, 3);
                if (g.input.craftFourPressed) TryCraft(&g, 4);
                if (g.input.craftFivePressed) TryCraft(&g, 5);
                if (g.input.craftSixPressed) TryCraft(&g, 6);
                if (g.input.craftSevenPressed) TryCraft(&g, 7);
                if (g.input.craftEightPressed) TryCraft(&g, 8);
            }
            if (g.input.buildPressed && g.state == ELDORIA_STATE_PLAYING) TryBuild(&g);
            while (acc >= ELDORIA_FIXED_STEP) {
                if (g.state == ELDORIA_STATE_PLAYING) {
                    UpdatePlayer(&g, ELDORIA_FIXED_STEP); UpdateEnemies(&g, ELDORIA_FIXED_STEP); UpdateNPCs(&g, ELDORIA_FIXED_STEP); UpdatePet(&g, ELDORIA_FIXED_STEP);
                    EldoriaNetUpdate(&gNetwork, ELDORIA_FIXED_STEP, g.player.position, g.player.yaw, g.player.health);
                    if (g.input.interactPressed) TryInteract(&g);
                    if (g.input.attackPressed) TryAttack(&g);
                    g.world.timeOfDay = fmodf(g.world.timeOfDay + ELDORIA_FIXED_STEP*.002f, 1.0f);
                    if (g.messageTimer > 0) g.messageTimer -= ELDORIA_FIXED_STEP;
                    if (g.player.health <= 0) {
                        EldoriaRealm fallenRealm = g.world.realm;
                        g.player.health = g.player.maxHealth * 0.72f;
                        g.player.stamina = g.player.maxStamina;
                        EnterRealm(&g, ELDORIA_REALM_VALLEY);
                        if (fallenRealm == ELDORIA_REALM_FURNACE) SetMessage(&g, "A Placa Rubra absorveu o último impacto. Você despertou no Vale das Lanternas.");
                        else SetMessage(&g, "Maela o encontrou antes que a fenda o consumisse.");
                    }
                }
                acc -= ELDORIA_FIXED_STEP;
            }
            UpdateGameCamera(&g);
            Color skyA = g.world.realm == ELDORIA_REALM_FURNACE ? (Color){87,31,29,255} : (g.world.realm == ELDORIA_REALM_COBALT ? (Color){38,62,115,255} : (Color){116,169,175,255});
            DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), skyA, (Color){15,28,35,255});
            if (gWorldShaderReady) {
                gShaderTime += dt;
                int viewLoc = GetShaderLocation(gWorldShader, "viewPos");
                int timeLoc = GetShaderLocation(gWorldShader, "time");
                float viewPos[3] = { g.camera.position.x, g.camera.position.y, g.camera.position.z };
                SetShaderValue(gWorldShader, viewLoc, viewPos, SHADER_UNIFORM_VEC3);
                SetShaderValue(gWorldShader, timeLoc, &gShaderTime, SHADER_UNIFORM_FLOAT);
                BeginShaderMode(gWorldShader);
                BeginMode3D(g.camera); DrawWorld(&g); EndMode3D();
                EndShaderMode();
            } else {
                BeginMode3D(g.camera); DrawWorld(&g); EndMode3D();
            }
            DrawAtmosphere(&g); DrawHud(&g); DrawTouchControls();

            if (g.state == ELDORIA_STATE_INVENTORY) { DrawRectangle(180,120,GetScreenWidth()-360,GetScreenHeight()-240,(Color){15,27,31,245}); DrawText("INVENTÁRIO",220,160,28,IVORY); DrawText(TextFormat("Madeira: %d\nPedra: %d\nCristal: %d\nFerro Vivo: %d\nCobalto Astral: %d\nNúcleo Rubro: %d\nPoções: %d",CountItem(&g.player,ELDORIA_RESOURCE_WOOD),CountItem(&g.player,ELDORIA_RESOURCE_STONE),CountItem(&g.player,ELDORIA_RESOURCE_CRYSTAL),CountItem(&g.player,ELDORIA_RESOURCE_IRON),CountItem(&g.player,ELDORIA_RESOURCE_COBALT),CountItem(&g.player,ELDORIA_RESOURCE_RUBYCORE),CountItem(&g.player,ELDORIA_RESOURCE_POTION)),220,214,18,IVORY); DrawText("TAB para retornar",220,GetScreenHeight()-180,14,AMBER); }
            if (g.state == ELDORIA_STATE_CRAFTING) DrawCrafting(&g);
        }
        EndDrawing();
    }
    EnableCursor();
    SaveGame(&g);
    SaveSettings(&g.settings);
    EldoriaNetStop(&gNetwork);
    UnloadWorldShader();
    UnloadAudioAssets();
    UnloadVisualAssets();
    CloseWindow();
    return 0;
}
