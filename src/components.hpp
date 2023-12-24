#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"
#include <chrono>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>


/**
 * @brief Fishing system: When you throw your fishing rod, it lands on one tile.
 * The probability of catching a fish is determined by the tile you land on.
 * how long you need to wait for each species fish to bite will be a continuous random variable
 * refs in structs: https://stackoverflow.com/questions/14789058/initialize-struct-contain-references-to-structs
 */

struct LakeId
{
    int id = 0;
};

struct Gift
{
    int type = 0;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(type));
    }
};

struct Sellable
{
    float price = 0.f;
};

struct InventoryItem
{
};

struct Fish // entity that has species, fishable components
{
    int species_id = 0;
    int lake_id = 0;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(species_id), CEREAL_NVP(lake_id));
    }
};

struct FishingResult
{
    Fish fish;
    float ms_until_catch;
};

struct FishShadow // for keeping track of fish shadow entities
{
};


struct ShinySpot // for keeping track of shiny spot entities
{
};

struct FishingRod // different fishing rods have different catch probabilities
{
    float probability;
    std::string name;
};


struct Enemy
{
    FishSpecies species;
    std::vector<Skill> skills;
    int actionIndex = -1;
    std::vector<CurrEffect> currEffects;
};

struct Tile
{
    // Lake &lake;
};
struct LandTile : Tile
{
};
struct WaterTile : Tile
{
    float catch_likeliness = 1.f; // This a factor relative to lake probability, different areas in lake can have slightly different probabilities
};

// For party member stats
struct Stats
{
    int attack;
    int healing_bonus;
    int speed;
    float critical_rate = 0.1f;
    float critical_value = 1.5f; //default crit values, 10% chance to do 1.5x damage
};

// For party member friendship level
struct Friendship
{
    int friendship_value = 0;
};

using namespace std::chrono;
struct FishingLog
{
    int64_t timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int species_id = 0;
    int lake_id = 0;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(timestamp), CEREAL_NVP(species_id), CEREAL_NVP(lake_id));
    }
};

// Player component
// add flags for completing certain things in the game
// NOTE: num_fish_caught DOESN'T COUNT BATTLE FISH (ONLY MINIGAME ONES)
struct Player
{
    int num_fish_caught_lake1 = 0;
    int num_fish_caught_lake2 = 0;
    bool shiny_tutorial = false;
    bool ally1_recruited = false;
    bool ally2_recruited = false;
    bool battle_tutorial_complete = false;
    bool basic_tutorial_complete = false;
    bool fishing_tutorial_complete = false;
    bool lake1_boss_defeated = false;
    bool lake2_entered = false;
};

struct Wallet
{
    int gold = 0;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(gold));
    }

};

// Turtles have a hard shell
struct HardShell
{
};

// Fish and Salmon have a soft shell
struct SoftShell
{
};

// All data relevant to the shape and motion of entities
struct Motion
{
    vec2 position = {0.f, 0.f};
    float angle = 0.f;
    vec2 velocity = {0.f, 0.f};
    vec2 scale = {10.f, 10.f};

    // have to split the vec2 into floats because the cereal library is extremely
    // allergic to any types besides primitives.
    float posX = position.x;
    float posY = position.y;
    float scaleX = scale.x;
    float scaleY = scale.y;
    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(posX), CEREAL_NVP(posY), CEREAL_NVP(angle), CEREAL_NVP(scaleX), CEREAL_NVP(scaleY));
    }
};

/**
 * fishing lures + baits
 * lures = artificial bait -> last longer (I think we should stick with only lures or only baits, too complex to have both)
 * baits are one use only
 */
// struct ShopItem {
// 	float durability = 0.f;
// 	float angle = 0.f;
// 	vec2 velocity = { 0.f, 0.f };
// 	vec2 scale = { 10.f, 10.f };
// };

// default max upgrade is 10, remember to increase when event happens (e.g. kill first boss or discover new lake)
struct Durability
{
    float max = 0.f;
    float current = 0.f;
    float num_upgrades = 0;
    float max_upgrade = 20;
};

struct Attack
{
    int damage = 0;
    float num_upgrades = 0;
    float max_upgrade = 20;
};

struct Defense
{
    int value = 0;
};

struct Buff
{
    float charm = 0.f;	  // increase chance of hooking fish
    float strength = 0.f; // increase chance of reeling in fish after hooked

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(charm), CEREAL_NVP(strength));
    }
};

