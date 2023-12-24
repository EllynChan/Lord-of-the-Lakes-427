#pragma once
#include <vector>
#include <../cereal/archives/json.hpp>
#include <../cereal/cereal.hpp>
#include <fstream>

#include "tiny_ecs.hpp"
#include "components.hpp"

class ECSRegistry
{
	// Callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registry_list;

public:
	// Manually created list of all components this game has
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<FishingTimer> fishingTimers;
	ComponentContainer<ShadowTimer> shadowTimers;
	ComponentContainer<RemoveEntityTimer> removeEntityTimers;
    ComponentContainer<FishShadow> fishShadows;
	ComponentContainer<PendingCaughtFish> pendingCaughtFish;
	ComponentContainer<ShinySpot> shinySpots;
	ComponentContainer<Motion> motions;
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<Mesh*> meshPtrs;
	ComponentContainer<RenderRequest> renderRequests;
	ComponentContainer<ScreenState> screenStates;
	ComponentContainer<SoftShell> softShells;
	ComponentContainer<HardShell> hardShells;
	ComponentContainer<DebugComponent> debugComponents;
	ComponentContainer<vec3> colors;
	ComponentContainer<LakeId> lakes;
	ComponentContainer<Wallet> wallet;
	ComponentContainer<FishingLog> fishingLog;
	ComponentContainer<Fish> fishInventory;
	ComponentContainer<Gift> giftInventory;
	ComponentContainer<Lure> lures;
	ComponentContainer<EquipLure> luresEquipped; // index of registry.lures
	ComponentContainer<Fishable> fishables;
	ComponentContainer<WaterTile> waterTiles;
	ComponentContainer<LandTile> landTiles;
	ComponentContainer<Buff> buffs;
	ComponentContainer<FishingRod> fishingRods;
	ComponentContainer<Durability> durabilities;
	ComponentContainer<Attack> attacks;
	ComponentContainer<Defense> defenses;
	ComponentContainer<Sellable> sellableItems;
	ComponentContainer<CatchingBar> catchingBars;
	ComponentContainer<Enemy> enemies;
    ComponentContainer<PartyMember> partyMembers;
    ComponentContainer<Stats> stats;
    ComponentContainer<Friendship> friendshipLevels;
	ComponentContainer<Sprite> sprites;
	ComponentContainer<Dialogue> dialogues;
	ComponentContainer<Boss> bosses;
	ComponentContainer<LightUp> lightUp;

	// constructor that adds all containers for looping over them
	// IMPORTANT: Don't forget to add any newly added containers!
	ECSRegistry()
	{
		// TODO: A1 add a LightUp component
		registry_list.push_back(&wallet);
		registry_list.push_back(&fishingLog);
		registry_list.push_back(&deathTimers);
		registry_list.push_back(&fishingTimers);
		registry_list.push_back(&shadowTimers);
		registry_list.push_back(&removeEntityTimers);
		registry_list.push_back(&pendingCaughtFish);
		registry_list.push_back(&shinySpots);
		registry_list.push_back(&buffs);
		registry_list.push_back(&fishShadows);
		registry_list.push_back(&motions);
		registry_list.push_back(&collisions);
		registry_list.push_back(&players);
		registry_list.push_back(&meshPtrs);
		registry_list.push_back(&renderRequests);
		registry_list.push_back(&screenStates);
		registry_list.push_back(&softShells);
		registry_list.push_back(&hardShells);
		registry_list.push_back(&debugComponents);
		registry_list.push_back(&colors);
		registry_list.push_back(&lakes);
		registry_list.push_back(&fishInventory);
		registry_list.push_back(&lures);
		registry_list.push_back(&luresEquipped);
		registry_list.push_back(&fishables);
		registry_list.push_back(&waterTiles);
		registry_list.push_back(&landTiles);
		registry_list.push_back(&fishingRods);
		registry_list.push_back(&durabilities);
		registry_list.push_back(&attacks);
		registry_list.push_back(&defenses);
		registry_list.push_back(&sellableItems);
		registry_list.push_back(&catchingBars);
		registry_list.push_back(&enemies);
		registry_list.push_back(&partyMembers);
		registry_list.push_back(&sprites);
		registry_list.push_back(&dialogues);
		registry_list.push_back(&bosses);
		registry_list.push_back(&lightUp);
	}

	void clear_all_components() {
		for (ContainerInterface* reg : registry_list)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registry_list)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(Entity e) {
		for (ContainerInterface* reg : registry_list)
			reg->remove(e);
	}

};

extern ECSRegistry registry;