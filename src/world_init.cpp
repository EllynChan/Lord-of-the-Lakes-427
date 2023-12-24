#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

/**
 * @brief Fishing system
 * handle collide between fishing rod and water tile
 * On collide, use random number generator to determine which fish, + how long it takes to catch
 * wait until timer goes to zero; if player moves, cancel fishing
 * reeling: currently just always successfully catch, add one to fishing count, this can be adjusted later
 */

Entity createPlayer(RenderSystem* renderer, vec2 pos, int gold, int lake_id)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE_SHEET);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting initial motion values
    Motion& motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = vec2({ PLAYER_WIDTH, PLAYER_HEIGHT });

    // Create and (empty) Salmon component to be able to refer to all turtles
    registry.players.emplace(entity);

    LakeId& lakeInfo = registry.lakes.emplace(entity);
    lakeInfo.id = lake_id;

	Wallet& player_wallet = registry.wallet.emplace(entity);
    player_wallet.gold = gold;
    Sprite& sprite = registry.sprites.emplace(entity);
    sprite.rows = 1;
    sprite.columns = 8;
    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::PLAYER_LEFT_SHEET,
              EFFECT_ASSET_ID::TEXTURED,
              GEOMETRY_BUFFER_ID::SPRITE_SHEET });

    return entity;
}

Entity createFish(RenderSystem* renderer, FishSpecies& species, int lake_id)
{
    // Reserve en entity
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Initialize in inventory, make sellable
    Sellable& sellable = registry.sellableItems.emplace(entity);
    sellable.price = (float) species.price;

    // Create an (empty) Fish component to be able to refer to all fish
    Fish& fish = registry.fishInventory.emplace(entity);
    fish.species_id = species.id;
	fish.lake_id = lake_id;

	FishingLog& log = registry.fishingLog.emplace(entity);
	log.species_id = species.id;
	log.lake_id = lake_id;

    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::TEXTURE_COUNT,
              EFFECT_ASSET_ID::TEXTURED,
              GEOMETRY_BUFFER_ID::SPRITE });

    return entity;
}

Entity createLure(RenderSystem* renderer, Lure givenLure, Buff buff)
{
    // Reserve en entity
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object

    // // Initialize in inventory, make sellable
    // Sellable& sellable = registry.sellableItems.emplace(entity);
    // sellable.price = (float) lureType.price;

    // Create an (empty) Lure component to be able to refer to all lures
    registry.lures.insert(entity, givenLure);
    registry.buffs.insert(entity, buff);

    return entity;
}

Entity createFishShadow(RenderSystem* renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // For keeping track of fish shadows
    registry.fishShadows.emplace(entity);

    // Initialize the motion
    auto& motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.position = position;

    // Setting initial values,
    motion.scale = vec2({ FISHSHADOW_BB_WIDTH, FISHSHADOW_BB_HEIGHT });

    // Create and (empty) FishShadow component
    registry.hardShells.emplace(entity);
    registry.shadowTimers.emplace(entity);
    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::FISH_SHADOW,
              EFFECT_ASSET_ID::TEXTURED,
              GEOMETRY_BUFFER_ID::SPRITE });

    return entity;
}

Entity createBossShadow(RenderSystem* renderer, vec2 position)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Initialize the motion
    auto& motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.position = position;

    // Setting initial values,
    motion.scale = vec2({ FISHSHADOW_BB_WIDTH * 1.5f, FISHSHADOW_BB_HEIGHT * 1.5f });

    // Create and (empty) FishShadow component
    registry.bosses.emplace(entity);
    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::BOSS_SHADOW,
          EFFECT_ASSET_ID::TEXTURED,
          GEOMETRY_BUFFER_ID::SPRITE });

    return entity;
}

Entity createShinySpot(RenderSystem* renderer, vec2 position, Buff buff)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    // For keeping track of shiny spots
    ShinySpot &shiny = registry.shinySpots.emplace(entity);
    // Initialize the motion
    auto& motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.position = position;

    // Setting initial values,
    motion.scale = vec2({ SHINY_BB_WIDTH, SHINY_BB_HEIGHT });

    registry.buffs.insert(entity, buff);

    Sprite& sprite = registry.sprites.emplace(entity);
    sprite.rows = 1;
    sprite.columns = 3;
    sprite.end_frame = 3;
    sprite.num_frames = 3;
    sprite.frame_duration = 0.2f;
    sprite.loop = true;

    RemoveEntityTimer& removeEntityTimer = registry.removeEntityTimers.emplace(entity);
    removeEntityTimer.timer_ms = 20000.f;

    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::SHINY_SPOT,
              EFFECT_ASSET_ID::TEXTURED,
              GEOMETRY_BUFFER_ID::SPRITE_SHEET });

    return entity;
}

Entity createLine(vec2 position, vec2 scale)
{
    Entity entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::TEXTURE_COUNT,
              EFFECT_ASSET_ID::PEBBLE,
              GEOMETRY_BUFFER_ID::DEBUG_LINE });

    // Create motion
    Motion& motion = registry.motions.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.position = position;
    motion.scale = scale;

    registry.debugComponents.emplace(entity);
    return entity;
}

