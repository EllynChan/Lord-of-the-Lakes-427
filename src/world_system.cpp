// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

// stlib
#include <cassert>
#include <sstream>
#include <iostream>

#include "physics_system.hpp"
#include "battle_system.hpp"

extern bool partyMemberOneAdded;
extern Transform viewMatrix;
bool rightKeyDown = false;
bool leftKeyDown = false;
bool upKeyDown = false;
bool downKeyDown = false;

int device_width;
int device_height;

const size_t MAX_FISHSHADOW = 3;
const size_t FISHSHADOW_DELAY_MS = 3000;

int tempSpiceID;

float show_fish_timer = 0.f;
float catch_chance_timer = 0.f;
bool is_fish_caught = false;
bool is_casting = false;
float shadow_timer_ms = 10000.f;
float shiny_spot_timer_ms = 5000.f;

Entity bgEntity;
bool second_lake_unlocked = true; // need to change to false later
bool currently_lake_1 = true;

// Create the fish world
WorldSystem::WorldSystem()
        : next_fishshadow_spawn(0.f)
        , next_fish_spawn(0.f) {
    // Seeding rng with random device
    rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem()
{
    // Destroy music components
    /*if (background_music != nullptr)
        Mix_FreeMusic(background_music);*/
    //if (start_fish_splash != nullptr)
    //    Mix_FreeChunk(start_fish_splash);
    //if (catch_fish_splash != nullptr)
    //    Mix_FreeChunk(catch_fish_splash);
    //if (rod_swing != nullptr)
    //    Mix_FreeChunk(rod_swing);
    //if (catch_alert != nullptr)
    //    Mix_FreeChunk(catch_alert);
    //Mix_CloseAudio();
    // Destroy all created components
    registry.clear_all_components();

    // Close the window
    glfwDestroyWindow(window);
}

// Debugging
namespace
{
    void glfw_err_cb(int error, const char *desc)
    {
        fprintf(stderr, "%d: %s", error, desc);
    }
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow *WorldSystem::create_window()
{
    ///////////////////////////////////////
    // Initialize GLFW
    glfwSetErrorCallback(glfw_err_cb);
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW");
        return nullptr;
    }

    //-------------------------------------------------------------------------
    // If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
    // enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
    // GLFW / OGL Initializations
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

    // Get device resolution
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    device_width = mode->width;
    device_height = mode->height;

    // subtracting from height to account for potential OS taskbars
    device_height -= 100;
    int w, h;
    // assume that monitor width > height
    w = window_width_px * device_height / window_height_px;
    h = device_height;
    // if width ends up being larger than screen width
    if (w > device_width) {
        w = device_width;
        h = window_height_px * device_width / window_width_px;
    }

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(w, h, "Lord of the Lakes", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}
    // set the window so that it always appears from the top of the screen, including title bar, and is centered
    int title_bar_height;
    glfwGetWindowFrameSize(window, NULL, &title_bar_height, NULL, NULL);
    glfwSetWindowPos(window, device_width/2 - w/2, title_bar_height);
	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow *wnd, int _0, int _1, int _2, int _3)
	{ ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow *wnd, double _0, double _1)
	{ ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_move({_0, _1}); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	auto mouse_button_callback = [](GLFWwindow *wnd, int _0, int _1, int _2)
	{ ((WorldSystem *)glfwGetWindowUserPointer(wnd))->on_mouse_button(_0, _1, _2); };
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	return window;
}

void WorldSystem::init(RenderSystem *renderer_arg, SoundSystem* sound_system_arg, GAME_STATE_ID* game_state_arg)
{
    this->renderer = renderer_arg;
    this->sound_system = sound_system_arg;
    this->current_game_state = game_state_arg;
    // Playing background music indefinitely
    //Mix_PlayMusic(background_music, -1);
    //fprintf(stderr, "Loaded music\n");
    //sound_system->playBGM(sound_system->lake_one_bgm);
    // Set all states to default
    restart_game();
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update)
{
    // Updating window title with points
    std::stringstream title_ss;
    title_ss << "Lord of the Lakes";
    glfwSetWindowTitle(window, title_ss.str().c_str());

    // Remove debug info from the last step
    while (registry.debugComponents.entities.size() > 0)
        registry.remove_all_components_of(registry.debugComponents.entities.back());

    // Processing the salmon state
    assert(registry.screenStates.components.size() <= 1);
    //ScreenState &screen = registry.screenStates.components[0];

    for (Entity entity : registry.pendingCaughtFish.entities) {
        PendingCaughtFish& pending = registry.pendingCaughtFish.get(entity);
        catch_fish(pending.species_id, true);
        registry.pendingCaughtFish.remove(entity);
    }

    float min_timer_ms = 100.f;

    for (Entity entity : registry.deathTimers.entities) {
        // progress timer
        DeathTimer& timer = registry.deathTimers.get(entity);
        timer.timer_ms -= elapsed_ms_since_last_update;
        if(timer.timer_ms < min_timer_ms){
            min_timer_ms = timer.timer_ms;
        }

        if (timer.timer_ms < 0) {
            registry.deathTimers.remove(entity);
            registry.colors.get(entity) = vec3(1, 0.8f, 0.8f);
            registry.motions.get(entity).velocity = vec2(0, 0);
            return true;
        }
    }

    if (registry.shinySpots.size() < 2) {
        if (shiny_spot_timer_ms > 0) {
            shiny_spot_timer_ms -= elapsed_ms_since_last_update;
        } else {
            // Create random shiny spots
            std::uniform_real_distribution<float> shiny_uniform_real_dist(0, 3.f);
            // Do stuff
            float num = shiny_uniform_real_dist(rng);
            if (num <= 1.f) {
                // set initial random position
                vec2 pos = vec2(uniform_dist(rng) * (window_width_px), uniform_dist(rng) * (window_height_px));
                createShinySpot(renderer, pos, { 10, 0 });
            }
            shiny_spot_timer_ms = 5000.f;
        }
    }

    for (Entity entity : registry.removeEntityTimers.entities) {
        // progress timer
        RemoveEntityTimer& timer = registry.removeEntityTimers.get(entity);

        // remove light up
        if (registry.lightUp.has(player) && !registry.collisions.has(entity)) {
            registry.lightUp.remove(player);
        }

        timer.timer_ms -= elapsed_ms_since_last_update;
        if(timer.timer_ms < min_timer_ms){
            min_timer_ms = timer.timer_ms;
        }

        if (timer.timer_ms < 0) {
            registry.remove_all_components_of(entity);
        }
    }

    Sprite& player_sprite = registry.sprites.get(player);
    // Create new random fish shadows
    if (registry.fishShadows.size() < MAX_FISHSHADOW) {
        if (shadow_timer_ms < 0.f) {
            Entity entity = createFishShadow(renderer, { 0,0 });
            // Setting random initial position and constant velocity
            Motion& motion = registry.motions.get(entity);
            motion.position =
                vec2(uniform_dist(rng) * (window_width_px),
                    uniform_dist(rng) * (window_height_px));
            motion.velocity = vec2((float)uniform_dist_int(rng) * 200.f, (float)uniform_dist_int(rng) * 100.f);
            shadow_timer_ms = 10000.f;
        } else {
            shadow_timer_ms -= elapsed_ms_since_last_update;
        }
    }

    // reduce window brightness if any of the present salmons is dying
    //screen.screen_darken_factor = 1 - min_timer_ms / 3000;

    // fishing timer indicates the amount of time spent waiting for a catch
    float fishing_min_timer_ms = 3000.f;
    for (Entity entity : registry.fishingTimers.entities)
    {
        // progress timer
        FishingTimer &timer = registry.fishingTimers.get(entity);
        timer.timer_ms -= elapsed_ms_since_last_update;
        if (timer.timer_ms < fishing_min_timer_ms)
        {
            fishing_min_timer_ms = timer.timer_ms;
        }
        //FishSpecies species = id_to_fish_species.at(timer.fish.species_id);
        // catch the fish once the timer expired
        if (timer.timer_ms < 0)
        {
            // Entity caught_fish = createFish(renderer, species);
            timer = registry.fishingTimers.get(entity);
            registry.fishingTimers.remove(entity);

            registry.renderRequests.get(exclamation).is_visible = true;
            //Mix_PlayChannel(-1, catch_alert, 0);
            sound_system->playSound(sound_system->catch_alert);
            setPositionRelativeToPlayer(registry.motions.get(exclamation), vec2(50.f, 80.f), vec2(50.f, 80.f), vec2(0.f, 90.f), vec2(0.f, 110.f));
            catch_chance_timer = 1500.f;

			// Entity caught_fish = createFish(renderer, species, static_cast<int>(lakeEntity)); // TODO: give lake an actual id
            // registry.fishingTimers.remove(entity);
        }

    }
    // catching chance timer indicates the amount of time the player has the chance to mouse click and trigger a catch
    if (catch_chance_timer < 0) {
        registry.renderRequests.get(exclamation).is_visible = false;
        setSpriteFrames(player_sprite, 0, 0, 0, 1);
        catch_chance_timer = 0;
    }
    else if (catch_chance_timer > 0) {
        catch_chance_timer -= elapsed_ms_since_last_update;
    }

    for (Entity entity : registry.catchingBars.entities) {
      // progress timer
      struct CatchingBar& cb = registry.catchingBars.get(entity);
      cb.filled -= elapsed_ms_since_last_update / 8000.f;

      if (cb.filled <= 0.1) {
          registry.renderRequests.get(catchingBar).is_visible = false;
          registry.catchingBars.remove(entity);
          setSpriteFrames(player_sprite, 0, 0, 0, 1);
      }
    }

    bool is_fishing = registry.fishingTimers.size() > 0;
    bool is_catching = registry.catchingBars.size() > 0;
    bool is_showing = show_fish_timer > 0;

    Motion& motion = registry.motions.get(player);
    if (is_fishing || is_catching || is_showing || catch_chance_timer > 0.f) {
        motion.velocity = vec2(0, 0);
    }
    if (is_fishing || catch_chance_timer > 0.f) {
        setSpriteFrames(player_sprite, 6, 6, 6, 1);
    }
    if (is_catching) {
        setSpriteFrames(player_sprite, 3, 3, 3, 1);
    }

    // when player animation finishes, start fishing timer
    if (player_sprite.current_frame == 6 && !is_fishing && catch_chance_timer == 0.f) {
        // give fishing rod position to check for collisions with shiny spots
        Motion& rodMotion = registry.motions.emplace(fishingRod);
        rodMotion.position = motion.position;
        rodMotion.velocity = vec2(0, 0);
        rodMotion.scale = vec2({ PLAYER_WIDTH, PLAYER_HEIGHT });
        // registry.collisions.emplace_with_duplicates(fishingRod, randomWaterTile); // fishing rod + water tile
        // registry.collisions.emplace_with_duplicates(randomWaterTile, fishingRod); // just test w/ any water tile for now
        registry.collisions.emplace_with_duplicates(fishingRod, randomWaterTile); // fishing rod + water tile
        registry.collisions.emplace_with_duplicates(randomWaterTile, fishingRod); // just test w/ any water tile for now
    }

    // when catching animation finishes, start showing fish
    if (player_sprite.current_frame == 3 && !is_catching && is_fish_caught) {
        show_fish_timer = 1500.f;
        setSpriteFrames(player_sprite, 7, 7, 7, 1);
            // remove motion removes the caught fish from rendering
        registry.renderRequests.get(caughtFish).is_visible = true;
        is_fish_caught = false;
    }

    // display the caught fish for 1.5 seconds
    if (show_fish_timer < 0.f && !is_fish_caught && registry.motions.has(caughtFish)) {
        // remove motion removes the caught fish from rendering
        registry.motions.remove(caughtFish);
        setSpriteFrames(player_sprite, 0, 0, 0, 1);
    }
    else {
        show_fish_timer -= elapsed_ms_since_last_update;
    }

    LakeId& lakeInfo = registry.lakes.get(player);
    // when catching animation finishes, PLAY ally1_name RECRUIT CUTSCENE
    // catch_chance_timer > 0 instead of !is_casting
    if (player_sprite.current_frame == 3 && !registry.players.get(player).ally1_recruited && !is_casting && !is_fish_caught && lakeInfo.id == 1) {
        createPartyMember(renderer, ally1_name, 15.f, 0.f, 15.f, 0.05f, 1.5f, { manifest, curse, doom, run }, TEXTURE_ASSET_ID::ALLY1_PORTRAIT, TEXTURE_ASSET_ID::ALLY1, ally1_desc);
        Dialogue& dialogue = registry.dialogues.emplace(player);
        dialogue.cutscene_id = std::string("ally_1_recruit");
        *current_game_state = GAME_STATE_ID::CUTSCENE;
        setSpriteFrames(player_sprite, 0, 0, 0, 1);
    }

    // when catching animation finishes, PLAY ALLY 2 RECRUIT CUTSCENE
    // catch_chance_timer > 0 instead of !is_casting
    if (player_sprite.current_frame == 3 && !registry.players.get(player).ally2_recruited && !is_casting && !is_fish_caught && lakeInfo.id == 2) {
        createPartyMember(renderer, ally2_name, 10.f, 1.2f, 5.f, 0.05f, 1.5f, { distract, fortress, indestructible, run }, TEXTURE_ASSET_ID::ALLY2_PORTRAIT, TEXTURE_ASSET_ID::ALLY2, ally2_desc);
        Dialogue& dialogue = registry.dialogues.emplace(player);
        dialogue.cutscene_id = std::string("ally_2_recruit");
        *current_game_state = GAME_STATE_ID::CUTSCENE;
        setSpriteFrames(player_sprite, 0, 0, 0, 1);
    }

    for (Entity entity : registry.shadowTimers.entities)
    {
        // make the fish stop moving when player is in the middle of doing some kind of fishing action
        Motion& motion = registry.motions.get(entity);
        if (is_fishing || is_catching || is_showing || is_casting) {
            motion.velocity = vec2(0, 0);
        }
        else {
            // progress timer
            ShadowTimer& timer = registry.shadowTimers.get(entity);
            timer.timer_ms -= elapsed_ms_since_last_update;
            // stop moving
            if (timer.timer_ms < FISHSHADOW_DELAY_MS) {
                motion.velocity = vec2(0, 0);
            }
            // reset timer
            if (timer.timer_ms < 0) {
                timer.timer_ms = FISHSHADOW_DELAY_MS * 2;
                motion.velocity = vec2((float)uniform_dist_int(rng) * 200.f, (float)uniform_dist_int(rng) * 100.f);
            }
        }
    }

    if (registry.durabilities.get(fishingRod).num_upgrades >= 7.f &&
        registry.attacks.get(fishingRod).num_upgrades >= 7.f && registry.players.get(player).ally1_recruited) {
        if (!registry.bosses.size() && !registry.players.get(player).lake1_boss_defeated) {
            Dialogue& dialogue = registry.dialogues.emplace(player);
            dialogue.cutscene_id = std::string("boss_appear");
            *current_game_state = GAME_STATE_ID::CUTSCENE;
            Entity entity = createBossShadow(renderer, { window_width_px - 200.f, window_height_px });
        }
    }

    // first time entering lake 2
    if (lakeInfo.id == 2 && !registry.players.get(player).lake2_entered) {
        Dialogue& dialogue = registry.dialogues.emplace(player);
        dialogue.cutscene_id = std::string("lake_2_entry");
        *current_game_state = GAME_STATE_ID::CUTSCENE;
        registry.players.get(player).lake2_entered = true;
    }
    return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game()
{
    // Debugging for memory/component leaks
    registry.list_all_components();
    printf("Restarting\n");

    current_speed = 1.f;

    // Remove all entities that we created
    // All that have a motion, we could also iterate over all fish, turtles, ... but that would be more cumbersome
    while (registry.motions.entities.size() > 0)
        registry.remove_all_components_of(registry.motions.entities.back());

    while (registry.renderRequests.entities.size() > 0)
        registry.remove_all_components_of(registry.renderRequests.entities.back());

    while (registry.fishingRods.entities.size() > 0)
        registry.remove_all_components_of(registry.fishingRods.entities.back());

    while (registry.players.entities.size() > 0)
        registry.remove_all_components_of(registry.players.entities.back());

    while (registry.partyMembers.entities.size() > 0)
        registry.remove_all_components_of(registry.partyMembers.entities.back());

    while (registry.lures.entities.size() > 0)
        registry.remove_all_components_of(registry.lures.entities.back());

    while (registry.buffs.entities.size() > 0)
        registry.remove_all_components_of(registry.buffs.entities.back());


    // Debugging for memory/component leaks
    registry.list_all_components();

    // Create BG
    bgEntity = createBG(renderer, {window_width_px / 2, window_height_px / 2});

    // Create lake
    lakeEntity = createLake(renderer, {window_width_px / 2, window_height_px / 2});

    // Get lake boundary (to prevent player from moving outside the lake)
    std::vector<TexturedVertex> lakeBoundary = renderer->lakeMesh;

    // Create random fish shadows
    for (float i = 0; i < MAX_FISHSHADOW; i++) {
        Entity entity = createFishShadow(renderer, { 0,0 });
        // Setting random initial position and constant velocity
        Motion& motion = registry.motions.get(entity);
        float rand_wid_num = uniform_dist(rng);
        float rand_hei_num = uniform_dist(rng);
        if (rand_wid_num >= 0.5 && rand_wid_num <= 0.85) {
            rand_wid_num = 0.85;
        } else if (rand_wid_num >= 0.15 && rand_wid_num < 0.5) {
            rand_wid_num = 0.15;
        }
        if (rand_hei_num >= 0.5 && rand_hei_num <= 0.85) {
            rand_hei_num = 0.85;
        }
        else if (rand_hei_num >= 0.15 && rand_hei_num < 0.5) {
            rand_hei_num = 0.15;
        }
        motion.position =
            vec2(rand_wid_num * (window_width_px),
                rand_hei_num * (window_height_px));
        motion.velocity = vec2((float)uniform_dist_int(rng) * 100.f, (float)uniform_dist_int(rng) * 50.f);
    }
    // starting values
    float posX = (float) window_width_px / 2.f;
    float posY = (float) window_height_px / 2.f;
    int gold = 0;
    float maxDur = 120.f;
    float durUpgrades = 0.f;
    int atk = 10;
    float atkUpgrades = 0.f;
    int def = 5;
    int numOwned1 = 0.f;
    int numOwned2 = 0.f;

    bool shinyTutorial = false;
    bool ally1Recruit = false;
    bool ally2Recruit = false;
    bool battleTutorialComplete = false;
    bool basicTutorialComplete = false;
    bool fishingTutorialComplete = false;
    bool lake1BossDefeated = false;
    bool lake2Entered = false;
    int numFishCaughtLake1 = 0;
    int numFishCaughtLake2 = 0;
    int lakeId = 1;

    if (should_load_save) {
        float camX = 0.f;
        float camY = 0.f;
        should_load_save = false;
        std::vector<Fish> fishes;
        std::vector<FishingLog> fishlog;
        std::vector<Lure> lures;
        std::ifstream is("../savedata.json");
        cereal::JSONInputArchive archive(is);
        archive(cereal::make_nvp("posX", posX), cereal::make_nvp("posY", posY),
                cereal::make_nvp("cameraX", camX), cereal::make_nvp("cameraY", camY),
                cereal::make_nvp("gold", gold), cereal::make_nvp("dur_max", maxDur),
                cereal::make_nvp("attack", atk), cereal::make_nvp("fishes", fishes),
                cereal::make_nvp("defense", def), cereal::make_nvp("dur_upgrades", durUpgrades),
                cereal::make_nvp("attack_upgrades", atkUpgrades), cereal::make_nvp("ally1_recruited", ally1Recruit), cereal::make_nvp("ally2_recruited", ally2Recruit),
                cereal::make_nvp("shiny_tutorial", shinyTutorial), cereal::make_nvp("num_fish_caught_lake1", numFishCaughtLake1), cereal::make_nvp("num_fish_caught_lake2", numFishCaughtLake2),
                cereal::make_nvp("fishlog", fishlog), cereal::make_nvp("currently_lake_1", currently_lake_1), cereal::make_nvp("lake_id", lakeId),
                cereal::make_nvp("second_lake_unlocked", second_lake_unlocked),
                cereal::make_nvp("battle_tutorial_complete", battleTutorialComplete),
                cereal::make_nvp("basic_tutorial_complete", basicTutorialComplete),
                cereal::make_nvp("lake1_boss_defeated", lake1BossDefeated),
                cereal::make_nvp("fishing_tutorial_complete", fishingTutorialComplete),
                cereal::make_nvp("lake2_entered", lake2Entered));

        // load camera coordinates
        viewMatrix.mat[2][0] = camX;
        viewMatrix.mat[2][1] = camY;

        // load fishes
        if (!fishes.empty()) {
            for (auto & fish : fishes) {
                FishSpecies species = id_to_fish_species.at(fish.species_id);
                createFish(renderer, species, fish.lake_id);
            }
        }

        // load fishing log
        if (!fishlog.empty()) {
            for (auto & entry : fishlog) {
                auto entity = Entity();
                FishingLog& log = registry.fishingLog.emplace(entity);
                log.species_id = entry.species_id;
                log.lake_id = entry.lake_id;
            }
        }

        // load lures
        if (lures.size() >= 2) {
            numOwned1 = lures[0].numOwned;
            numOwned2 = lures[1].numOwned;
        }

        // load lake 2, if applicable
        if (lakeId == 2) {
            RenderRequest& rr = registry.renderRequests.get(bgEntity);
            rr.used_texture = TEXTURE_ASSET_ID::BACKGROUND_2;
        }

        if (!fishingTutorialComplete) {
            *current_game_state = GAME_STATE_ID::TUTORIAL_FISH;
        }

        if (!basicTutorialComplete) {
            *current_game_state = GAME_STATE_ID::TUTORIAL_BASIC;
        }
    }

    // create lure entities
    const Lure lures[] = { { "Lure #1", 10, "Catch rate +5%", numOwned1, (int)TEXTURE_ASSET_ID::LURE_1 }, { "Lure #2", 21, "Catch rate +10%", numOwned2, (int)TEXTURE_ASSET_ID::LURE_2 } };
    createLure(renderer, lures[0], { 5.f });
    createLure(renderer, lures[1], { 10.f });

    // Create a new player
    player = createPlayer(renderer, { posX, posY }, gold, lakeId);
    Player& player_info = registry.players.get(player);
    player_info.ally1_recruited = ally1Recruit;
    player_info.ally2_recruited = ally2Recruit;
    player_info.shiny_tutorial = shinyTutorial;
    player_info.battle_tutorial_complete = battleTutorialComplete;
    player_info.lake1_boss_defeated = lake1BossDefeated;
    player_info.num_fish_caught_lake1 = numFishCaughtLake1;
    player_info.num_fish_caught_lake2 = numFishCaughtLake2;
    player_info.basic_tutorial_complete = basicTutorialComplete;
    player_info.fishing_tutorial_complete = fishingTutorialComplete;
    player_info.lake2_entered = lake2Entered;
    renderer->player = player; // Pass player entity to the renderer
    sound_system->player = player; // Pass player entity to the sound system
    //this way, catchingBar and exclamation should only have one id, instead of creating multiple ids
    catchingBar = createCatchingBar(renderer);
    exclamation = createExclamationMark(renderer);
    fishingRod = createFishingRod(renderer, maxDur, atk, def);
    registry.durabilities.get(fishingRod).num_upgrades = durUpgrades;
    registry.attacks.get(fishingRod).num_upgrades = atkUpgrades;
    randomWaterTile = createWaterTile();
    registry.colors.insert(player, {1, 0.8f, 0.8f});
    createPartyMember(renderer, mc_name, 10.f, 1.f, 10.f, 0.05f, 1.5f, { reel, release, determination, run }, TEXTURE_ASSET_ID::MC_PORTRAIT, TEXTURE_ASSET_ID::MC_PORTRAIT_1, mc_desc);
    if (ally1Recruit) {
        createPartyMember(renderer, ally1_name, 15.f, 0.f, 15.f, 0.05f, 1.5f, { manifest, curse, doom, run }, TEXTURE_ASSET_ID::ALLY1_PORTRAIT, TEXTURE_ASSET_ID::ALLY1, ally1_desc);
    }
    if (ally2Recruit) {
        createPartyMember(renderer, ally2_name, 10.f, 1.2f, 5.f, 0.05f, 1.5f, { distract, fortress, indestructible, run }, TEXTURE_ASSET_ID::ALLY2_PORTRAIT, TEXTURE_ASSET_ID::ALLY2, ally2_desc);
    }

    if (durUpgrades >= 7.f && atkUpgrades >= 7.f && ally1Recruit) {
        if (!registry.bosses.size() && !lake1BossDefeated) {
            Entity entity = createBossShadow(renderer, { window_width_px - 200.f, window_height_px });
        }
    }
}

// Compute collisions between entities
void WorldSystem::handle_collisions()
{
    // Loop over all collisions detected by the physics system
    int fishing_rod_seen = 0;
    auto &collisionsRegistry = registry.collisions;
    for (uint i = 0; i < collisionsRegistry.components.size(); i++)
    {
        // The entity and its collider
        Entity entity = collisionsRegistry.entities[i];
        Entity entity_other = collisionsRegistry.components[i].other_entity;

         // Removed the knockback effect when colliding with fish shadow.
        if (registry.players.has(entity))
        {
            if (registry.hardShells.has(entity_other) && !registry.catchingBars.entities.size() && !registry.enemies.entities.size()) {
                // TODO: texture_id is currently hardcoded, must fix
                if (!registry.players.get(player).battle_tutorial_complete) {
                    *current_game_state = GAME_STATE_ID::TUTORIAL_BATTLE_ADVANCED;
                }
                else {
                    *current_game_state = GAME_STATE_ID::TRANSITION;
                }
                std::random_device rd;
                std::mt19937 gen(rd());
                // probability of getting each enemy type is the same
                std::uniform_int_distribution<int> choiceDist(0, 2);
                FishSpecies species = possibleEnemies[choiceDist(gen)];
                if (species.name == "Turtle") {
                    createEnemy(renderer, species, { turtle_clap_clap, turtle_shell_collide, turtle_slowing_atk });
                }
                else if (species.name == "Narwhal") {
                    createEnemy(renderer, species, { narwhal_chant, narwhal_chant_intense, narwhal_dream_transfer });
                }
                else {
                    createEnemy(renderer, species, { walrus_flop_flop, walrus_flip_flop });
                }
                rightKeyDown = false;
                leftKeyDown = false;
                upKeyDown = false;
                downKeyDown = false;
                Motion& motion = registry.motions.get(player);
                motion.velocity = vec2(0, 0);
                registry.fishShadows.remove(entity_other);
                registry.remove_all_components_of(entity_other);
            }
            else if (registry.bosses.has(entity_other) && !registry.catchingBars.entities.size() && !registry.enemies.entities.size()) {
                *current_game_state = GAME_STATE_ID::TRANSITION;
                createEnemy(renderer, boss, { boss_na, boss_heavy, boss_throw_salmon, boss_execution, boss_field, boss_balance });
                rightKeyDown = false;
                leftKeyDown = false;
                upKeyDown = false;
                downKeyDown = false;
                Motion& motion = registry.motions.get(player);
                motion.velocity = vec2(0, 0);
                motion.position.x -= 100.f;
            }

            // check if player is in contact with a shiny spot
            if (registry.shinySpots.has(entity_other)) {
                if (!registry.lightUp.has(player)) {
                    registry.lightUp.emplace(player);
                }
                if (!registry.players.get(player).shiny_tutorial) {
                    Dialogue& dialogue = registry.dialogues.emplace(player);
                    dialogue.cutscene_id = std::string("shiny_spots");
                    *current_game_state = GAME_STATE_ID::CUTSCENE;
                    registry.players.get(player).shiny_tutorial = true;
                    registry.motions.get(player).velocity = { 0, 0 };
                }
            }
        }
        else if (registry.fishingRods.has(entity))
        {
            if (registry.shinySpots.has(entity_other) && fishing_rod_seen < 2) {
                // trigger fishing
                start_fishing_timer(entity, entity_other);
                // registry.remove_all_components_of(entity);
                // registry.remove_all_components_of(entity_other);
                fishing_rod_seen = 2;
            }
            // else if (registry.waterTiles.has(entity_other))
            // {
            //     // loop through all fishable items

            //     // handle fishing rod collision with water
            //     // TODO M1: random fishing should be triggered here
            //     // how to track what lake we are currently in?
            //     start_fishing_timer(entity, entity_other);
            // }
            else if (registry.hardShells.has(entity))
            {
              // handle the catching fish when the fish collides with the fishing rods

              // 1. show the progress bar
              // 2. wait fot mouse click until the progress is filled
              // createCatchingBar(renderer);
              // 3. catched the fish and jump into the battle

            }
            else {
                if (fishing_rod_seen == 0)
                    fishing_rod_seen = 1;
            }
        }
    }
    if (fishing_rod_seen == 1) {
        Entity entity = registry.fishingRods.entities[0];
        start_fishing_timer(entity, entity); // need two, only have 1, so just pass in itself
    }
    registry.motions.remove(fishingRod);

    // Remove all collisions from this simulation step
    registry.collisions.clear();
}

FishingResult WorldSystem::select_fish(Entity fishingRod, Entity waterTile)
{
    Buff buff;
    if (registry.shinySpots.has(waterTile)) {
        // should probably have a buffs compnoent
        buff = registry.buffs.get(waterTile);
    }
    if (registry.luresEquipped.has(fishingRod)) {
        // should probably have a buffs compnoent
        int lureIndex = registry.luresEquipped.get(fishingRod).lureIndex;
        if (lureIndex >= 0) {
            Entity lure = registry.lures.entities[lureIndex];
            Buff& lureBuff = registry.buffs.get(lure);
            buff.charm += lureBuff.charm;
            buff.strength += lureBuff.strength;
        }
    }

    // loop through all fish in lake
    // how to track what lake we are currently in?
    LakeId& lakeInfo = registry.lakes.get(player); // get lake player is currently in
    float min_timer_ms = -1.f;
    Fish fish; // set a dummy fish for no catch
    Lake lake = id_to_lake.at(lakeInfo.id);
    for (auto& it : lake.id_to_fishable)
    {
        int key = it.first;
        Fishable value = it.second;
        float probability = value.probability;
        if (registry.shinySpots.has(waterTile) && value.probability <= UNCOMMON_FISH) {
            // decrease chance of catching common+uncommon fish if shiny spot is present
            probability *= 1.5f;
        }

        std::uniform_real_distribution<float> fish_uniform_real_dist(0, probability * (100.f-buff.charm) / 100.f);
        // Do stuff
        float ms_until_catch = fish_uniform_real_dist(rng) * 2000.f;
        if (min_timer_ms < 0 || ms_until_catch < min_timer_ms)
        {
            min_timer_ms = ms_until_catch;
            fish.species_id = key;
            tempSpiceID = key;
            fish_rarity = value.probability; // use original probability here
        }
    };
    return { fish, min_timer_ms };
}

void WorldSystem::start_fishing_timer(Entity fishingRod, Entity waterTile)
{
    FishingResult fishing_results = select_fish(fishingRod, waterTile);
    sound_system->playSound(sound_system->start_fish_splash);
    //Mix_PlayChannel(-1, start_fish_splash, 0);
    FishingTimer& timer = registry.fishingTimers.emplace(fishingRod);
    timer.timer_ms = fishing_results.ms_until_catch;
    timer.fish = fishing_results.fish;
    is_casting = false;
}


// Should the game be over ?
bool WorldSystem::is_over() const
{
    return bool(glfwWindowShouldClose(window));
}

bool shift_is_pressed = false;

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod)
{
    // trigger "delete save data" screen by holding left-shift and delete at the same time
    if (key == GLFW_KEY_LEFT_SHIFT) {
        if (action == GLFW_PRESS) {
            shift_is_pressed = true;
        } else if (action == GLFW_RELEASE) {
            shift_is_pressed = false;
        }
    }
    if (key == GLFW_KEY_BACKSPACE) {
        if (action == GLFW_PRESS && shift_is_pressed) {
            if (*current_game_state != GAME_STATE_ID::BATTLE) {
                *current_game_state = GAME_STATE_ID::DELETE_SAVE;
                shift_is_pressed = false;
            }
        }
    }

    bool is_fishing = registry.fishingTimers.size() > 0 || registry.catchingBars.size() > 0 || show_fish_timer > 0.f || catch_chance_timer > 0.f;
    //registry.collisions.emplace_with_duplicates(fishingRod, randomWaterTile); // fishing rod + water tile
    //registry.collisions.emplace_with_duplicates(randomWaterTile, fishingRod); // just test w/ any water tile for now\
    // Test fishing
    if (key == GLFW_KEY_F && action == GLFW_PRESS && !is_fishing && *current_game_state == GAME_STATE_ID::WORLD)
    {
        // WaterTile &water = registry.waterTiles.get(player);
        // FishingRod &rod = registry.fishingRods.get(player);
        is_casting = true;
        if (registry.sprites.has(player)) {
            Sprite& player_sprite = registry.sprites.get(player);
            // complete animation is 7 frames
            setSpriteFrames(player_sprite, 0, 7, 0, 7);
        }
        sound_system->playSound(sound_system->rod_swing);

        rightKeyDown = false;
        leftKeyDown = false;
        upKeyDown = false;
        downKeyDown = false;
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS && catch_chance_timer > 0) {
        catch_chance_timer = 0.f;
        Player& player_info = registry.players.get(player);
        LakeId& lake_info = registry.lakes.get(player);
        if (fish_rarity == COMMON_FISH || (!player_info.ally1_recruited && lake_info.id == 1) || (!player_info.ally2_recruited && lake_info.id == 2)) {
            catch_fish(tempSpiceID);
        }
        // trigger the catching bar when mouse click during catch chance
        else if (!registry.catchingBars.has(catchingBar)) {
            registry.catchingBars.emplace(catchingBar);
            setPositionRelativeToPlayer(registry.motions.get(catchingBar), vec2(120.f, 20.f), vec2(120.f, 20.f), vec2(80.f, 20.f), vec2(80.f, 40.f));
        }
        registry.renderRequests.get(exclamation).is_visible = false;
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS && registry.catchingBars.size() > 0) {
        registry.renderRequests.get(catchingBar).is_visible = true;

        struct CatchingBar& cb = registry.catchingBars.get(catchingBar);
        cb.filled += 0.05f;
        cb.filled = min(cb.filled, 1.f);
        if (cb.filled == 1.f) {
            // caught the fish once the bar is filled
            catch_fish(tempSpiceID);
            registry.renderRequests.get(catchingBar).is_visible = false;
            registry.catchingBars.remove(catchingBar);
        }
    }

    // HANDLE PLAYER MOVEMENT
    Motion& motion = registry.motions.get(player);
    RenderRequest& render_request = registry.renderRequests.get(player);
    float speed = 250.f;

    if (!registry.deathTimers.has(player) && !is_fishing && *current_game_state == GAME_STATE_ID::WORLD)
    {
        if (key == GLFW_KEY_D)
        {
            if (action == GLFW_PRESS)
            {
                motion.velocity.x += speed;
                render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_RIGHT_SHEET;
                render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE_SHEET;
                rightKeyDown = true;
            }
            if (action == GLFW_RELEASE && rightKeyDown == true)
            {
                motion.velocity.x -= speed;
                rightKeyDown = false;
            }
        }
        else if (key == GLFW_KEY_A)
        {
            if (action == GLFW_PRESS)
            {
                motion.velocity.x -= speed;
                render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_LEFT_SHEET;
                render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE_SHEET;
                leftKeyDown = true;
            }
            if (action == GLFW_RELEASE && leftKeyDown == true)
            {
                motion.velocity.x += speed;
                leftKeyDown = false;
            }
        }
        else if (key == GLFW_KEY_W)
        {
            if (action == GLFW_PRESS)
            {
                motion.velocity.y -= speed;
                render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_UP_SHEET;
                render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE_SHEET;
                upKeyDown = true;
            }
            if (action == GLFW_RELEASE && upKeyDown == true)
            {
                motion.velocity.y += speed;
                upKeyDown = false;
            }
        }
        else if (key == GLFW_KEY_S)
        {
            if (action == GLFW_PRESS)
            {
                motion.velocity.y += speed;
                render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_DOWN_SHEET;
                render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE_SHEET;
                downKeyDown = true;
            }
            if (action == GLFW_RELEASE && downKeyDown == true)
            {
                motion.velocity.y -= speed;
                downKeyDown = false;
            }
        }
    }


    // Resetting game
    if (action == GLFW_RELEASE && key == GLFW_KEY_R)
    {
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        restart_game();
    }

    // Debugging
    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_RELEASE)
            debugging.in_debug_mode = false;
        else
            debugging.in_debug_mode = true;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        if (*current_game_state == GAME_STATE_ID::INVENTORY) {
            *current_game_state = GAME_STATE_ID::WORLD;
        }
        else {
            *current_game_state = GAME_STATE_ID::INVENTORY;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (*current_game_state == GAME_STATE_ID::PARTY) {
            *current_game_state = GAME_STATE_ID::WORLD;
        }
        else {
            *current_game_state = GAME_STATE_ID::PARTY;
        }
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        if (*current_game_state == GAME_STATE_ID::SHOP) {
            *current_game_state = GAME_STATE_ID::WORLD;
        }
        else {
            *current_game_state = GAME_STATE_ID::SHOP;
        }
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        // switch lures
        // get next lure with numOwned > 0
        EquipLure& equipped = registry.luresEquipped.get(fishingRod);
        for (int i=-1; i < registry.lures.size(); i++) // go through all lures + empty lure (-1)
        {
            equipped.lureIndex++;
            if (equipped.lureIndex == registry.lures.size()) {
                equipped.lureIndex = -1;
                break;
            }
            if (registry.lures.components[equipped.lureIndex].numOwned > 0) {
                break;
            }
        }
    }

    /*if (key == GLFW_KEY_B && action == GLFW_PRESS) {
        if (*current_game_state == GAME_STATE_ID::WORLD) {
            createEnemy(renderer, { 0, "The Fish", 5, 20, 100.f, {"Attack"} });
            *current_game_state = GAME_STATE_ID::BATTLE;
            rightKeyDown = false;
            leftKeyDown = false;
            upKeyDown = false;
            downKeyDown = false;
            motion.velocity = vec2(0, 0);
            *current_game_state = GAME_STATE_ID::BATTLE;
        }
        else {
            *current_game_state = GAME_STATE_ID::WORLD;
        }
    }*/


    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        Wallet& wallet = registry.wallet.get(player);
        wallet.gold += 50;
    }

    if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        // simulate fishing 10 fish
        for (int i=0; i<10; i++) {
            FishingResult fishRes = select_fish(fishingRod, randomWaterTile);
            FishSpecies species = id_to_fish_species.at(fishRes.fish.species_id);
            LakeId& lakeInfo = registry.lakes.get(player);
            caughtFish = createFish(renderer, species, lakeInfo.id);
        }
    }

    // Open tutorial page
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (*current_game_state == GAME_STATE_ID::SETTINGS) {
            *current_game_state = GAME_STATE_ID::WORLD;
        }
        else {
            *current_game_state = GAME_STATE_ID::SETTINGS;
        }
    }

    // Open teleport window (for moving between lakes)
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        if (registry.players.get(player).lake1_boss_defeated) {
            if (*current_game_state == GAME_STATE_ID::WORLD) {
                if (currently_lake_1) {
                    *current_game_state = GAME_STATE_ID::TELEPORT_2;
                } else {
                    *current_game_state = GAME_STATE_ID::TELEPORT_1;
                }
            }
        }
