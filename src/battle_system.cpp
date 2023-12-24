// in registry, allies should have a name, an abilities list, overworld sprite, and battle portrait sprite
// store enemy info - fish species, health, abilities
// store fishing rod health
// fishing rod and characters animation info
//from render system press enter, modify character and rod state in a function here. the hp bar function in render system must get hp info from this file so it can be updated.
#include "battle_system.hpp"
#include <iostream>
#include <random>
bool notActivated = true;

float PARTICLE_DELAY = 100;
bool soundPlayed = false;

void RespawnParticle(Particle& particle, vec2 position, vec2 velocity, vec2 size, vec2 sizeChange, vec4 color, vec4 colorChange)
{
	float random = ((rand() % 100) - 50) / 10.0f;
	//float rColor = 0.5f + ((rand() % 100) / 100.0f);
	particle.position = position + random;
	particle.color = color;
	particle.life = 1.0f;
	particle.size = size;
	particle.sizeChange = sizeChange;
	particle.colorChange = colorChange;
	// particle.velocity = { velocity.x * 0.1f * cos(random), velocity.y * 0.1f * sin(random) };
	particle.velocity = velocity;
}

bool compareBySpd(const PartyMember& a, const PartyMember& b) {
	return a.stats.speed > b.stats.speed;
}

void BattleSystem::start(GAME_STATE_ID* game_state_arg, SoundSystem* sound_system_arg)
{
	this->current_game_state = game_state_arg;
	this->sound_system = sound_system_arg;
	roundCounter = 1;
	currMemberIndex = -1;
	curr_battle_state = STATE_ROUND_BEGIN;
	selectedSkill = -1;
	enemySelectedSkill = -1;
	dmgVals = { 0, false, battle_dmg_colour, false };
	selectedAnime = NONE;
	initialized = false;
	enemyActed = false;
	enemy = nullptr;
	for (int i = 0; i < nr_particles; ++i) {
		particles.push_back(Particle());
	}
}

void BattleSystem::update(float step_ms)
{
	//load enemy
	if (registry.enemies.entities.size() > 0) {
		if (enemySpecies.name != registry.enemies.get(registry.enemies.entities[0]).species.name) {
			enemy = &registry.enemies.get(registry.enemies.entities[0]);
			enemySpecies = enemy->species;
			enemyMaxHealth = enemySpecies.health;
		}
	}
	//assuming only 1 unique rod and load rod
	if (registry.fishingRods.entities.size() > 0) {
		if (currAttack == -1) {
			Entity rod = registry.fishingRods.entities[0];
			durability = &registry.durabilities.get(rod);
			currHealth = durability->current;
			maxHealth = durability->max;
			attack = &registry.attacks.get(rod);
			defense = &registry.defenses.get(rod);
			currAttack = (float) attack->damage;
			currDefense = (float) defense->value;
		}
	}

	//load all party members if haven't done so, and set active character to the one with highest speed
	if (registry.partyMembers.entities.size() > 0 && currMemberIndex == -1) {
		std::vector<Entity> members = registry.partyMembers.entities;
		for (int i = 0; i < members.size(); i++) {
			PartyMember p = registry.partyMembers.get(members[i]);
			allMembers.push_back(p);
		}
		std::sort(allMembers.begin(), allMembers.end(), compareBySpd);
		int enemyIn = 0;
		for (int i = 0; i < allMembers.size(); i++) {
			if (allMembers[i].stats.speed < enemySpecies.speed && enemyIn == 0) {
				enemyIn = 1;
				enemy->actionIndex = i;
				allMembers[i].actionIndex = i + 1;
			}
			else {
				allMembers[i].actionIndex = i + enemyIn;
			}
		}
		if (enemy->actionIndex == -1)
			enemy->actionIndex = allMembers.size();
		currMemberIndex = 0; //here assume at least the main character will be in partyMembers. allMembers should be kept sorted by speed
	}

	initialized = true;

	//handle player and enemy actions here
	if (curr_battle_state == STATE_PLAYER_TURN) {
		if (selectedSkill != -1 && allMembers[currMemberIndex].skills[selectedSkill].skill_name == "Run") {
			dmgVals.shouldDisplay = false;
			curr_battle_state = STATE_PLAYER_DEFEATED;
		} else if (selectedSkill != -1) {
			curr_battle_state = STATE_PLAYER_ACTING;
			activateSkill();
			selectedSkill = -1;
		}
	}
	else if (curr_battle_state == STATE_ENEMY_TURN) {
		enemyAction();
	}
	// update all particles
	for (unsigned int i = 0; i < nr_particles; ++i)
	{
		Particle& p = particles[i];
		p.life -= 0.04f; // reduce life
		if (p.life > 0.0f)
		{	// particle is alive, thus update
			p.position -= p.velocity;
			p.size += p.sizeChange;
			p.color += p.colorChange;
		}
	}
	PARTICLE_DELAY -= step_ms;

	if (curr_battle_state == STATE_EFFECT_PLAYING) {
		if (!soundPlayed) {
			switch (selectedAnime) {
				case ALLY_MANIFEST:
				case ALLY_DISTRACT:
				case PLAYER_ATK:
				case ENEMY_ATK:
				case ALLY_ATK:
					sound_system->playSound(sound_system->on_hit);
					break;
				case PLAYER_BUFF_ATK:
				case PLAYER_BUFF_DEF:
				case PLAYER_HEAL:
					sound_system->playSound(sound_system->buff);
					break;
				case ALLY_CURSE:
				case ENEMY_DEBUFF:
					sound_system->playSound(sound_system->debuff);
					break;
				case ALLY_DOOM:
					sound_system->playSound(sound_system->curse);
					break;
				case ENEMY_EXECUTION:
					sound_system->playSound(sound_system->rock);
					break;
			}
			soundPlayed = true;
		}
	}
	else {
		soundPlayed = false;
	}
}

