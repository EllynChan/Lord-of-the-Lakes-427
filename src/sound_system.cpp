// Header
#include "sound_system.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>

#include "tiny_ecs_registry.hpp"
bool SoundSystem::init(GAME_STATE_ID* game_state) {
	this->current_game_state = game_state;
	this->previous_game_state = *game_state;
	this->previous_lake.id = 1;
	//////////////////////////////////////
// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "Failed to initialize SDL Audio");
		return false;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
	{
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	lake_one_world_bgm = Mix_LoadMUS(audio_path("lake_one_world_bgm.wav").c_str());
	lake_two_world_bgm = Mix_LoadMUS(audio_path("lake_two_world_bgm.wav").c_str());
	lake_one_battle_bgm = Mix_LoadMUS(audio_path("lake_one_battle_bgm.wav").c_str());
	start_fish_splash = Mix_LoadWAV(audio_path("start_splash.wav").c_str());
	catch_fish_splash = Mix_LoadWAV(audio_path("catch_fish_splash.wav").c_str());
	rod_swing = Mix_LoadWAV(audio_path("fish_rod_swing.wav").c_str());
	catch_alert = Mix_LoadWAV(audio_path("message_alert.wav").c_str());
	chaching = Mix_LoadWAV(audio_path("chaching.wav").c_str());
	denied = Mix_LoadWAV(audio_path("denied.wav").c_str());
	sell = Mix_LoadWAV(audio_path("sell.wav").c_str());
	on_hit = Mix_LoadWAV(audio_path("on_hit.wav").c_str());
	buff = Mix_LoadWAV(audio_path("buff.wav").c_str());
	debuff = Mix_LoadWAV(audio_path("debuff.wav").c_str());
	curse = Mix_LoadWAV(audio_path("curse.wav").c_str());
	manifest = Mix_LoadWAV(audio_path("manifest.wav").c_str());
	rock = Mix_LoadWAV(audio_path("rock.wav").c_str());

	if (lake_one_world_bgm == nullptr || start_fish_splash == nullptr || catch_fish_splash == nullptr ||
		rod_swing == nullptr || catch_alert == nullptr || lake_one_battle_bgm == nullptr)
	{
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("lake_one_world_bgm.wav").c_str(),
			audio_path("lake_one_battle_bgm.wav").c_str(),
			audio_path("lake_two_world_bgm.wav").c_str(),
			audio_path("start_splash.wav").c_str(),
			audio_path("catch_fish_splash").c_str(),
			audio_path("fish_rod_swing").c_str(),
			audio_path("message_alert").c_str(),
			audio_path("chaching").c_str(),
			audio_path("denied").c_str(),
			audio_path("sell").c_str(),
			audio_path("on_hit").c_str(),
			audio_path("buff").c_str(),
			audio_path("debuff").c_str(),
			audio_path("curse").c_str(),
			audio_path("manifest").c_str(),
			audio_path("rock").c_str()
		);
		return false;
	}

	// first bgm playing right now is always lake_one_bgm. the title screen music
	playBGM(lake_one_world_bgm);
	changeMusicVolume(16);
	Mix_Volume(-1, 32);
	return true;
}

void SoundSystem::update() {
	// detect if state changed
	Mix_Music* bgm = lake_one_world_bgm;
	if (registry.lakes.has(player)) {
		lakeInfo = registry.lakes.get(player);
		switch (lakeInfo.id) {
		case 1:
			bgm = lake_one_world_bgm;
			break;
		case 2:
			bgm = lake_two_world_bgm;
			break;
		default:
			bgm = lake_one_world_bgm;
		}
	}

	if (*current_game_state != previous_game_state || lakeInfo.id != previous_lake.id) {
		switch (*current_game_state) {
			case GAME_STATE_ID::WORLD:
				if (previous_game_state == GAME_STATE_ID::BATTLE || previous_game_state == GAME_STATE_ID::START_MENU || lakeInfo.id != previous_lake.id) {
					playBGM(bgm);
				}
				changeMusicVolume(16);
				break;
			case GAME_STATE_ID::BATTLE:
				playBGM(lake_one_battle_bgm);
				changeMusicVolume(16);
				break;
			default:
				// dim music volume on states other than world or battle
				if (previous_game_state == GAME_STATE_ID::BATTLE || previous_game_state == GAME_STATE_ID::START_MENU || lakeInfo.id != previous_lake.id) {
					playBGM(bgm);
				}
				changeMusicVolume(8);
		}
		previous_lake = lakeInfo;
		previous_game_state = *current_game_state;
	}
}
SoundSystem::~SoundSystem()
{
    // Destroy music components
    if (lake_one_world_bgm != nullptr)
        Mix_FreeMusic(lake_one_world_bgm);
	if (lake_one_battle_bgm != nullptr)
		Mix_FreeMusic(lake_one_battle_bgm);
	if (lake_two_world_bgm != nullptr)
		Mix_FreeMusic(lake_two_world_bgm);
    if (start_fish_splash != nullptr)
        Mix_FreeChunk(start_fish_splash);
    if (catch_fish_splash != nullptr)
        Mix_FreeChunk(catch_fish_splash);
    if (rod_swing != nullptr)
        Mix_FreeChunk(rod_swing);
    if (catch_alert != nullptr)
        Mix_FreeChunk(catch_alert);
	if (chaching != nullptr)
		Mix_FreeChunk(chaching);
	if (denied != nullptr)
		Mix_FreeChunk(denied);
	if (sell != nullptr)
		Mix_FreeChunk(sell);
    Mix_CloseAudio();
}

void SoundSystem::playSound(Mix_Chunk* sound_effect) {
	Mix_PlayChannel(-1, sound_effect, 0);
}

void SoundSystem::playBGM(Mix_Music* track) {
	Mix_PlayMusic(track, -1);
}

void SoundSystem::changeMusicVolume(int volume) {
	Mix_VolumeMusic(volume);
}