#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
// internal
#include "common.hpp"
#include "components.hpp"
class SoundSystem
{
public:
	bool init(GAME_STATE_ID* game_state);
	void update();
	// music references
	Mix_Music* lake_one_world_bgm;
	Mix_Music* lake_one_battle_bgm;
	Mix_Music* lake_two_world_bgm;
	Mix_Chunk* start_fish_splash;
	Mix_Chunk* catch_fish_splash;
	Mix_Chunk* rod_swing;
	Mix_Chunk* catch_alert;
	Mix_Chunk* chaching;
	Mix_Chunk* denied;
	Mix_Chunk* sell;
	Mix_Chunk* on_hit;
	Mix_Chunk* buff;
	Mix_Chunk* debuff;
	Mix_Chunk* curse;
	Mix_Chunk* manifest;
	Mix_Chunk* rock;

	Entity player;
	LakeId lakeInfo;
	//void playMusic();
	void playSound(Mix_Chunk* sound_effect);
	void playBGM(Mix_Music* track);
	void changeMusicVolume(int volume);
	~SoundSystem();

private:
	GAME_STATE_ID* current_game_state;
	GAME_STATE_ID previous_game_state;

	LakeId* current_lake;
	LakeId previous_lake;
};