//        RenderRequest& rr = registry.renderRequests.get(bgEntity);
//        rr.used_texture = TEXTURE_ASSET_ID::BACKGROUND_2;
    }

    //if (key == GLFW_KEY_C && action == GLFW_PRESS) {
    //    if (*current_game_state == GAME_STATE_ID::CUTSCENE) {
    //        if (!registry.dialogues.components.empty()) {
    //            registry.dialogues.remove(player);
    //        }
    //        *current_game_state = GAME_STATE_ID::WORLD;
    //    }
    //    else {
    //        Dialogue& dialogue = registry.dialogues.emplace(player);
    //        dialogue.cutscene_id = "ally_1_recruit";
    //        *current_game_state = GAME_STATE_ID::CUTSCENE;
    //    }
    //}

    // can add this back in if we need to debug without doing the gameplay
    // Add party member
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        Player& player_info = registry.players.get(player);
        if (!player_info.ally1_recruited) {
            player_info.ally1_recruited = true;
            createPartyMember(renderer, ally2_name, 14.f, 0, 5.f, 0.05f, 1.5f, { distract, fortress, indestructible, run }, TEXTURE_ASSET_ID::ALLY2_PORTRAIT, TEXTURE_ASSET_ID::ALLY2, ally2_desc);
        }
    }
}


