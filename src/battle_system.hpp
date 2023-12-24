
#pragma once

#include "tiny_ecs_registry.hpp"
#include "sound_system.hpp"
#include "common.hpp"

class BattleSystem {
public:
	enum StateEnum {
		STATE_PLAYER_TURN = 0,
		STATE_PLAYER_ACTING = STATE_PLAYER_TURN + 1,
		STATE_ENEMY_TURN = STATE_PLAYER_ACTING + 1,
		STATE_ENEMY_ACTING = STATE_ENEMY_TURN + 1,
		STATE_EFFECT_PLAYING = STATE_ENEMY_ACTING + 1,
		STATE_EFFECT_PLAYING_ENEMY = STATE_EFFECT_PLAYING + 1,
		STATE_ROUND_BEGIN = STATE_EFFECT_PLAYING_ENEMY + 1,
		STATE_ENEMY_DEFEATED = STATE_ROUND_BEGIN + 1,
		STATE_PLAYER_DEFEATED = STATE_ENEMY_DEFEATED + 1,
	};
	enum AnimeEnum {
		PLAYER_ATK = 0,
		PLAYER_HEAL = PLAYER_ATK + 1,
		PLAYER_BUFF_ATK = PLAYER_HEAL + 1,
		PLAYER_BUFF_DEF = PLAYER_BUFF_ATK + 1,
		ENEMY_ATK = PLAYER_BUFF_DEF + 1,
		ALLY_ATK = ENEMY_ATK + 1,
		ENEMY_DEBUFF = ALLY_ATK + 1,
		ALLY_MANIFEST = ENEMY_DEBUFF + 1,
		ALLY_CURSE = ALLY_MANIFEST + 1,
		ALLY_DOOM = ALLY_CURSE + 1,
		ENEMY_EXECUTION = ALLY_DOOM + 1,
		ALLY_DISTRACT = ENEMY_EXECUTION + 1,
		NONE = ALLY_DISTRACT + 1 // default
	};

	bool initialized;
	int selectedSkill;
	int enemySelectedSkill;
	bool enemyActed;
	int roundCounter;
	DmgTextVals dmgVals;
	//these are rod stats only
	float currHealth;
	float maxHealth;
	float currAttack = -1;
	float currDefense;
	std::vector<CurrEffect> rodEffects;

	float enemyMaxHealth;
	void start(GAME_STATE_ID* current_game_state, SoundSystem* sound_system);
	void update(float step_ms);
	void checkRoundOver(int actionIndex);
	void roundUpdate();
	void createParticles(AnimeEnum anime);
	void reset();

	StateEnum curr_battle_state;
	AnimeEnum selectedAnime;
	std::vector<PartyMember> allMembers;
	int currMemberIndex;
	FishSpecies enemySpecies;
	Enemy* enemy;

	// reference: https://learnopengl.com/In-Practice/2D-Game/Particles
	std::vector<Particle> particles;
	unsigned int nr_particles = 20;
private:
	GAME_STATE_ID* current_game_state;
	SoundSystem* sound_system;
	Durability* durability;
	Attack* attack;
	Defense* defense;
	void activateSkill();
	void enemyAction();
};
