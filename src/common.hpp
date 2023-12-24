#pragma once

// stlib
#include <fstream> // stdout, stderr..
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

// The glm library provides vector and matrix operations as in GLSL
#include <glm/vec2.hpp>				// vec2
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec3.hpp>             // vec3
#include <glm/mat3x3.hpp>           // mat3
using namespace glm;

#include "tiny_ecs.hpp"
#include "../imgui/imgui.h"

// Simple utility functions to avoid mistyping directory name
// audio_path("audio.ogg") -> data/audio/audio.ogg
// Get defintion of PROJECT_SOURCE_DIR from:
#include "../ext/project_path.hpp"
inline std::string data_path() { return std::string(PROJECT_SOURCE_DIR) + "data"; };
inline std::string shader_path(const std::string& name) {return std::string(PROJECT_SOURCE_DIR) + "/shaders/" + name;};
inline std::string textures_path(const std::string& name) {return data_path() + "/textures/" + std::string(name);};
inline std::string fish_path(const std::string& name) {return data_path() + "/textures/fish/" + std::string(name);};
inline std::string dialogue_path(const std::string& name) { return data_path() + "/textures/dialogue/" + std::string(name); };
inline std::string audio_path(const std::string& name) {return data_path() + "/audio/" + std::string(name);};
inline std::string mesh_path(const std::string& name) {return data_path() + "/meshes/" + std::string(name);};
inline std::string font_path(const std::string& name) { return data_path() + "/fonts/" + std::string(name); };

const int window_width_px = 1500;
const int window_height_px = 900;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

const ImVec4 battle_yellow_colour = ImVec4(0.9804f, 0.9137f, 0.7647f, 1.0f);
const ImVec4 ui_border_colour = ImVec4(0.5922f, 0.3647f, 0.2157f, 1.0f);
const ImVec4 battle_blue_colour = ImVec4(0.3760f, 0.6040f, 0.7100f, 1.0f);
const ImVec4 border_blue_colour = ImVec4(0.231f, 0.369f, 0.494f, 1.0f);
const ImVec4 battle_blue_highlighted = ImVec4(0.4863f, 0.7059f, 0.8078f, 1.0f);
const ImU32 battle_dmg_colour = IM_COL32(255.f, 192.f, 161.f, 255.f);
const ImU32 battle_heal_colour = IM_COL32(109.f, 255.f, 187.f, 255.f);
const vec4 ally1_purple = vec4(0.639f, 0.173f, 0.769f, 1.f);
const vec4 ally1_purple_lighter = vec4(0.922f, 0.812f, 1.f, 1.f);

// The 'Transform' component handles transformations passed to the Vertex shader
// (similar to the gl Immediate mode equivalent, e.g., glTranslate()...)
// We recomment making all components non-copyable by derving from ComponentNonCopyable
struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

// Storing the enum game state here. A global variable is created in main.cpp that stores the current game state.
// TODO: Possible make a new class with multiple enums that handle game states and UI states
enum class GAME_STATE_ID
{
	WORLD = 0,
	INVENTORY = WORLD + 1,
	BATTLE = INVENTORY + 1,
	CUTSCENE = BATTLE + 1,
	SHOP = CUTSCENE + 1,
	PARTY = SHOP + 1,
	MAP = PARTY + 1,
    TUTORIAL = MAP + 1,
	SETTINGS = TUTORIAL + 1,
    TUTORIAL_BASIC = SETTINGS + 1,
    TUTORIAL_FISH = TUTORIAL_BASIC + 1,
    TUTORIAL_BATTLE = TUTORIAL_FISH + 1,
    SAVE = TUTORIAL_BATTLE + 1,
    SAVE_DONE = SAVE + 1,
    LOAD = SAVE_DONE + 1,
    LOAD_SAVE = LOAD + 1,
    DELETE_SAVE = LOAD_SAVE + 1,
    DELETE_SAVE_DONE = DELETE_SAVE + 1,
    LOAD_FAIL = DELETE_SAVE_DONE + 1,
    TRANSITION = LOAD_FAIL + 1,
	CUTSCENE_TRANSITION = TRANSITION + 1,
	START_MENU = CUTSCENE_TRANSITION + 1,
    TELEPORT_1 = START_MENU + 1,
    TELEPORT_2 = TELEPORT_1 + 1,
    TELEPORT_1_DONE = TELEPORT_2 + 1,
    TELEPORT_2_DONE = TELEPORT_1_DONE + 1,
	TUTORIAL_BATTLE_ADVANCED = TELEPORT_2_DONE + 1
};