void WorldSystem::on_mouse_move(vec2 mouse_position)
{
	(vec2) mouse_position; // dummy to avoid compiler warning
}

void WorldSystem::on_mouse_button(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && catch_chance_timer > 0) {
        catch_chance_timer = 0.f;

        Player& player_info = registry.players.get(player);
        LakeId& lake_info = registry.lakes.get(player);
        if (fish_rarity == COMMON_FISH || (!player_info.ally1_recruited && lake_info.id == 1) || (!player_info.ally2_recruited && lake_info.id == 2)) {
            catch_fish(tempSpiceID);
        }
        // trigger the catching bar when mouse click during catch chance
        else if (!registry.catchingBars.has(catchingBar)) {
            registry.catchingBars.emplace(catchingBar);
            setPositionRelativeToPlayer(registry.motions.get(catchingBar), vec2(120.f, 20.f), vec2(120.f, 20.f), vec2(80.f, 20.f), vec2(80.f, 40.f));
        }
        registry.renderRequests.get(exclamation).is_visible = false;
    }
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && registry.catchingBars.size() > 0) {
        registry.renderRequests.get(catchingBar).is_visible = true;

		struct CatchingBar& cb = registry.catchingBars.get(catchingBar);
		cb.filled += 0.05f;
		cb.filled = min(cb.filled, 1.f);
		if (cb.filled == 1.f) {
            // caught the fish once the bar is filled
            catch_fish(tempSpiceID);
            registry.renderRequests.get(catchingBar).is_visible = false;
            registry.catchingBars.remove(catchingBar);
		}
	}

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && *current_game_state == GAME_STATE_ID::WORLD) {
        /*if (!registry.catchingBars.entities.size()) {
            vec3& color = registry.colors.get(player);
            registry.deathTimers.emplace(player);
            Mix_PlayChannel(-1, player_dead_sound, 0);
            color = vec3(1.0f, 0.0f, 0.0f);
        }*/
    }

    //int button; // dummy to avoid compiler warning
}

