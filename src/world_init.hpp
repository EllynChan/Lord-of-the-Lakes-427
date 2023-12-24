#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"

// These are ahrd coded to the dimensions of the entity texture
const float CATCHING_WIDTH = 0.15f * 400.f;
const float CATCHING_HEIGHT = 0.4f * 350.f;
const float PLAYER_WIDTH = 0.4f * 731.f;
const float PLAYER_HEIGHT = 0.4f * 512.f;
const float FISH_BB_WIDTH = 0.4f * 296.f;
const float FISH_BB_HEIGHT = 0.4f * 165.f;
const float FISHSHADOW_BB_WIDTH = 0.4f * 140.f;
const float FISHSHADOW_BB_HEIGHT = 0.4f * 78.f;
const float SHINY_BB_WIDTH = 0.6f * 300.f;
const float SHINY_BB_HEIGHT = 0.6f * 202.f;
const float FISH_SPRITE_WIDTH = 75.f;
const float FISH_SPRITE_HEIGHT = 75.f;
const float UI_WIDTH = 0.5f* 1500.f;
const float UI_HEIGHT = 0.5f* 900.f;

// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos, int gold, int lake_id);
// the prey
Entity createFish(RenderSystem* renderer, FishSpecies& species, int lake_id);
// the enemy
Entity createFishShadow(RenderSystem* renderer, vec2 position);
Entity createShinySpot(RenderSystem* renderer, vec2 position, Buff buff);
Entity createBossShadow(RenderSystem* renderer, vec2 position);
// a red line for debugging purposes
Entity createLine(vec2 position, vec2 size);
// a pebble
Entity createPebble(vec2 pos, vec2 size);
// a lake
Entity createLake(RenderSystem* renderer, vec2 pos);

Entity createLure(RenderSystem* renderer, Lure lure, Buff buff);

Entity createFishingRod(RenderSystem* renderer, float maxDurability, int initial_attack, int initial_def);

Entity createWaterTile();

Entity createCatchingBar(RenderSystem* renderer);

Entity createBG(RenderSystem* renderer, vec2 pos);

Entity createEnemy(RenderSystem* renderer, FishSpecies species, std::vector<Skill> skills);

Entity createPartyMember(RenderSystem* renderer, std::string name, int attack, int healing_bonus, int speed, float crit_rate, float crit_value, std::vector<Skill> skills, TEXTURE_ASSET_ID battle_id, TEXTURE_ASSET_ID menu_id, std::string desc);

Entity createExclamationMark(RenderSystem* renderer);

Entity createParticle(vec2 pos, vec2 size);

Entity createSalmon(RenderSystem* renderer, vec2 pos);