struct Lure // has durability + Buff
{
    std::string name;
    int price = 0; // price of 10 lures
    std::string description;
    int numOwned = 0;
    int texture_id = 0;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(price), CEREAL_NVP(description), CEREAL_NVP(numOwned), CEREAL_NVP(texture_id));
    }
};

struct EquipLure
{
    int lureIndex = -1;
};

struct PendingCaughtFish
{
    int species_id = 0;
};

// Stucture to store collision information
struct Collision
{
    // Note, the first object is stored in the ECS container.entities
    Entity other_entity; // the second object involved in the collision
    Collision(Entity &other_entity) { this->other_entity = other_entity; };
};

struct CatchingBar {
	float filled = 0.3f;
};

// Data structure for toggling debug mode
struct Debug
{
    bool in_debug_mode = 0;
    bool in_freeze_mode = 0;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
    float screen_darken_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
    // Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
    float timer_ms = 100.f;
};

// A timer that will be associated to dying salmon
struct RemoveEntityTimer
{
    float timer_ms = 30000.f;
};

// A timer that will be associated to how long it takes to catch the fish
struct FishingTimer
{
    float timer_ms = 3000.f;
    Fish fish;
};

// A timer that will be associated to how long it takes for next shadow movement
struct ShadowTimer
{
    float timer_ms = 5000.f;
    Fish fish;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex
{
    vec3 position;
    vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
    vec3 position;
    vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
    static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex> &out_vertices, std::vector<uint16_t> &out_vertex_indices, vec2 &out_size);
    vec2 original_size = {1, 1};
    std::vector<ColoredVertex> vertices;
    std::vector<uint16_t> vertex_indices;
};

// Sprite data structure for number of frames, start frame, end frame, current frame, and rows and columns of sheet
struct Sprite
{
    int num_frames = 1;
    int end_frame = 0;
    int start_frame = 0;
    int current_frame = 0;
    int rows;
    int columns;
    double frame_duration = 0.1;
    bool loop = false;
    float last_time = 0.0f;
};

// Dialogue component
struct Dialogue
{
    int current_line = 0;
    std::string cutscene_id;
    GAME_STATE_ID next_game_state = GAME_STATE_ID::WORLD;
};

struct Boss
{
};

// Light up
struct LightUp {
};