void BattleSystem::createParticles(AnimeEnum anime)
{
	if (curr_battle_state != STATE_PLAYER_ACTING && curr_battle_state != STATE_EFFECT_PLAYING)
		return;
	float circleRadius;
	float angle = rand();
	vec2 size = vec2(10.f);
	vec2 sizeChange = vec2(0);
	vec4 color = vec4(1.f);
	vec4 colorChange = vec4(0);
	float x;
	float y;
	vec2 pos = vec2(0);
	vec2 vel = vec2(0);
	float delay = 100.f;
	switch (anime) {
	case ALLY_MANIFEST:
		circleRadius = 100.f;
		angle = rand() % 10 - 5.f;
		x = circleRadius * cos(angle);
		y = circleRadius * sin(angle);
		pos = vec2(window_width_px / 2 + x, 280.f + y);
		vel = vec2(4 * x / sqrt(x * x + y * y), 4 * y / sqrt(x * x + y * y));
		sizeChange = vec2(0.6f);
		break;
	case ALLY_DOOM:
		delay = 20.f;
		pos = vec2(window_width_px / 2, 280.f);
		vel = vec2(5.f * (rand() % 10 - 5.f), 0);
		size = vec2(60.f, 40.f);
		sizeChange = vec2(-1.5f);
		colorChange = vec4(0, 0, 0, -0.009f);
		int random = rand() % 2;
		if (random == 1) {
			color = ally1_purple_lighter;
		}
		else {
			color = ally1_purple;
		}
		break;
	}
	// add new particles
	if (PARTICLE_DELAY <= 0) {
		for (unsigned int i = 0; i < nr_particles; ++i)
		{
			if (particles[i].life <= 0) {
				RespawnParticle(particles[i], pos, vel, size, sizeChange, color, colorChange);
				PARTICLE_DELAY = delay;
				break;
			}

		}
	}
}
// TODO: regarding the skills system, i just realized very late that there should be a struct to track the original stat, so overlapping buff types don't scale off the already buffed stat...
void BattleSystem::activateSkill()
{
	if (curr_battle_state != STATE_PLAYER_ACTING)
		return;
	//std::cout << allMembers[currMemberIndex].stats.speed << std::endl;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dmgDist(-2, 2); // damage has a random range of 4
	std::uniform_real_distribution<float> critDist(0.f, 1.f); // rand representing crit rate, 0 - 100%
	Skill currSkill = allMembers[currMemberIndex].skills[selectedSkill];
	std::string currSkillName = currSkill.skill_name;
	float damage;
	float chosenAtk = currAttack;
	if (allMembers[currMemberIndex].name != "Jonah") //Watch out! MC's name change will fail this code
		chosenAtk = allMembers[currMemberIndex].stats.attack;
	if (critDist(gen) < allMembers[currMemberIndex].stats.critical_rate) {
		dmgVals.textColour = IM_COL32_WHITE;
		damage = allMembers[currMemberIndex].stats.critical_value * (chosenAtk * currSkill.skill_scale + dmgDist(gen)) - enemySpecies.defense;
	}
	else {
		dmgVals.textColour = battle_dmg_colour;
		damage = (chosenAtk * currSkill.skill_scale + dmgDist(gen)) - enemySpecies.defense;
	}
	if (damage < 1)
		damage = 1;
	dmgVals.value = damage;
	dmgVals.targetPlayer = false;
	float healingVal = allMembers[currMemberIndex].stats.healing_bonus * currSkill.skill_scale * maxHealth + dmgDist(gen);
	switch (currSkill.skill_type)
	{
		case SKILL_TYPE::ATK:
			dmgVals.shouldDisplay = true;
			if (currSkillName == "Reel") {
				sound_system->playSound(sound_system->catch_fish_splash);
				selectedAnime = PLAYER_ATK;
			}
			else if (currSkillName == "Manifest") {
				sound_system->playSound(sound_system->manifest);
				selectedAnime = ALLY_MANIFEST;
			}
			else if (currSkillName == "Curse") {
				selectedAnime = ALLY_CURSE;
			}
			else if (currSkillName == "Doom") {
				selectedAnime = ALLY_DOOM;
				int multiplier = enemy->currEffects.size();
				for (CurrEffect e : enemy->currEffects) {
					if (e.effect.type == EFFECT_TYPE::DEBUFF_ALL) {
						multiplier += 2;
					}
				}
				if (multiplier > 7)
					multiplier = 7;
				if (multiplier > 0) {
					float bonusDmg = multiplier * 0.45 * damage + dmgDist(gen);
					allMembers[currMemberIndex].followUpDmg.push_back(bonusDmg);
				}
			}
			else if (currSkillName == "Distract") {
				selectedAnime = ALLY_DISTRACT;
			}
			else {
				selectedAnime = ALLY_ATK;
			}
			if (enemySpecies.health > damage) {
				enemySpecies.health -= damage;
			}
			else {
				enemySpecies.health = 0;
			}
			for (float a : allMembers[currMemberIndex].followUpDmg) {
				if (enemySpecies.health > a) {
					enemySpecies.health -= a;
				}
				else {
					enemySpecies.health = 0;
				}
			}
			break;
		case SKILL_TYPE::HEAL:
			dmgVals.shouldDisplay = true;
			dmgVals.value = healingVal;
			dmgVals.targetPlayer = true;
			dmgVals.textColour = battle_heal_colour;
			if ((currHealth + healingVal) > maxHealth && (durability->current + healingVal) > durability->max) {
				currHealth = maxHealth;
				durability->current = durability->max;
			}
			else {
				currHealth += healingVal;
				durability->current += healingVal;
			}
			selectedAnime = PLAYER_HEAL;
			break;
		case SKILL_TYPE::BUFF:
			dmgVals.shouldDisplay = false;
			if (currSkill.skill_effect.type == EFFECT_TYPE::BUFF_ATK) {
				selectedAnime = PLAYER_BUFF_ATK;
			}
			else if (currSkill.skill_effect.type == EFFECT_TYPE::BUFF_DEF) {
				selectedAnime = PLAYER_BUFF_DEF;
			}
			break;
		case SKILL_TYPE::NONE:
			break;
	}

	CurrEffect effect;
	bool hasEffect = false;
	int effectIndex = -1;
	float resultStat = 0;
	switch (currSkill.skill_effect.type) {
		case EFFECT_TYPE::DEBUFF_ALL:
			effect = { currSkillName, {currSkill.skill_effect.type, currSkill.skill_effect.num_rounds}, currSkill.effect_scale }; // note in debuff all's case, just record the scaling factor as value cause multiple values are changed
			hasEffect = false;
			effectIndex = -1;
			for (int i = 0; i < enemy->currEffects.size(); i++) {
				if (enemy->currEffects[i].skill_name == currSkillName) {
					hasEffect = true;
					effectIndex = i;
				}
			}
			if (hasEffect) {
				enemy->currEffects[effectIndex] = effect;
			}
			else {
				enemySpecies.attack = enemySpecies.attack * currSkill.effect_scale;
				enemySpecies.defense = enemySpecies.defense * currSkill.effect_scale;
				enemySpecies.speed = enemySpecies.speed * currSkill.effect_scale;
				enemy->currEffects.push_back(effect);
			}
			break;
		case EFFECT_TYPE::DEBUFF_ATK:
			effect = { currSkillName, {currSkill.skill_effect.type, currSkill.skill_effect.num_rounds}, currSkill.effect_scale };
			hasEffect = false;
			effectIndex = -1;
			for (int i = 0; i < enemy->currEffects.size(); i++) {
				if (enemy->currEffects[i].skill_name == currSkillName) {
					hasEffect = true;
					effectIndex = i;
				}
			}
			if (hasEffect) {
				enemy->currEffects[effectIndex] = effect;
			}
			else {
				enemySpecies.attack = enemySpecies.attack * currSkill.effect_scale;
				enemy->currEffects.push_back(effect);
			}
			break;
		case EFFECT_TYPE::BUFF_ATK:
			for (int i = 0; i < allMembers.size(); i++) {
				resultStat = currSkill.effect_scale * allMembers[i].stats.attack;
				if (currSkill.skill_name == "Determination")
					resultStat = currAttack * currSkill.effect_scale + allMembers[i].stats.attack;
				effect = { currSkillName, currSkill.skill_effect, resultStat - allMembers[i].stats.attack };
				hasEffect = false;
				effectIndex = -1;
				for (int j = 0; j < allMembers[i].currEffects.size(); j++) {
					if (allMembers[i].currEffects[j].skill_name == currSkillName) {
						hasEffect = true;
						effectIndex = j;
					}
				}
				if (hasEffect) {
					//buff already exists, then refresh duration and don't stack buff value
					if (currSkill.skill_name == "Determination") {
						int num_rounds = effect.effect.num_rounds + 3;
						if (num_rounds > 7)
							num_rounds = 7;
						allMembers[i].currEffects[effectIndex] = { currSkillName, {currSkill.skill_effect.type, num_rounds }, allMembers[i].currEffects[effectIndex].value };
					}
					else {
						allMembers[i].currEffects[effectIndex] = effect;
					}
				}
				else {
					allMembers[i].currEffects.push_back(effect);
					allMembers[i].stats.attack = resultStat;
				}
			}
			break;
		case EFFECT_TYPE::BUFF_DEF:
			resultStat = currSkill.effect_scale * currDefense;
			if (currSkill.skill_name == "Mode: Indestructible") {
				resultStat = currSkill.effect_scale * 5.f; //this 5 is supposed to be the original def but rn there's no stat for that
				for (int i = 0; i < rodEffects.size(); i++) {
					if (rodEffects[i].skill_name == "Fortress") {
						resultStat += rodEffects[i].effect.num_rounds * 5.f;
						currDefense -= static_cast<int>(rodEffects[i].value);
						rodEffects.erase(rodEffects.begin() + i);
					}
				}
			}
			// this setting is fine and all... but its mainly done because stacking the same buff type is broken rn
			else if (currSkill.skill_name == "Fortress") {
				for (int i = 0; i < rodEffects.size(); i++) {
					if (rodEffects[i].skill_name == "Mode: Indestructible") {
						return;
					}
				}
			}
			effect = { currSkillName, currSkill.skill_effect, resultStat - currDefense };
			hasEffect = false;
			effectIndex = -1;
			for (int i = 0; i < rodEffects.size(); i++) {
				if (rodEffects[i].skill_name == currSkillName) {
					hasEffect = true;
					effectIndex = i;
				}
			}
			if (hasEffect) {
				if (currSkill.skill_name == "Mode: Indestructible") {
					currDefense -= rodEffects[effectIndex].value;
					effect.value = 10; //hard coded due to no original stat track
					currDefense += effect.value;
				}
				rodEffects[effectIndex] = effect;
			}
			else {
				rodEffects.push_back(effect);
				currDefense = resultStat;
			}
			break;
	}
}

