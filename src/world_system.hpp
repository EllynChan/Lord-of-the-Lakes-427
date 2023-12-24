#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#include "sound_system.hpp"
#include "render_system.hpp"

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	WorldSystem();

	// Creates a window
	GLFWwindow *create_window();

	// starts the game
	void init(RenderSystem *renderer, SoundSystem *sound_system, GAME_STATE_ID *game_state);

	// Releases all associated resources
	~WorldSystem();

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Check for collisions
	void handle_collisions();

	// Should the game be over ?
	bool is_over() const;

    // restart level
    void restart_game();

    bool should_load_save = false;

private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 pos);
	void on_mouse_button(int button, int action, int mods);

	// helper function for sprites
	void setSpriteFrames(Sprite& sprite, int start, int end, int current, int num, bool loop = false, double frame_dur = 0.1);
	// helper function for sprites
	void setPositionRelativeToPlayer(Motion& motion, vec2 offset_left, vec2 offset_right, vec2 offset_down, vec2 offset_up);

	void catch_fish(int fish_id, bool from_battle = false);
	void start_fishing_timer(Entity fishingRod, Entity shinySpot);
	FishingResult select_fish(Entity fishingRod, Entity shinySpot);

	// OpenGL window handle
	GLFWwindow *window;

	// Game state
	GAME_STATE_ID* current_game_state;

	RenderSystem *renderer;
	SoundSystem* sound_system;
	Entity player;
	Entity caughtFish;
	Entity map;
	Entity fishingRod;
	Entity randomWaterTile;
	Entity lakeEntity;
	Entity catchingBar;
	Entity exclamation;

	float current_speed;
	float next_fishshadow_spawn;
	float next_fish_spawn;

	// determines mini game
	float fish_rarity;

	// constants TODO M1: where are constants supposed to go?
	// Lake lake1 = Lake({{1, Fishable(10)}, {2, Fishable(20)}});
	// Lake ALL_LAKES[1] = {lake1};

	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1
	std::uniform_real_distribution<float> uniform_dist_timer{ 1.f, 3.f }; // number between 1..3
	std::uniform_int_distribution<int> uniform_dist_int{ -1, 1 }; // number between -1, 0, 1
};