enum class ANIMATION_FRAMES
{
    CAST = 3,
    RAISE = 4,
    FISHING = 7,
    REELING = 12
};
/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID
{
    FISH = 0,
    FISH_SHADOW = FISH + 1,
    LAKE = FISH_SHADOW + 1, // For lake texture
    SHORE = LAKE + 1, // For shore texture
    PLAYER_LEFT = SHORE + 1,
    PLAYER_RIGHT = PLAYER_LEFT + 1,
    PLAYER_UP = PLAYER_RIGHT + 1,
    PLAYER_DOWN = PLAYER_UP + 1,
    TUTORIAL_BG = PLAYER_DOWN + 1, // For tutorial background texture
    BACKGROUND = TUTORIAL_BG + 1, // For lake/land texture
    MAINUI = BACKGROUND + 1,
    MENU_BG = MAINUI + 1,
    CHINOOK = MENU_BG + 1,
    ITEM_CELL = CHINOOK + 1,
    CHINOOK_SHADOW = ITEM_CELL + 1,
    SHOPKEEPER = CHINOOK_SHADOW + 1,
    CATCHING_BAR = SHOPKEEPER + 1,
    BATTLE_UI_MAIN = CATCHING_BAR + 1,
    MC_PORTRAIT_1 = BATTLE_UI_MAIN + 1,
    MC_PORTRAIT_2 = MC_PORTRAIT_1 + 1,
    ENEMY = MC_PORTRAIT_2 + 1,
    TUTORIAL_BASIC = ENEMY + 1,
    TUTORIAL_FISH = TUTORIAL_BASIC + 1,
    TUTORIAL_BATTLE = TUTORIAL_FISH + 1,
    PARTY_MEMBER_BASIC = TUTORIAL_BATTLE + 1,
    PARTY_MEMBER_DETAIL = PARTY_MEMBER_BASIC + 1,
    ROD_IDLE = PARTY_MEMBER_DETAIL + 1,
    PLAYER_LEFT_SHEET = ROD_IDLE + 1,
    PLAYER_RIGHT_SHEET = PLAYER_LEFT_SHEET + 1,
    PLAYER_UP_SHEET = PLAYER_RIGHT_SHEET + 1,
    PLAYER_DOWN_SHEET = PLAYER_UP_SHEET + 1,
    ROD_REEL = PLAYER_DOWN_SHEET + 1,
    ON_HIT_EFFECT = ROD_REEL + 1,
    EXCLAMATION = ON_HIT_EFFECT + 1,
    SETTINGS = EXCLAMATION + 1,
    SAVE = SETTINGS + 1,
    SAVE_DONE = SAVE + 1,
    LOAD = SAVE_DONE + 1,
    DELETE_SAVE = LOAD + 1,
    DELETE_SAVE_DONE = DELETE_SAVE + 1,
    LOAD_FAIL = DELETE_SAVE_DONE + 1,
    ROD_RELEASE = LOAD_FAIL + 1,
    ROD_ON_HIT = ROD_RELEASE + 1,
    HEAL_EFFECT = ROD_ON_HIT + 1,
    ATK_BUFF_EFFECT = HEAL_EFFECT + 1,
    ROD_DEFEATED = ATK_BUFF_EFFECT + 1,
    MC_PORTRAIT = ROD_DEFEATED + 1,
    SPD_BUFF_EFFECT = MC_PORTRAIT + 1,
    TRANSITION = SPD_BUFF_EFFECT + 1,
    ALLY1_PORTRAIT = TRANSITION + 1,
    MC_DIALOGUE = ALLY1_PORTRAIT + 1,
    ALLY1_DIALOGUE = MC_DIALOGUE + 1,
    ALLY1 = ALLY1_DIALOGUE + 1,
    ARROW = ALLY1 + 1,
    DEF_BUFF_EFFECT = ARROW + 1,
    SHINY_SPOT = DEF_BUFF_EFFECT + 1,
    PARTICLE = SHINY_SPOT + 1,
    DOOM = PARTICLE + 1,
    CURSE = DOOM + 1,
    ALLY2_PORTRAIT = CURSE + 1,
    ALLY2 = ALLY2_PORTRAIT + 1,
    ITEM_CELL_SELECTED = ALLY2 + 1,
    TUTORIAL_BATTLE1 = ITEM_CELL_SELECTED + 1,
    TUTORIAL_BATTLE2 = TUTORIAL_BATTLE1 + 1,
    TUTORIAL_BATTLE3 = TUTORIAL_BATTLE2 + 1,
    TUTORIAL_BATTLE4 = TUTORIAL_BATTLE3 + 1,
    TUTORIAL_BATTLE5 = TUTORIAL_BATTLE4 + 1,
    TUTORIAL_BATTLE6 = TUTORIAL_BATTLE5 + 1,
    TUTORIAL_BATTLE7 = TUTORIAL_BATTLE6 + 1,
    SLEEP_NARWHAL = TUTORIAL_BATTLE7 + 1,
    TURTLE = SLEEP_NARWHAL + 1,
    WALRUS = TURTLE + 1,
    TITLE = WALRUS + 1,
    ALLY1_RECRUIT = TITLE + 1,
    BACKGROUND_2 = ALLY1_RECRUIT + 1,
    TELEPORT_1 = BACKGROUND_2 + 1,
    TELEPORT_2 = TELEPORT_1 + 1,
    LEGEND_FISH_BG = TELEPORT_2 + 1,
    MC_INTRO = LEGEND_FISH_BG + 1,
    SHINY_TUTORIAL = MC_INTRO + 1,
    ALLY2_DIALOGUE = SHINY_TUTORIAL + 1,
    ALLY2_RECRUIT = ALLY2_DIALOGUE + 1,
    BOSS_SHADOW = ALLY2_RECRUIT + 1,
    BOSS_EXECUTION = BOSS_SHADOW + 1,
    INKWHALE = BOSS_EXECUTION + 1,
    LURE_1 = INKWHALE + 1,
    LURE_2 = LURE_1 + 1,
    BOSS_POPUP = LURE_2 + 1,
    BOSS_DEFEATED_POPUP = BOSS_POPUP + 1,
    TEXTURE_COUNT = BOSS_DEFEATED_POPUP + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class FISH_TEXTURE_ASSET_ID
{
    CRACKER = 0,
    CARP = CRACKER + 1,
    CHINOOK = CARP + 1,
    CUCUMBER = CHINOOK + 1,
    HAMMER = CUCUMBER + 1,
    SHARPIE = HAMMER + 1,
    SWEET_POTATO = SHARPIE + 1,
    TEAL = SWEET_POTATO + 1,
    UPWARDS = TEAL + 1,
    THE_FISH = UPWARDS + 1,
    ANGEL = THE_FISH + 1,
    BANDAGE = ANGEL + 1,
    CHEESE = BANDAGE + 1,
    TODDLER = CHEESE + 1,
    JACK = TODDLER + 1,
    SQUID = JACK + 1,
    INKWHALE = SQUID + 1,
    SLEEP_NARWHAL = INKWHALE + 1,
    TURTLE = SLEEP_NARWHAL + 1,
    WALRUS = TURTLE + 1,
    WRINKLY_CORAL = WALRUS + 1,
    YAM_ROLL = WRINKLY_CORAL + 1,
    SEAL_FISH = YAM_ROLL + 1,
    TACO_RAY = SEAL_FISH + 1,
    BLUE_RAY = TACO_RAY + 1,
    BABY_FLOCK = BLUE_RAY + 1,
    CRAB = BABY_FLOCK + 1,
    FISH_TEXTURE_COUNT = CRAB + 1
};

const int fish_texture_count = (int)FISH_TEXTURE_ASSET_ID::FISH_TEXTURE_COUNT;

enum class EFFECT_ASSET_ID
{
	COLOURED = 0,
	PEBBLE = COLOURED + 1,
	PLAYER = PEBBLE + 1,
	TEXTURED = PLAYER + 1,
	WATER = TEXTURED + 1,
	CATCHING_BAR = WATER + 1,
    EFFECT = CATCHING_BAR + 1,
    PARTICLE = EFFECT + 1,
    SALMON = PARTICLE + 1,
	EFFECT_COUNT = SALMON + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
    PLAYER_LEFT = 0,
    PLAYER_RIGHT = PLAYER_LEFT + 1,
    PLAYER_UP = PLAYER_RIGHT + 1,
    PLAYER_DOWN = PLAYER_UP + 1,
    SPRITE = PLAYER_DOWN + 1,
    PEBBLE = SPRITE + 1,
    DEBUG_LINE = PEBBLE + 1,
    SCREEN_TRIANGLE = DEBUG_LINE + 1,
    LAKE = SCREEN_TRIANGLE + 1,
    SHORE = LAKE + 1,
    TUTORIAL_BG = SHORE + 1,
    BACKGROUND = TUTORIAL_BG + 1,
    CATCHING_BAR = BACKGROUND + 1,
	SPRITE_SHEET = CATCHING_BAR + 1,
    SALMON = SPRITE_SHEET + 1,
    GEOMETRY_COUNT = SALMON + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest
{
    TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
    EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
    GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
    bool is_visible = true;
    FISH_TEXTURE_ASSET_ID fish_texture = FISH_TEXTURE_ASSET_ID::FISH_TEXTURE_COUNT;
};

struct RenderRequestsNonEntity
{
    std::string id;
    GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
    TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
    EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
    vec4 color = vec4(1.f);
    vec2 pos;
    float angle;
    vec2 scale;
};

struct PartyMember
{
    std::string name;
    std::string description;
    Stats stats;
    std::vector <Skill> skills;
    int actionIndex = -1;
    std::vector<CurrEffect> currEffects;
    TEXTURE_ASSET_ID texture_id;
    std::vector<float> followUpDmg;
    TEXTURE_ASSET_ID menu_texture_id;
};

const std::vector<TEXTURE_ASSET_ID> battle_tutorials = {TEXTURE_ASSET_ID::TUTORIAL_BATTLE1, TEXTURE_ASSET_ID::TUTORIAL_BATTLE2, TEXTURE_ASSET_ID::TUTORIAL_BATTLE3, TEXTURE_ASSET_ID::TUTORIAL_BATTLE4, TEXTURE_ASSET_ID::TUTORIAL_BATTLE5, TEXTURE_ASSET_ID::TUTORIAL_BATTLE6, TEXTURE_ASSET_ID::TUTORIAL_BATTLE7 };