void BattleSystem::enemyAction()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dmgDist(-2, 2);
	std::uniform_real_distribution<float> critDist(0.f, 1.f);
	std::uniform_real_distribution<float> skillDist(0.f, 1.f);

	int skillIndex = 0;
	float prob = skillDist(gen);
	float culmulatedProb = enemy->skills[0].probability;
	for (int i = 0; i < enemy->skills.size() - 1; i++) {
		if (prob > culmulatedProb && prob < (enemy->skills[i + 1].probability + culmulatedProb)) {
			skillIndex = i + 1;
		}
		culmulatedProb += enemy->skills[i + 1].probability;
	}
	if (enemySpecies.name == "Boss") {
		if (roundCounter > 15)
			skillIndex = 3;
		if (roundCounter == 1)
			skillIndex = 4;
		if (enemySpecies.health <= 0.6f * enemyMaxHealth && notActivated) {
			skillIndex = 5;
			notActivated = false;
		}
	}
	enemySelectedSkill = skillIndex;
	Skill currSkill = enemy->skills[skillIndex];
	float damage;
	if (critDist(gen) < enemySpecies.critical_rate) {
		dmgVals.textColour = IM_COL32_WHITE;
		damage = enemySpecies.critical_value * (currSkill.skill_scale * enemySpecies.attack + dmgDist(gen)) - currDefense;
	}
	else {
		dmgVals.textColour = battle_dmg_colour;
		damage = (currSkill.skill_scale * enemySpecies.attack + dmgDist(gen)) - currDefense;
	}
	if (damage < 1)
		damage = 1;
	dmgVals.value = damage;
	dmgVals.targetPlayer = true;
	switch (currSkill.skill_type)
	{
		case SKILL_TYPE::PERCENTAGE_DMG:
			damage = currSkill.skill_scale * currHealth;
			dmgVals.value = damage;
		case SKILL_TYPE::ATK:
			dmgVals.shouldDisplay = true;
			if (currHealth > damage && durability->current > damage) {
				currHealth -= damage;
				durability->current -= damage;
			}
			else {
				currHealth = 0.f;
				durability->current = 0.f;
			}
			selectedAnime = ENEMY_ATK;
			if (currSkill.skill_name == "Execution")
				selectedAnime = ENEMY_EXECUTION;
			break;
		case SKILL_TYPE::DEBUFF:
			dmgVals.shouldDisplay = false;
			selectedAnime = ENEMY_DEBUFF;
			break;
	}
	switch (currSkill.skill_effect.type) {
		case EFFECT_TYPE::DEBUFF_SPD:
			for (int i = 0; i < allMembers.size(); i++) {
				CurrEffect effect = { currSkill.skill_name, currSkill.skill_effect, currSkill.effect_scale * allMembers[i].stats.speed - allMembers[i].stats.speed };
				bool hasEffect = false;
				int effectIndex = -1;
				for (int j = 0; j < allMembers[i].currEffects.size(); j++) {
					if (allMembers[i].currEffects[j].skill_name == currSkill.skill_name) {
						hasEffect = true;
						effectIndex = j;
					}
				}
				if (hasEffect) {
					allMembers[i].currEffects[effectIndex] = effect;
				}
				else {
					allMembers[i].currEffects.push_back(effect);
					allMembers[i].stats.speed = currSkill.effect_scale * allMembers[i].stats.speed;
				}
			}
			break;
		case EFFECT_TYPE::DEBUFF_DEF:
			CurrEffect effect = { currSkill.skill_name, currSkill.skill_effect, currSkill.effect_scale * currDefense - currDefense };
			bool hasEffect = false;
			int effectIndex = -1;
			for (int i = 0; i < rodEffects.size(); i++) {
				if (rodEffects[i].skill_name == currSkill.skill_name) {
					hasEffect = true;
					effectIndex = i;
				}
			}
			if (hasEffect) {
				rodEffects[effectIndex] = effect;
			}
			else {
				rodEffects.push_back(effect);
				currDefense = currSkill.effect_scale * currDefense;
			}

			if (currSkill.skill_name == "Dream Transfer") {
				CurrEffect effectBuff = { currSkill.skill_name, {EFFECT_TYPE::BUFF_ATK, currSkill.skill_effect.num_rounds}, -1.f * effect.value };
				bool hasEffect = false;
				int effectIndex = -1;
				for (int i = 0; i < enemy->currEffects.size(); i++) {
					if (enemy->currEffects[i].skill_name == currSkill.skill_name) {
						hasEffect = true;
						effectIndex = i;
					}
				}
				if (hasEffect) {
					enemy->currEffects[effectIndex] = effectBuff;
				}
				else {
					enemy->currEffects.push_back(effectBuff);
					enemySpecies.attack -= effect.value; // this is minusing a negative, which is a buff to enemy
				}
			}

			break;
	}
	curr_battle_state = STATE_ENEMY_ACTING;
}