Entity createWaterTile()
{
    // by default follows probabilities for lake its in, but additional properties can be added to override lake defaults
    auto entity = Entity();

    // Setting initial values

    // Create and (empty) Salmon component to be able to refer to all turtles
    registry.waterTiles.emplace(entity);
    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
              EFFECT_ASSET_ID::EFFECT_COUNT, // TODO M1: add effects + geometry
              GEOMETRY_BUFFER_ID::GEOMETRY_COUNT });

    return entity;
}


Entity createLake(RenderSystem* renderer, vec2 pos)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::LAKE);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting initial motion values
    Motion& motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };

//    registry.renderRequests.insert(
//            entity,
//            { TEXTURE_ASSET_ID::LAKE,
//              EFFECT_ASSET_ID::TEXTURED,
//              GEOMETRY_BUFFER_ID::LAKE });

    return entity;
}


Entity createFishingRod(RenderSystem* renderer, float maxDurability, int initial_attack, int initial_def)
{
    // by default follows probabilities for lake its in, but additional properties can be added to override lake defaults
    auto entity = Entity();

    // Setting initial values
    Durability& durability = registry.durabilities.emplace(entity);
    durability.max = maxDurability;
    durability.current = maxDurability;

    Attack& attack = registry.attacks.emplace(entity);
    attack.damage = initial_attack;
    // also can have a special bait, but default has no/default bait

    Defense& defense = registry.defenses.emplace(entity);
    defense.value = initial_def;

    registry.luresEquipped.emplace(entity);
    // Create and (empty) FishingRod component to be able to refer to all turtles
    registry.fishingRods.emplace(entity);
    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
              EFFECT_ASSET_ID::EFFECT_COUNT, // TODO M1: add effects + geometry
              GEOMETRY_BUFFER_ID::GEOMETRY_COUNT });

    return entity;
}

Entity createBG(RenderSystem* renderer, vec2 pos)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::BACKGROUND);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting initial motion values
    Motion& motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = { 1.f, 1.f };

    registry.renderRequests.insert(
            entity,
            { TEXTURE_ASSET_ID::BACKGROUND,
              EFFECT_ASSET_ID::TEXTURED,
              GEOMETRY_BUFFER_ID::BACKGROUND });

    return entity;
}

Entity createCatchingBar(RenderSystem* renderer)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);

	motion.scale = vec2({ CATCHING_WIDTH, CATCHING_HEIGHT });

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::CATCHING_BAR,
			EFFECT_ASSET_ID::CATCHING_BAR,
			GEOMETRY_BUFFER_ID::SPRITE,
            false});

	return entity;
}

Entity createEnemy(RenderSystem* renderer, FishSpecies species, std::vector<Skill> skills)
{
    // can attach sprite here, or alternatively let imgui handle separately
    auto entity = Entity();

    // Setting initial values
    Enemy& enemy = registry.enemies.emplace(entity);
    enemy.species = species;
    enemy.skills = skills;
    // also can have a special bait, but default has no/default bait
    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
          EFFECT_ASSET_ID::EFFECT_COUNT, // TODO M1: add effects + geometry
          GEOMETRY_BUFFER_ID::GEOMETRY_COUNT });

    return entity;
}

Entity createPartyMember(RenderSystem* renderer, std::string name, int attack, int healing_bonus, int speed, float crit_rate, float crit_value, std::vector<Skill> skills, TEXTURE_ASSET_ID battle_id, TEXTURE_ASSET_ID menu_id, std::string desc) {
    auto entity = Entity();

    PartyMember& partyMember = registry.partyMembers.emplace(entity);
    partyMember.name = name;
    partyMember.description = desc;
    partyMember.stats = { attack, healing_bonus, speed, crit_rate, crit_value };
    partyMember.skills = skills;
    partyMember.texture_id = battle_id;
    partyMember.menu_texture_id = menu_id;

    Stats& stats = registry.stats.emplace(entity);
    stats.attack = attack;
    stats.healing_bonus = healing_bonus;
    stats.speed = speed;
    stats.critical_rate = crit_rate;
    stats.critical_value = crit_value;

    registry.friendshipLevels.emplace(entity);

    return entity;
}

Entity createExclamationMark(RenderSystem* renderer) {
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(entity, &mesh);

    Entity player = registry.players.entities[0];
    // Setting initial motion values
    Motion& motion = registry.motions.emplace(entity);

    motion.scale = vec2({ 50.f, 50.f });
    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::EXCLAMATION,
          EFFECT_ASSET_ID::TEXTURED,
          GEOMETRY_BUFFER_ID::SPRITE,
          false});


    return entity;
}

Entity createSalmon(RenderSystem* renderer, vec2 pos)
{
    auto entity = Entity();

    // Store a reference to the potentially re-used mesh object
    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SALMON);
    registry.meshPtrs.emplace(entity, &mesh);

    // Setting initial motion values
    Motion& motion = registry.motions.emplace(entity);
    motion.position = pos;
    motion.angle = 0.f;
    motion.velocity = { 0.f, 0.f };
    motion.scale = mesh.original_size * 150.f;
    motion.scale.x *= -1; // point front to the right

    // Create and (empty) Salmon component to be able to refer to all turtles
    registry.players.emplace(entity);
    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no txture is needed
            EFFECT_ASSET_ID::EFFECT_COUNT,
            GEOMETRY_BUFFER_ID::SALMON });

    return entity;
}
