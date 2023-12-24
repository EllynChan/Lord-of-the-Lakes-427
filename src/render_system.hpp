#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"
#include "battle_system.hpp"
#include "sound_system.hpp"

#include "../imgui/imgui.h"
#include <../nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
    /**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
    std::array<GLuint, texture_count> texture_gl_handles;
    std::array<ivec2, texture_count> texture_dimensions;
    std::array<GLuint, texture_count> fish_texture_gl_handles;
    std::array<ivec2, texture_count> fish_texture_dimensions;

    // no need for this if we aren't using any mesh
    const std::vector < std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
            {
                std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::SALMON, mesh_path("salmon.obj")),
            };

    // Make sure these paths remain in sync with the associated enumerators.
    const std::array<std::string, texture_count> texture_paths = {
            textures_path("fish.png"),
            textures_path("fish_shadow.png"),
            textures_path("lightBlueBG.png"),
            textures_path("lightGreenBG.png"),
            textures_path("player_left.png"),
            textures_path("player_right.png"),
            textures_path("player_back.png"),
            textures_path("player_front.png"),
            textures_path("tutorial-main.png"),
            textures_path("land.png"),
            textures_path("mainUI.png"),
            textures_path("menu_background.png"),
            textures_path("chinook.png"),
            textures_path("item_cell.png"),
            textures_path("chinook_shadow.png"),
            textures_path("shopcat.png"),
            textures_path("catching_bar.png"),
            textures_path("battleUIMain.png"),
            textures_path("mc1.png"),
            textures_path("mc2.png"),
            textures_path("enemy.png"),
            textures_path("tutorial-basic.png"),
            textures_path("tutorial-fishing.png"),
            textures_path("tutorial-battle.png"),
            textures_path("party-member-basic.png"),
            textures_path("party-member-detail.png"),
            textures_path("rod_idle.png"),
            textures_path("player_left_sheet.png"),
            textures_path("player_right_sheet.png"),
            textures_path("player_up_sheet.png"),
            textures_path("player_down_sheet.png"),
            textures_path("rod_reel.png"),
            textures_path("on_hit.png"),
            textures_path("exclamation_mark.png"),
            textures_path("settings.png"),
            textures_path("save.png"),
            textures_path("save_complete.png"),
            textures_path("load.png"),
            textures_path("delete_save.png"),
            textures_path("delete_save_complete.png"),
            textures_path("load_fail.png"),
            textures_path("rod_release.png"),
            textures_path("rod_on_hit.png"),
            textures_path("heal_effect.png"),
            textures_path("atk_buff_effect.png"),
            textures_path("rod_defeated.png"),
            textures_path("mc_portrait.png"),
            textures_path("spd_buff_effect.png"),
            textures_path("transition.png"),
            textures_path("ally1_portraits.png"),
            textures_path("MC_talk_sheet.png"),
            textures_path("ally1_talk_sheet.png"),
            textures_path("ally1.png"),
            textures_path("arrow.png"),
            textures_path("def_buff_effect.png"),
            textures_path("shiny_spot_sheet.png"),
            textures_path("particle.png"),
            textures_path("doom.png"),
            textures_path("curse.png"),
            textures_path("ally2_portraits.png"),
            textures_path("ally2.png"),
            textures_path("item_cell_selected.png"),
            textures_path("battle_tutorial1.png"),
            textures_path("battle_tutorial2.png"),
            textures_path("battle_tutorial3.png"),
            textures_path("battle_tutorial4.png"),
            textures_path("battle_tutorial5.png"),
            textures_path("battle_tutorial6.png"),
            textures_path("battle_tutorial7.png"),
            textures_path("sleepy_narwhal.png"),
            textures_path("turtle1.png"),
            textures_path("walrus.png"),
            textures_path("title.png"),
            textures_path("ally1_recruit.png"),
            textures_path("land2.png"),
            textures_path("teleport1.png"),
            textures_path("teleport2.png"),
            textures_path("legend_fish_bg.png"),
            textures_path("mc_intro.png"),
            textures_path("shiny_tutorial.png"),
            textures_path("ally2_talk_sheet.png"),
            textures_path("ally2_recruit.png"),
            textures_path("boss_shadow.png"),
            textures_path("execution.png"),
            textures_path("whale2.png"),
            textures_path("lure1.png"),
            textures_path("lure2.png"),
            textures_path("boss_popup.png"),
            textures_path("boss1_defeat.png"), };

        const std::array<std::string, texture_count> fish_texture_paths = {
            fish_path("cracker_fish.png"),
            fish_path("little_blue.png"), // TODO: image for carp needed
            fish_path("chinook.png"),
            fish_path("cucumber_fish.png"),
            fish_path("hammer_fish.png"),
            fish_path("sharpie_fish.png"),
            fish_path("sweet_potato_fish.png"),
            fish_path("teal_fish.png"),
            fish_path("upwards_fish.png"),
            fish_path("enemy.png"),
            fish_path("angel_fish.png"),
            fish_path("bandages.png"),
            fish_path("cheese_fish.png"),
            fish_path("toddler_fish.png"),
            fish_path("jack.png"),
            fish_path("squid.png"),
            fish_path("whale2.png"),
            fish_path("sleepy_narwhal.png"),
            fish_path("turtle1.png"),
            fish_path("walrus.png"),
            fish_path("wrinkly_coral.png"),
            fish_path("yam_roll.png"),
            fish_path("seal_fish.png"),
            fish_path("taco_ray.png"),
            fish_path("blue_ray.png"),
            fish_path("baby_flock.png"),
            fish_path("crab.png"), };


    std::array<GLuint, effect_count> effects;
    // Make sure these paths remain in sync with the associated enumerators.
    const std::array<std::string, effect_count> effect_paths = {
            shader_path("coloured"),
            shader_path("pebble"),
            shader_path("salmon"),
            shader_path("textured"),
            shader_path("water"),
            shader_path("catching"),
            shader_path("effect"),
            shader_path("particle"),
            shader_path("salmon"),
    };

    std::array<GLuint, geometry_count> vertex_buffers;
    std::array<GLuint, geometry_count> index_buffers;
    std::array<Mesh, geometry_count> meshes;

public:
    std::vector<TexturedVertex> lakeMesh; // For creating lake
    std::vector<vec2> lakeEdges; // For creating lake edges

    bool should_open_tutorial = false; // For checking if tutorial screen should be opened

    // Initialize the window
    bool init(GLFWwindow* window, GAME_STATE_ID* current_game_state, BattleSystem* battle_system, SoundSystem* sound_system);

    template <class T>
    void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

    void initializeGlTextures();

    void initializeGlEffects();

    void initializeGlMeshes();
    Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

    void initializeGlGeometryBuffers();
    // Initialize the screen texture used as intermediate render target
    // The draw loop first renders to this texture, then it is used for the water
    // shader
    bool initScreenTexture();

    // Destroy resources associated to one or all entities created by the system
    ~RenderSystem();

    void initFonts();
    // feel free to add or change the sizes. you can use ImGui::PushFont(<variable name>) to use specific fonts (and don't forget to ImGui::PopFont())
    ImFont* font_default; // 20px
    ImFont* font_small; // 16px
    ImFont* font_large; // 32px
    ImFont* font_effect; // for the damage numbers

    // storing the json dialogue objext
    json dialogue;
    json popup;
    // Draw all entities
    void draw();

    mat3 createProjectionMatrix();
    //mat3 createFixedProjectionMatrix();

    Entity player;
    bool should_restart_game = false;

    float scale_x;
    float scale_y;

    std::vector<Particle> particles;

private:
    void saveData();
    //Dear ImGui functions
    void createMainUIButtonWindow(GAME_STATE_ID id, const ImVec2& position);
    void drawMenuItems();
    void drawMenu();
    void drawInventory();
    void drawShop();
    void drawParty();
    void drawMap();
    void drawSettingsMain(const char* id, TEXTURE_ASSET_ID tex_id);
    void drawTutorialScreen(const char* id, const int texture_id);
    void createTutorialButtonWindow(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state);
    void createTutorialExitButton(const char* id, const char* button_id, const ImVec2& position);
    void createTutorialBackButton(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state);
    void createSettingsButtonWindow(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state);
    void createSaveButtons(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state);
    void createBattleTutorialButton(const ImVec2& position);
    void setDrawCursorScreenPos(ImVec2 offset);
    void drawCharacterDetail(int selectedCharacter);
    void drawPartyListPopup(int selectedCharacter);
    void drawExitButton();
    void drawDialogueCutscene();
    void drawStartMenu();
	// Internal drawing functions for each entity type
	void drawTexturedMesh(Entity entity, const mat3& projection);
	void drawBattle();
	void drawToScreen();
	void createSkillsTable();
	void createBattleLog();
	void createCharacterPortrait(ImVec2 position, int index);
	void createHPBar();
	void createEnemy();
	void initializeGlTexturesFromPaths(std::array<std::string, texture_count> texture_paths,
        std::array<GLuint, texture_count>& texture_gl_handles,
        std::array<ivec2, texture_count>& texture_dimension);
	void drawOnMenu();
	void drawSpriteAnime(int frame, int num_rows, int num_columns, TEXTURE_ASSET_ID texture_asset_id, vec2 position, vec2 scale, float darken = 0.f, bool use_screen_matrix = true);
    void playEnemyAnime(BattleSystem::AnimeEnum i);
    void playEffect(BattleSystem::AnimeEnum i);
    void drawMeshEffect(GEOMETRY_BUFFER_ID geo_id, EFFECT_ASSET_ID eff_id, vec4 c, vec2 pos, float angle, vec2 scale);
    void drawSpriteEffect(TEXTURE_ASSET_ID id, EFFECT_ASSET_ID eff_id, vec4 c, vec2 pos, vec2 scale);
    void drawPlayerAnime();
    void drawRoundBanner();
    void drawPortraitsAnime(float xpos, TEXTURE_ASSET_ID asset_id);
    void drawDmgText(const char* text, ImVec2 position, ImU32 textColor);

    std::vector<RenderRequestsNonEntity> renderRequestsNonEntity;

    // Window handle
    GLFWwindow* window;


    // Current game state
    GAME_STATE_ID* current_game_state;

    //RenderMenu* menu_renderer;
    // Screen texture handles
    GLuint frame_buffer;
    GLuint off_screen_render_buffer_color;
    GLuint off_screen_render_buffer_depth;

    Entity screen_state_entity;
    BattleSystem* battle_system;
    SoundSystem* sound_system;
};

bool loadEffectFromFile(
        const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