void BattleSystem::checkRoundOver(int actionIndex) {
	if (enemySpecies.health <= 0.f) {
		curr_battle_state = STATE_ENEMY_DEFEATED;

		Entity rod = registry.fishingRods.entities[0];
		PendingCaughtFish& pending = registry.pendingCaughtFish.emplace(rod);
		pending.species_id = enemySpecies.id;

		if (enemySpecies.name == "Boss") {
			Entity boss = registry.bosses.entities[0];
			registry.bosses.remove(boss);
			registry.remove_all_components_of(boss);
			registry.bosses.clear();
			Entity player = registry.players.entities[0];
			registry.players.get(player).lake1_boss_defeated = true;
		}
		return;
	}
	else if (currHealth <= 0.f) {
		curr_battle_state = STATE_PLAYER_DEFEATED;
		return;
	}
	if (actionIndex == allMembers.size()) {
		roundCounter++;
		roundUpdate();
		curr_battle_state = STATE_ROUND_BEGIN;
	}
	else {
		//if the current ally is not last ally, decide whether player or enemy goes next
		if (currMemberIndex < allMembers.size() - 1) {
			// if next ally is faster than enemy or enemy already acted, then player turn. else, enemy turn
			if (enemy->actionIndex != actionIndex)
				currMemberIndex++;
			if (allMembers[currMemberIndex].actionIndex < enemy->actionIndex || enemyActed) {
				curr_battle_state = STATE_PLAYER_TURN;
			}
			else {
				curr_battle_state = STATE_ENEMY_TURN;
			}
		}
		// if the current ally is the last ally
		else {
			if (enemyActed) {
				curr_battle_state = STATE_PLAYER_TURN;
			}
			else {
				curr_battle_state = STATE_ENEMY_TURN;
			}
		}
	}
}