// set start, end, current, and number of frames for the passed in sprite
void WorldSystem::setSpriteFrames(Sprite& sprite, int start, int end, int current, int num, bool is_looping, double frame_dur) {
    sprite.start_frame = start;
    sprite.end_frame = end;
    sprite.current_frame = current;
    sprite.num_frames = num;
    sprite.loop = is_looping;
    sprite.frame_duration = frame_dur;
}

void WorldSystem::setPositionRelativeToPlayer(Motion& entity_motion, vec2 offset_left, vec2 offset_right, vec2 offset_down, vec2 offset_up) {
    TEXTURE_ASSET_ID orientation = registry.renderRequests.get(player).used_texture;
    vec2 pos = registry.motions.get(player).position;
    switch (orientation) {
    case TEXTURE_ASSET_ID::PLAYER_LEFT_SHEET:
        pos.x -= offset_left.x;
        pos.y -= offset_left.y;
        break;
    case TEXTURE_ASSET_ID::PLAYER_RIGHT_SHEET:
        pos.x += offset_right.x;
        pos.y -= offset_right.y;
        break;
    case TEXTURE_ASSET_ID::PLAYER_DOWN_SHEET:
        pos.x -= offset_down.x;
        pos.y -= offset_down.y;
        break;
    case TEXTURE_ASSET_ID::PLAYER_UP_SHEET:
        pos.x += offset_up.x;
        pos.y -= offset_up.y;
        break;
    }
    entity_motion.position = pos;
}