bool gl_has_errors();

enum class EFFECT_TYPE
{
	BUFF_ATK = 0,
	BUFF_DEF = BUFF_ATK + 1,
	BUFF_SPD = BUFF_DEF + 1,
	DEBUFF_ATK = BUFF_DEF + 1,
	DEBUFF_DEF = DEBUFF_ATK + 1,
	DEBUFF_SPD = DEBUFF_DEF + 1,
	DEBUFF_ALL = DEBUFF_SPD + 1,
	NONE = DEBUFF_ALL + 1 //no effect
};

enum class SKILL_TYPE
{
	ATK = 0,
	HEAL = ATK + 1,
	DEF = HEAL + 1,
	BUFF = DEF + 1,
	DEBUFF = BUFF + 1,
	PERCENTAGE_DMG = DEBUFF + 1,
	NONE = PERCENTAGE_DMG + 1 // special cases, like run
};

// records info about the type of buffs/debuffs and the rounds its effective for
struct Effect {
	EFFECT_TYPE type;
	int num_rounds;
};

// contains an effect, and also the skill name it came from, and value is the DIFF in stats resulted from this effect
struct CurrEffect {
	std::string skill_name;
	Effect effect;
	float value;
};

// For party member skills
struct Skill
{
	std::string skill_name; // skill name is unique
	std::string skill_description;
	SKILL_TYPE skill_type; // Skill type can be attack, buff, or healing?
	float skill_scale; // skill scale factor
	Effect skill_effect;
	float effect_scale; // effect scale factor
	float probability; // this is for enemy ai
};

struct DmgTextVals {
	float value;
	bool targetPlayer; // true = text on rod, false = on enemy
	ImU32 textColour;
	bool shouldDisplay;
};

struct FishSpecies
{
	int id = 0;
	std::string name;
	int price = 0;
	float health = 0.f;
	float attack = 0.f;
	float defense = 0.f;
	float speed = 0.f;
	float critical_rate = 0.05f; //default crit values, meaning 5% change to do 1.5x damage
	float critical_value = 1.5f;
};


struct GiftType
{
	int id = 0;
	std::string name;
	float price = 0.f;
	int texture_id;
};

struct Particle {
	vec2 position, velocity, size, sizeChange;
	vec4 color, colorChange;
	float life;

	Particle()
		: position(0.0f), velocity(0.0f), size(200.f), sizeChange(0.0f), color(1.0f), colorChange(0.0f), life(0.0f) { }
};