void BattleSystem::roundUpdate() {
	for (int i = 0; i < allMembers.size(); i++) {
			for (int j = 0; j < allMembers[i].currEffects.size(); j++) {
				allMembers[i].currEffects[j].effect.num_rounds--;
				CurrEffect temp = allMembers[i].currEffects[j];
				if (temp.effect.num_rounds == 0) {
					// note to restore is always -= value. The value is the amount changed, and will be negative if debuff, positive if buff
					switch (temp.effect.type) {
						case EFFECT_TYPE::BUFF_ATK:
							allMembers[i].stats.attack -= static_cast<int>(temp.value);
							break;
						case EFFECT_TYPE::DEBUFF_SPD:
							allMembers[i].stats.speed -= static_cast<int>(temp.value);
							break;
					}
					allMembers[i].currEffects.erase(allMembers[i].currEffects.begin() + j);
				}
			}
			while (!allMembers[i].followUpDmg.empty()) {
				allMembers[i].followUpDmg.pop_back();
			}
		}
	for (int i = 0; i < rodEffects.size(); i++) {
		rodEffects[i].effect.num_rounds--;
		if (rodEffects[i].effect.num_rounds == 0) {
			switch (rodEffects[i].effect.type) {
			case EFFECT_TYPE::DEBUFF_DEF:
				currDefense -= static_cast<int>(rodEffects[i].value);
				if (rodEffects[i].skill_name == "Dream Transfer")
					enemySpecies.attack += rodEffects[i].value; // since this value is negative, enemy attack will decrease here
				break;
			case EFFECT_TYPE::BUFF_DEF:
				currDefense -= static_cast<int>(rodEffects[i].value);
				break;
			}
			rodEffects.erase(rodEffects.begin() + i);
		}
	}
	for (int i = 0; i < enemy->currEffects.size(); i++) {
		enemy->currEffects[i].effect.num_rounds--;
		if (enemy->currEffects[i].effect.num_rounds == 0) {
			switch (enemy->currEffects[i].effect.type) {
				case EFFECT_TYPE::DEBUFF_ALL:
					enemySpecies.attack = static_cast<int>(enemySpecies.attack / enemy->currEffects[i].value);
					enemySpecies.defense = static_cast<int>(enemySpecies.defense / enemy->currEffects[i].value);
					enemySpecies.speed = static_cast<int>(enemySpecies.speed / enemy->currEffects[i].value);
					break;
				case EFFECT_TYPE::DEBUFF_ATK:
					enemySpecies.attack = static_cast<int>(enemySpecies.attack / enemy->currEffects[i].value);
					break;
			}
			enemy->currEffects.erase(enemy->currEffects.begin() + i);
		}
	}

	// reconstruct action order, as spd might have been changed due to buffs
	std::sort(allMembers.begin(), allMembers.end(), compareBySpd);
	int enemyIn = 0;
	enemy->actionIndex = -1;
	for (int i = 0; i < allMembers.size(); i++) {
		if (allMembers[i].stats.speed < enemySpecies.speed && enemyIn == 0) {
			enemyIn = 1;
			enemy->actionIndex = i;
			allMembers[i].actionIndex = i + 1;
		}
		else {
			allMembers[i].actionIndex = i + enemyIn;
		}
	}
	if (enemy->actionIndex == -1)
		enemy->actionIndex = allMembers.size();
	currMemberIndex = 0;
	enemyActed = false;
}

void BattleSystem::reset()
{
	curr_battle_state = STATE_ROUND_BEGIN;
	selectedSkill = -1;
	enemySelectedSkill = -1;
	selectedAnime = NONE;
	roundCounter = 1;
	durability->current = durability->max;
	currMemberIndex = -1;
	currHealth = durability->max;
	currAttack = -1;
	dmgVals = { 0, false, battle_dmg_colour, false };
	initialized = false;
	enemyActed = false;
	while (!allMembers.empty()) {
		allMembers.pop_back();
	}
	while (!rodEffects.empty()) {
		rodEffects.pop_back();
	}
	enemySpecies.health = enemyMaxHealth;
	enemy->actionIndex = 0;
	registry.enemies.clear();
	notActivated = true;
	*current_game_state = GAME_STATE_ID::WORLD;
}