void WorldSystem::catch_fish(int fish_id, bool from_battle) {

    Player& player_info = registry.players.get(player);
    Sprite& player_sprite = registry.sprites.get(player);
    LakeId& lakeInfo = registry.lakes.get(player);
    // will change condition to be on a shiny spot first?
    if (!player_info.ally1_recruited && player_info.num_fish_caught_lake1 == 0 && !from_battle) {
        sound_system->playSound(sound_system->catch_fish_splash);
        setSpriteFrames(player_sprite, 0, 4, 0, 4, false, 0.12);
    }
    else if (!player_info.ally2_recruited && player_info.num_fish_caught_lake2 == 0 && !from_battle && lakeInfo.id == 2) {
        sound_system->playSound(sound_system->catch_fish_splash);
        setSpriteFrames(player_sprite, 0, 4, 0, 4, false, 0.12);
    }
    else {
        FishSpecies species = id_to_fish_species.at(fish_id);
        caughtFish = createFish(renderer, species, lakeInfo.id);
        RenderRequest& fish_render = registry.renderRequests.get(caughtFish);
        fish_render.fish_texture = (FISH_TEXTURE_ASSET_ID)fish_id;
        fish_render.is_visible = false;

        // remove lure after use
        EquipLure& equipped = registry.luresEquipped.get(fishingRod);
        if (equipped.lureIndex >= 0) {
            Lure& lure = registry.lures.components[equipped.lureIndex];
            lure.numOwned--;
            if (lure.numOwned <= 0) {
                equipped.lureIndex = -1;
            }
        }

        Motion& fish_motion = registry.motions.emplace(caughtFish);
        setPositionRelativeToPlayer(fish_motion, vec2(20.f, 95.f), vec2(25.f, 95.f), vec2(0.f, 95.f), vec2(0.f, 115.f));
        fish_motion.scale = vec2({ FISH_SPRITE_WIDTH, FISH_SPRITE_HEIGHT });

        sound_system->playSound(sound_system->catch_fish_splash);
        setSpriteFrames(player_sprite, 0, 4, 0, 4, false, 0.12);
        is_fish_caught = true;
        if (!from_battle && lakeInfo.id == 1) {
            player_info.num_fish_caught_lake1++;
        }
        else if (!from_battle && lakeInfo.id == 2) {
            player_info.num_fish_caught_lake2++;
        }

        if (species.id == boss.id) {
            Dialogue& dialogue = registry.dialogues.emplace(player);
            dialogue.cutscene_id = std::string("boss_win");
            *current_game_state = GAME_STATE_ID::CUTSCENE;
        }
    }

}