//Party Members
const Skill reel = { "Reel", "Reel: Jonah further reels in with the fishing rod, dealing damage to the fish based on the fishing rod's attack", SKILL_TYPE::ATK, 1.f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill release = { "Release", "Release: Jonah lets go of the fishing line to release tension, recovering some durability for the fishing rod", SKILL_TYPE::HEAL, .4f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill determination = { "Determination", "Determination: Increases everyone's attack for 3 rounds based on the fishing rod's attack, duration stackable, max 7 rounds", SKILL_TYPE::BUFF, 1.f, {EFFECT_TYPE::BUFF_ATK, 3}, .5f, 0 };
const Skill manifest = { "Manifest", "Manifest: Creating something out of nothing, an art passed down her clan. Deals damage.", SKILL_TYPE::ATK, 1.3f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill curse = { "Curse", "Curse: Forbidden art that brings down misfortune. Deals damage, debuffs enemy ATK, DEF, SPD for 3 rounds", SKILL_TYPE::ATK, .6f, {EFFECT_TYPE::DEBUFF_ALL, 3}, 0.9f, 0 };
const Skill doom = { "Doom", "Doom: Secret technique that deals damage + follow up damage based on number of status effects on enemy", SKILL_TYPE::ATK, 1.1f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill distract = { "Distract", "Distract: Divert enemy attention by attacking while shouting unnecessarily loud. Decrease enemy ATK for 3 rounds", SKILL_TYPE::ATK, 1.1f, {EFFECT_TYPE::DEBUFF_ATK, 3}, 0.7f, 0 };
const Skill fortress = { "Fortress", "Fortress: Use self as shield for the fishing rod. Increase fishing rod DEF for 5 rounds. Has no effect while under the effect of 'Mode: Indestructible'", SKILL_TYPE::BUFF, 0, {EFFECT_TYPE::BUFF_DEF, 5}, 2.f, 0 };
const Skill indestructible = { "Mode: Indestructible", "Mode: Indestructible: Greatly increase fishing rod DEF for the next round that scales with rounds of 'Fortress' remaining and clear the effects of 'Fortress'", SKILL_TYPE::BUFF, 0, {EFFECT_TYPE::BUFF_DEF, 2}, 3.f, 0 };
const Skill normal_attack = { "Normal Attack", "Normal Attack: A very normal attack", SKILL_TYPE::ATK, 1.f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill run = { "Run", "Run: Cut the fishing line to escape battle", SKILL_TYPE::NONE, 0, {EFFECT_TYPE::NONE, 0}, 0, 0 };

const std::string mc_name = "Jonah";
const std::string ally1_name = "Nix";
const std::string ally2_name = "Lance";
//Party Member Descriptions
const std::string mc_desc = "A fisher on a quest to catch the Legendary Fish in order to claim they are the strongest fisher in the world.";
const std::string ally1_desc = "A practioner of the dark arts and seeker of gourmet meals. "
	"Though her dark magic makes her seem unapproachable, it is balanced out by her calm demeanour and her blatant appetite for fine cuisine. She loves all kinds of seafood. ";
const std::string ally2_desc = "A sturdy and persistent young man. He tends to get lost easily, but no matter where he finds himself, his optimistic and kind heart means he doesn't hesitate to lend a helping hand to those he thinks in need.";

//Enemies
const Skill turtle_clap_clap = { "Clap Clap", "Clap Clap: Turtle claps the rod with its hands (?)", SKILL_TYPE::ATK, 1.f, {EFFECT_TYPE::NONE, 0}, 0, 0.6f };
const Skill turtle_shell_collide = { "Shell Collide", "Shell Collide: Turtle bumps the rod with its shell", SKILL_TYPE::ATK, 1.5f, {EFFECT_TYPE::NONE, 0}, 0, 0.25f };
const Skill turtle_slowing_atk = { "Slowing Attack", "Slowing Attack: Turtle infects all allies with its slowness (applies spd debuff for 3 rounds)", SKILL_TYPE::ATK, .6f, {EFFECT_TYPE::DEBUFF_SPD, 3}, 0.8f, 0.15f};
const Skill walrus_flop_flop = { "Flop Flop", "Flop Flop: Walrus flops the rod", SKILL_TYPE::ATK, 1.1f, {EFFECT_TYPE::NONE, 0}, 0, 0.6f };
const Skill walrus_flip_flop = { "Flip Flop", "Flip Flop: Walrus flips then flops the rod", SKILL_TYPE::ATK, 1.4f, {EFFECT_TYPE::NONE, 0}, 0, 0.4f };
const Skill narwhal_chant = { "Chant", "Chant: Narwhal chants some unicorn languaged spell in its sleep that deals damage", SKILL_TYPE::ATK, 1.f, {EFFECT_TYPE::NONE, 0}, 0, 0.55f };
const Skill narwhal_chant_intense = { "Chant V: Intense", "Chant V: Intense: Narwhal chants some advanced unicorn languaged spell in its sleep that deals intense damage", SKILL_TYPE::ATK, 1.6f, {EFFECT_TYPE::NONE, 0}, 0, .3f };
const Skill narwhal_dream_transfer = { "Dream Transfer", "Dream Transfer: Narwhal transfers all of fishing rod's defense into its own attack for 3 rounds", SKILL_TYPE::DEBUFF, 0, {EFFECT_TYPE::DEBUFF_DEF, 3}, 0.f, 0.15f };
const Skill boss_na = { "Slap Slap", "Slap Slap: Boss does a normal slaping attack", SKILL_TYPE::ATK, 1.f, {EFFECT_TYPE::NONE, 0}, 0, 0.4f };
const Skill boss_heavy = { "Heavy Slap", "Heavy Slap: Boss does a heavier slap", SKILL_TYPE::ATK, 1.3f, {EFFECT_TYPE::NONE, 0}, 0, 0.3f };
const Skill boss_throw_salmon = { "Throw Salmon", "Throw Salmon: Boss throws a salmon at rod", SKILL_TYPE::ATK, 1.5f, {EFFECT_TYPE::NONE, 0}, 0, 0.3f };
const Skill boss_execution = { "Execution", "Execution: Boss blubs: your journey ends here!", SKILL_TYPE::ATK, 20.f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill boss_field = { "Open Field", "Open Field: Summon the waves in its favor", SKILL_TYPE::DEBUFF, 0, {EFFECT_TYPE::DEBUFF_SPD, 20}, 0.5f, 0 };
const Skill boss_balance = { "Balance", "Balance: damage rod for 50% of its current durability", SKILL_TYPE::PERCENTAGE_DMG, 0.5f, {EFFECT_TYPE::NONE, 0}, 0, 0 };

const Skill fin_slap = { "Fin Slap", "Fin Slap: Fish girl smacks the fish with all her might, dealing damage", SKILL_TYPE::ATK, 1.f, {EFFECT_TYPE::NONE, 0}, 0, 0 };
const Skill whirlpool = { "Whirlpool", "Whirlpool: Fish girl swims rapidly around the enemy to distract it, decreasing its defense for 3 rounds", SKILL_TYPE::DEBUFF, 1.f, {EFFECT_TYPE::DEBUFF_DEF, 3}, 0.8f, 0 };
const Skill calm_waters = { "Calm Waters", "Calm Waters: Fish girl steadies the boat, recovering some durability and increasing party defense for this round", SKILL_TYPE::HEAL, 1.2f, {EFFECT_TYPE::BUFF_DEF, 1}, 1.2f, 0 };

// common, uncommon, rare, epic, and legendary
#define COMMON_FISH 10.f
#define UNCOMMON_FISH 14.f
#define RARE_FISH 30.f
#define EPIC_FISH 50.f
#define LEGENDARY_FISH 10000.f

const FishSpecies crackerFish = {0, "Cracker fish", 2, 5};
const FishSpecies carp = {1, "Carp", 1,  5}; // (1, "Carp", 1.f);
const FishSpecies chinook = {2, "Chinook", 100, 12}; // = FishSpecies(2, "Chinook", 5.f);
const FishSpecies cucumberFish = {3, "Cucumber Fish", 5, 12}; // uncommon
const FishSpecies hammerFish = {4, "Hammer Fish", 32, 12}; // epic
const FishSpecies sharpieFish = {5, "Sharpie Fish", 5, 12}; // uncommon
const FishSpecies sweetPotatoFish = {6, "SweetPotato Fish", 13, 12};
const FishSpecies tealFish = {7, "Teal Fish", 3, 12};
const FishSpecies upfish = {8, "Upfish", 11, 12};
// const FishSpecies bubbleFish = {9, "Bubble Fish", 20, 120.f, 20.f, 5.f, 8.f};
const FishSpecies theFish = { 9, "The Fish", 10, 20, 120.f, 20.f, 5.f, 9.f };
const FishSpecies angelFish = {10, "Angel Fish", 6, 12}; // uncommon
const FishSpecies bandages = {11, "Bandages", 4, 12};
const FishSpecies cheeseFish = {12, "Cheese Fish", 12, 12};
const FishSpecies toddlerFish = {13, "Toddler Fish", 7, 12};
const FishSpecies jack = {14, "Jack", 10, 12};
const FishSpecies squid = {15, "Squid", 20, 120.f, 20.f, 5.f, 8.f};
const FishSpecies inkWhale = {16, "Ink Whale", 40, 120.f, 20.f, 5.f, 8.f};
// createEnemy(renderer, { 0, "The Fish", 20, 120.f, 20.f, 5.f, 8.f }, { {"Normal Attack", SKILL_TYPE::ATK, 1.f, EFFECT_TYPE::NONE} });
const FishSpecies sleepyNarwhal = { 17, "Narwhal", 14, 150.f, 33.f, 5.f, 9.f };
const FishSpecies turtle = { 18, "Turtle", 18, 120.f, 19.f, 10.f, 9.f };
const FishSpecies walrus = { 19, "Walrus", 24, 180.f, 25.f, 7.f, 10.f };
const FishSpecies wrinklyCoral = {20, "Wrinkly coral", 8, 12};
const FishSpecies yamRoll = {21, "Yam Roll", 8, 12};
const FishSpecies sealFish = {22, "Seal Fish", 12, 12};
const FishSpecies tacoRay = {23, "Taco Ray", 16, 12};
const FishSpecies blueRay = {24, "Blue Ray", 10, 12};
const FishSpecies babyFlock = {25, "Baby Flock", 1, 12}; // how dare you kill babies
const FishSpecies crab = {26, "Crab", 20, 12};
const FishSpecies boss = { 16, "Boss", 30, 290.f, 32.f, 15.f, 15.f };
const std::vector<FishSpecies> possibleEnemies = {sleepyNarwhal, turtle, walrus};

// IMPORTANT: !!! cannot be more fish_species than texture_count !!!
const std::unordered_map<int, FishSpecies> id_to_fish_species = {
	{0, crackerFish},
	{1, carp},
	{2, chinook},
	{3, cucumberFish},
	{4, hammerFish},
	{5, sharpieFish},
	{6, sweetPotatoFish},
	{7, tealFish},
	{8, upfish},
	{9, theFish},
	{10, angelFish},
	{11, bandages},
	{12, cheeseFish},
	{13, toddlerFish},
	{14, jack},
	{15, squid},
	{16, inkWhale},
	{17, sleepyNarwhal},
	{18, turtle},
	{19, walrus},
	{20, wrinklyCoral},
	{21, yamRoll},
	{22, sealFish},
	{23, tacoRay},
	{24, blueRay},
	{25, babyFlock},
	{26, crab}
};

const GiftType bell = {1, "Bell", 5.f,  15};

const GiftType shoe = {2, "Shoe", 10.f, 15};

const std::unordered_map<int, GiftType> id_to_gift_type = {
	{1, bell},
	{2, shoe},
};

struct Fishable // Gift + FishSpecies both have the fishable component
{
    float probability; // higher probability means less likely to catch (I know this doesn't make sense)
    Fishable(float probability)
    {
        this->probability = probability;
    };
};

struct Lake
{
    int id = 0;
    std::unordered_map<int, Fishable> id_to_fishable;
};

const Lake lake1 = {1, { // more common fish than rare fish, all catchable fish go in here, no bosses
	{0, Fishable(COMMON_FISH)},
	{1, Fishable(COMMON_FISH)},
	{2, Fishable(LEGENDARY_FISH)},
	{3, Fishable(UNCOMMON_FISH)},
	{4, Fishable(EPIC_FISH)},
	{5, Fishable(UNCOMMON_FISH)},
	{6, Fishable(RARE_FISH)},
	{7, Fishable(COMMON_FISH)},
	{8, Fishable(RARE_FISH)},
	{10, Fishable(UNCOMMON_FISH)},
	{11, Fishable(COMMON_FISH)},
	{12, Fishable(RARE_FISH)},
	{13, Fishable(RARE_FISH)},
	{14, Fishable(RARE_FISH)}
}};

const Lake lake2 = {1, { // more common fish than rare fish, all catchable fish go in here, no bosses
	{0, Fishable(COMMON_FISH)},
	{1, Fishable(COMMON_FISH)},
	{3, Fishable(UNCOMMON_FISH)},
	{5, Fishable(UNCOMMON_FISH)},
	{7, Fishable(COMMON_FISH)},
	{10, Fishable(UNCOMMON_FISH)},
	{11, Fishable(COMMON_FISH)},
	{20, Fishable(RARE_FISH)},
	{21, Fishable(RARE_FISH)},
	{22, Fishable(RARE_FISH)},
	{23, Fishable(RARE_FISH)},
	{24, Fishable(UNCOMMON_FISH)},
	{25, Fishable(EPIC_FISH)},
	{26, Fishable(EPIC_FISH)}
}};

const std::unordered_map<int, Lake> id_to_lake = {
	{1, lake1},
	{2, lake2}
};