// internal
#include "render_system.hpp"
#include <SDL.h>
#include <iostream>
#include <map>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#include "tiny_ecs_registry.hpp"
#include "world_init.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

extern bool currently_lake_1;
extern bool second_lake_unlocked;
extern Entity bgEntity;

extern Transform viewMatrix;
bool show_inventory = true;
bool show_fishing_log = false;
bool inventory_button_active = true;
bool fishing_log_button_active = false;
bool show_character_detail = false;
bool show_party_list = false;
// int for knowing which character to show for the character detail
// need to know how party members are id'd for later implementation
int selected_character_detail = 0;
int selected_character_add = 0;

// using this to track selected skill, assuming max num skill is 4
bool skillSelected[4] = { true, false, false, false };
//animation vars
float lastTimeEnemy = 0.0f; // for enemy animating time
float lastTimePlayer = 0.0f; // for player animating time
float lastTimeEffect = 0.0f; // for effect animating time
float lastTimeBanner = 0.0f;
float lastTimeText = 0.0f;
float lastTimeTransition = 0.0f;
int buffIndex = 0;
float hpFill = 0.0f;
float hpFillEnemy = 0.0f;
char hpText[32];
char hpTextEnemy[32];
std::string battleLogText;
int battle_tutorial_index = 0;

bool partyMemberOneAdded = false;

float lastTime = 0;
vec2 translations[100];
// shop enum for dialogue
enum class SHOP_STATE
{
    WELCOME = 0,
    BUY = WELCOME + 1,
    SELL = BUY + 1,
    BUY_NO_MONEY = SELL + 1
};

SHOP_STATE shop_state = SHOP_STATE::WELCOME;

// https://stackoverflow.com/questions/64653747/how-to-center-align-text-horizontally
void textCentered(std::string text) {
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
    ImGui::SetCursorPosX(10.f);
    if (textWidth < windowWidth) {
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    }

    ImGui::TextWrapped(text.c_str());
}

void RenderSystem::drawTexturedMesh(Entity entity,
                                    const mat3 &projection)
{
    Motion &motion = registry.motions.get(entity);
    // Transformation code, see Rendering and Transformation in the template
    // specification for more info Incrementally updates transformation matrix,
    // thus ORDER IS IMPORTANT
    Transform transform;
    transform.translate(motion.position);
    transform.scale(motion.scale);


    assert(registry.renderRequests.has(entity));
    const RenderRequest &render_request = registry.renderRequests.get(entity);

    const GLuint used_effect_enum = (GLuint)render_request.used_effect;
    assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
    const GLuint program = (GLuint)effects[used_effect_enum];

    // Setting shaders
    glUseProgram(program);
    gl_has_errors();

    assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
    const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
    const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    // Input data location as in the vertex buffer
    if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED)
    {
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
        gl_has_errors();
        assert(in_texcoord_loc >= 0);

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
            sizeof(TexturedVertex), (void*)0);
        gl_has_errors();

        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(
            in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
            (void*)sizeof(
                vec3)); // note the stride to skip the preceeding vertex position

        // Enabling and binding texture to slot 0
        glActiveTexture(GL_TEXTURE0);
        gl_has_errors();

        assert(registry.renderRequests.has(entity));

        GLuint texture_id;
        if (render_request.fish_texture != FISH_TEXTURE_ASSET_ID::FISH_TEXTURE_COUNT) {
            texture_id =
                fish_texture_gl_handles[(GLuint)registry.renderRequests.get(entity).fish_texture];
        }
        else {
            texture_id =
                texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);
        gl_has_errors();
    }
    else if (render_request.used_effect == EFFECT_ASSET_ID::PLAYER)
    {
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        GLint in_color_loc = glGetAttribLocation(program, "in_color");
        gl_has_errors();

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                              sizeof(ColoredVertex), (void *)0);
        gl_has_errors();

        glEnableVertexAttribArray(in_color_loc);
        glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
                              sizeof(ColoredVertex), (void *)sizeof(vec3));
        gl_has_errors();
    }
    else if (render_request.used_effect == EFFECT_ASSET_ID::CATCHING_BAR)
    {
      GLint in_position_loc = glGetAttribLocation(program, "in_position");
      GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
      gl_has_errors();
      assert(in_texcoord_loc >= 0);

      glEnableVertexAttribArray(in_position_loc);
      glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                  sizeof(TexturedVertex), (void *)0);
      gl_has_errors();

      glEnableVertexAttribArray(in_texcoord_loc);
      glVertexAttribPointer(
        in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
        (void *)sizeof(
          vec3)); // note the stride to skip the preceeding vertex position

      // Enabling and binding texture to slot 0
      glActiveTexture(GL_TEXTURE0);
      gl_has_errors();

      assert(registry.renderRequests.has(entity));
      GLuint texture_id =
        texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];

      glBindTexture(GL_TEXTURE_2D, texture_id);
      gl_has_errors();

      GLint uFilled_uloc = glGetUniformLocation(program, "uFilled");
      const float filled = registry.catchingBars.get(registry.catchingBars.entities[0]).filled;
      glUniform1f(uFilled_uloc, (float) filled);
      gl_has_errors();
    }
    else
    {
        assert(false && "Type of render request not supported");
    }
    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(program, "fcolor");
    const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
    glUniform3fv(color_uloc, 1, (float *)&color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();

    GLsizei num_indices = size / sizeof(uint16_t);
    // GLsizei num_triangles = num_indices / 3;

    GLint currProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
    // Setting uniform values to the currently bound program
    GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
    GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);

    // glm::mat3 viewMatrix = glm::mat3(1.0f);
    GLuint view_loc = glGetUniformLocation(currProgram, "view");
    glUniformMatrix3fv(view_loc, 1, GL_FALSE, (float *)&viewMatrix);
    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();
}



// draw the intermediate texture to the screen
void RenderSystem::drawToScreen()
{
    // Setting shaders
    // get the water texture, sprite mesh, and program - the "water" is our world screen
    glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::WATER]);
    gl_has_errors();
    // Clearing backbuffer
    int w, h;
    glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, w, h);

    glDepthRange(0, 10);
    glClearColor(1.f, 0, 0, 1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    gl_has_errors();
    // Enabling alpha channel for textures
    glDisable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Draw the screen texture on the quad geometry
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
    // indices to the bound GL_ARRAY_BUFFER
    gl_has_errors();
    const GLuint water_program = effects[(GLuint)EFFECT_ASSET_ID::WATER];
    GLuint screen_darken_factor = glGetUniformLocation(water_program, "screen_darken_factor");
    ScreenState &screen = registry.screenStates.get(screen_state_entity);
    // darken the screen behind anything that cutscene will render
    if (*current_game_state == GAME_STATE_ID::CUTSCENE) {
        screen.screen_darken_factor = 0.4f;
    }
    else {
        screen.screen_darken_factor = 0.f;
    }

    glUniform1f(screen_darken_factor, screen.screen_darken_factor);
    gl_has_errors();
    // Set the vertex position and vertex texture coordinates (both stored in the
    // same VBO)
    GLint in_position_loc = glGetAttribLocation(water_program, "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
    gl_has_errors();

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
    gl_has_errors();
    // Draw
    glDrawElements(
            GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
            nullptr); // one triangle = 3 vertices; nullptr indicates that there is
    // no offset from the bound index buffer
    gl_has_errors();

    //reset the popup
    if (*current_game_state != GAME_STATE_ID::PARTY && show_party_list == true) {
        show_party_list = false;
    }

    if (*current_game_state != GAME_STATE_ID::SHOP && shop_state != SHOP_STATE::WELCOME) {
        shop_state = SHOP_STATE::WELCOME;
    }

    // This is the UI that always appears in the World View and the Menu View. It should not appear during Battle View though.
    float step = 35.f;
    switch (*current_game_state)
    {
        case GAME_STATE_ID::START_MENU: {
            drawStartMenu();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> xDistribution(-20.0f, 20.0f);
            std::uniform_real_distribution<float> yDistribution(-10.0f, 10.0f);
            int index = 0;
            float offset = 0.1f;
            if (glfwGetTime() - lastTime > 0.5f) {
                for (int y = 0; y < 10; ++y) {
                    for (int x = 0; x < 10; ++x) {
                        glm::vec2 translation;
                        translation.x = xDistribution(gen) * 0.05f;
                        translation.y = yDistribution(gen) * 0.05f - 1.f;
                        translations[index++] = translation;
                    }
                }
                lastTime = glfwGetTime();
            }
            
            mat3 projection_2D = createProjectionMatrix();
            Transform transform;

            transform.translate({ window_width_px / 2, window_height_px / 2 });
            transform.rotate(0);
            transform.scale(vec2(10.f));
            const GLuint used_effect_enum = (GLuint)EFFECT_ASSET_ID::PARTICLE;
            const GLuint program = (GLuint)effects[used_effect_enum];

            // Setting shaders
            glUseProgram(program);
            gl_has_errors();

            const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::PEBBLE];
            const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::PEBBLE];

            // Setting vertex and index buffers
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            gl_has_errors();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            gl_has_errors();

            GLint in_position_loc = glGetAttribLocation(program, "in_position");
            GLint in_color_loc = glGetAttribLocation(program, "in_color");
            gl_has_errors();

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glEnableVertexAttribArray(in_position_loc);
            glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                sizeof(ColoredVertex), (void*)0);
            gl_has_errors();

            glEnableVertexAttribArray(in_color_loc);
            glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
                sizeof(ColoredVertex), (void*)sizeof(vec3));
            gl_has_errors();

            // Getting uniform locations for glUniform* calls
            GLint color_uloc = glGetUniformLocation(program, "fcolor");
            const vec4 color = vec4(1.f);
            glUniform4fv(color_uloc, 1, (float*)&color);
            gl_has_errors();

            // Get number of indices from index buffer, which has elements uint16_t
            GLint size = 0;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
            gl_has_errors();

            GLsizei num_indices = size / sizeof(uint16_t);
            //GLsizei num_triangles = num_indices / 3;

            GLint currProgram;
            glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
            // Setting uniform values to the currently bound program
            
            GLint offsets_loc = glGetUniformLocation(currProgram, "offsets");
            glEnableVertexAttribArray(offsets_loc);
            glVertexAttribPointer(offsets_loc, 2, GL_FLOAT, GL_FALSE,
                sizeof(ColoredVertex), (void*)0);
            glUniform2fv(offsets_loc, 100, (float*)translations);
            GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
            glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
            GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
            glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection_2D);
            gl_has_errors();
            // Drawing of num_indices/3 triangles specified in the index buffer
            glm::mat3 viewScreenMatrix = glm::mat3(1.0f);
            GLuint view_loc = glGetUniformLocation(currProgram, "view");
            // rod idle depends on screen, not camera position
            glUniformMatrix3fv(view_loc, 1, GL_FALSE, (float*)&viewScreenMatrix);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);
            gl_has_errors();
            break;
        }
        case GAME_STATE_ID::CUTSCENE_TRANSITION:
        case GAME_STATE_ID::WORLD:
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("Main UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::SetWindowSize(ImVec2(window_width_px * scale_x, window_height_px * scale_y));
            ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::MAINUI], ImVec2(window_width_px*scale_x, window_height_px*scale_y));
            drawMenuItems();
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
			/*skillSelected[0] = true;
			skillSelected[1] = false;
			skillSelected[2] = false;
			skillSelected[3] = false;*/

            // CUTSCENE TRANSITION USES WORLD STATE'S IMAGE BUT THIS WAY KEEPS BACKGROUND FROM MOVING
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.5f, 5.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.0f);
            ImGui::SetNextWindowPos(ImVec2((1500.f / 2.f - 300.f) * scale_x, (window_height_px/2.f - 300.f) * scale_y));
            ImGui::SetNextWindowSize(ImVec2(600.0f * scale_x, 520.0f * scale_y));
            if (*current_game_state == GAME_STATE_ID::CUTSCENE_TRANSITION) {
                ImGui::OpenPopup("End Dialogue", ImGuiPopupFlags_NoOpenOverExistingPopup);
            }
            if (ImGui::BeginPopupModal("End Dialogue", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
                auto window_size = ImGui::GetWindowSize();
                int texture_id = popup.value("image", (int) TEXTURE_ASSET_ID::TEXTURE_COUNT);
                ImVec2 img_size = ImVec2(550.f * scale_x, 360.f * scale_y);
                if (texture_id != (int) TEXTURE_ASSET_ID::TEXTURE_COUNT) {
                    if (texture_id == (int) TEXTURE_ASSET_ID::SHINY_TUTORIAL || texture_id == (int)TEXTURE_ASSET_ID::BOSS_POPUP) {
                        img_size = ImVec2(400.f * scale_x, 256.f * scale_y);
                    }
                    setDrawCursorScreenPos(ImVec2((window_size.x - img_size.x - 20.f) / 2, 10.f));
                    ImGui::Image((void*)(intptr_t)texture_gl_handles[texture_id], img_size);
                }

                setDrawCursorScreenPos(ImVec2(0.f, 20.f));
                textCentered(popup.value("text", "error"));

                setDrawCursorScreenPos(ImVec2(0.f, 10.f));
                ImGui::SetCursorPosX((window_size.x - 120.f) * 0.5f);
                if (ImGui::Button("OK", ImVec2(100.f * scale_x, 35.f * scale_y))) {
                    *current_game_state = GAME_STATE_ID::WORLD;
                    ImGui::CloseCurrentPopup();

                }
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar(2);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::INVENTORY:
            drawInventory();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::SHOP:
            drawShop();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::PARTY:
            drawParty();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::SETTINGS:
            drawSettingsMain("Settings", TEXTURE_ASSET_ID::SETTINGS);

            // Draw buttons over settings options
            createSettingsButtonWindow("Save button", "##SaveButton", ImVec2(280.0f * scale_x, 225.0f * scale_y), GAME_STATE_ID::SAVE);
            createSettingsButtonWindow("Load button","##LoadButton", ImVec2(280.0f * scale_x, 365.0f * scale_y), GAME_STATE_ID::LOAD);
            createSettingsButtonWindow("Tutorial button","##TutorialButton", ImVec2(280.0f * scale_x, 500.0f * scale_y), GAME_STATE_ID::TUTORIAL);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::TELEPORT_1:
            drawSettingsMain("Teleport 1", TEXTURE_ASSET_ID::TELEPORT_1);

            // Draw buttons over teleport options
            createSaveButtons("Tel1 Yes", "##Tel1Button1", ImVec2(412.0f * scale_x, 495.0f * scale_y), GAME_STATE_ID::TELEPORT_1_DONE);
            createSaveButtons("Tel1 No","##Tel1Button2", ImVec2(815.0f * scale_x, 495.0f * scale_y), GAME_STATE_ID::WORLD);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::TELEPORT_1_DONE: {
            currently_lake_1 = true;
            LakeId& lakeInfo = registry.lakes.get(player);
            lakeInfo.id = 1;
            RenderRequest& rr = registry.renderRequests.get(bgEntity);
            rr.used_texture = TEXTURE_ASSET_ID::BACKGROUND;
            Motion& motion = registry.motions.get(player);
            motion.position.x = (float) window_width_px / 2.f;
            motion.position.y = (float) window_height_px / 2.f;
            viewMatrix.mat[2][0] = 0.f;
            viewMatrix.mat[2][1] = 0.f;
            *current_game_state = GAME_STATE_ID::WORLD;
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
        case GAME_STATE_ID::TELEPORT_2_DONE: {
            currently_lake_1 = false;
            LakeId& lakeInfo = registry.lakes.get(player);
            lakeInfo.id = 2;
            RenderRequest& rr = registry.renderRequests.get(bgEntity);
            rr.used_texture = TEXTURE_ASSET_ID::BACKGROUND_2;
            Motion& motion = registry.motions.get(player);
            motion.position.x = (float) window_width_px / 2.f;
            motion.position.y = (float) window_height_px / 2.f;
            viewMatrix.mat[2][0] = 0.f;
            viewMatrix.mat[2][1] = 0.f;
            *current_game_state = GAME_STATE_ID::WORLD;
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
        case GAME_STATE_ID::TELEPORT_2:
            drawSettingsMain("Teleport 2", TEXTURE_ASSET_ID::TELEPORT_2);

            // Draw buttons over teleport options
            createSaveButtons("Tel2 Yes", "##Tel2Button1", ImVec2(412.0f * scale_x, 495.0f * scale_y), GAME_STATE_ID::TELEPORT_2_DONE);
            createSaveButtons("Tel2 No","##Tel2Button2", ImVec2(815.0f * scale_x, 495.0f * scale_y), GAME_STATE_ID::WORLD);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::SAVE:
            drawSettingsMain("Save", TEXTURE_ASSET_ID::SAVE);

            // Draw buttons over save options
            createSaveButtons("Save Yes", "##SaveButton1", ImVec2(412.0f * scale_x, 495.0f * scale_y), GAME_STATE_ID::SAVE_DONE);
            createSaveButtons("Save No","##SaveButton2", ImVec2(815.0f * scale_x, 495.0f * scale_y), GAME_STATE_ID::SETTINGS);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::LOAD:
            drawSettingsMain("Load", TEXTURE_ASSET_ID::LOAD);

            // Draw buttons over load options
            createSaveButtons("Load Yes", "##LoadButton1", ImVec2(370.0f * scale_x, 525.0f * scale_y), GAME_STATE_ID::LOAD_SAVE);
            createSaveButtons("Load No","##LoadButton2", ImVec2(865.0f * scale_x, 525.0f * scale_y), GAME_STATE_ID::SETTINGS);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::LOAD_SAVE: {
            std::ifstream is("../savedata.json", std::ios::ate);
            if (is.tellg() == 0) { // check if save data exists
                *current_game_state = GAME_STATE_ID::LOAD_FAIL;
            } else {
                should_restart_game = true;
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
        case GAME_STATE_ID::LOAD_FAIL:
            drawSettingsMain("Load Fail", TEXTURE_ASSET_ID::LOAD_FAIL);

            createSaveButtons("Load Fail Button", "##LoadFailButton", ImVec2(620.0f * scale_x, 500.0f * scale_y),
                              GAME_STATE_ID::WORLD);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::SAVE_DONE: {
            saveData();

            drawSettingsMain("Save Done", TEXTURE_ASSET_ID::SAVE_DONE);

            createSaveButtons("Save Complete", "##SaveDoneButton", ImVec2(620.0f * scale_x, 500.0f * scale_y),
                              GAME_STATE_ID::WORLD);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
        case GAME_STATE_ID::DELETE_SAVE:
            drawSettingsMain("Delete Save", TEXTURE_ASSET_ID::DELETE_SAVE);

            // Draw buttons over load options
            createSaveButtons("Delete Yes", "##DelButton1", ImVec2(370.0f * scale_x, 525.0f * scale_y), GAME_STATE_ID::DELETE_SAVE_DONE);
            createSaveButtons("Delete No","##DelButton2", ImVec2(865.0f * scale_x, 525.0f * scale_y), GAME_STATE_ID::WORLD);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::DELETE_SAVE_DONE: {
            std::ofstream os("../savedata.json", std::ios::trunc);
            drawSettingsMain("Delete Done", TEXTURE_ASSET_ID::DELETE_SAVE_DONE);

            createSaveButtons("Delete Complete", "##DelDoneButton", ImVec2(620.0f * scale_x, 500.0f * scale_y),
                              GAME_STATE_ID::WORLD);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        }
        case GAME_STATE_ID::TUTORIAL:
            drawSettingsMain("Tutorial", TEXTURE_ASSET_ID::TUTORIAL_BG);

            // Draw buttons over tutorial options
            createTutorialButtonWindow("Basic Controls", "##TutorialButton1", ImVec2(200.0f * scale_x, 225.0f * scale_y), GAME_STATE_ID::TUTORIAL_BASIC);
            createTutorialButtonWindow("How to Fish","##TutorialButton2", ImVec2(200.0f * scale_x, 365.0f * scale_y), GAME_STATE_ID::TUTORIAL_FISH);
            createTutorialButtonWindow("How to Battle Fish","##TutorialButton3", ImVec2(200.0f * scale_x, 500.0f * scale_y), GAME_STATE_ID::TUTORIAL_BATTLE);

            createTutorialBackButton("Back Button", "##BackButton",ImVec2(0, 0), GAME_STATE_ID::SETTINGS);

            ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::TUTORIAL_BASIC:
            drawTutorialScreen("Basic Tutorial", 23);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            registry.players.get(player).basic_tutorial_complete = true;
            break;
        case GAME_STATE_ID::TUTORIAL_FISH:
            drawTutorialScreen("Fishing Tutorial", 24);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            registry.players.get(player).fishing_tutorial_complete = true;
            break;
        case GAME_STATE_ID::TUTORIAL_BATTLE:
            drawTutorialScreen("Battle Tutorial", 25);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
        case GAME_STATE_ID::CUTSCENE:
            // this may need refactoring into its own system since ideally we would pass cutscene id from the event that triggers cutscene...? or pass id through global variable lol. since only one cutscene can play at a time
            // add a loop here that will render based on component properties -> dialogue request
            // want to try using a sprite sheet as well to render portraits.
            if (!registry.dialogues.components.empty()) {
                drawDialogueCutscene();
            }
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            break;
		case GAME_STATE_ID::BATTLE:
            //this is to make sure on start data is synced
            if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_ROUND_BEGIN) {
                hpFill = battle_system->currHealth / battle_system->maxHealth;
                snprintf(hpText, sizeof(hpText), "%.0f/%.0f", battle_system->currHealth, battle_system->maxHealth);
                hpFillEnemy = battle_system->enemySpecies.health / battle_system->enemyMaxHealth;
                snprintf(hpTextEnemy, sizeof(hpTextEnemy), "%.0f/%.0f", battle_system->enemySpecies.health, battle_system->enemyMaxHealth);
            }
			drawBattle();
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            // fishing rod sprite animes
            drawPlayerAnime();
            // party member portraits
            for (int i = 0; i < battle_system->allMembers.size(); i++) {
                std::vector<Entity> members = registry.partyMembers.entities;
                PartyMember p = registry.partyMembers.get(members[i]);
                float xpos = 361.f + i * 300.f;
                drawPortraitsAnime(xpos, p.texture_id);
                if (p.name == battle_system->allMembers[battle_system->currMemberIndex].name && battle_system->curr_battle_state != BattleSystem::StateEnum::STATE_ROUND_BEGIN) {
                    float time = (float)glfwGetTime();
                    float oscillationValue = 0.02f * std::sin(M_PI * time) + .99f;
                    drawSpriteEffect(TEXTURE_ASSET_ID::ARROW, EFFECT_ASSET_ID::EFFECT, vec4(1), { xpos, window_height_px / 2.f * oscillationValue}, { 100.f, 100.f });
                }
            }
            // overlaying particle effects
            if (battle_system->curr_battle_state == BattleSystem::STATE_PLAYER_ACTING) {
                if (battle_system->selectedAnime == BattleSystem::ALLY_MANIFEST) {
                    drawSpriteEffect(TEXTURE_ASSET_ID::PARTICLE, EFFECT_ASSET_ID::EFFECT, ally1_purple, { window_width_px / 2, 280.f}, vec2(100.f + 10.f * sin(4.f * (float)glfwGetTime())));
                    drawSpriteEffect(TEXTURE_ASSET_ID::PARTICLE, EFFECT_ASSET_ID::EFFECT, vec4(1), { window_width_px / 2, 280.f}, vec2(75.f + 10.f * sin(4.f * (float)glfwGetTime())));
                    battle_system->createParticles(BattleSystem::ALLY_MANIFEST);
                }
            }
            for (Particle const& particle : battle_system->particles)
            {
                if (particle.life > 0.0f)
                {
                    drawSpriteEffect(TEXTURE_ASSET_ID::PARTICLE, EFFECT_ASSET_ID::EFFECT, particle.color, particle.position, particle.size);
                    //drawMeshEffect(GEOMETRY_BUFFER_ID::PEBBLE, EFFECT_ASSET_ID::PARTICLE, particle.color, { window_width_px / 2 + particle.position.x, window_height_px / 2 + particle.position.y }, 0, particle.size);
                }
            }
            if (battle_system->curr_battle_state == BattleSystem::STATE_EFFECT_PLAYING) {
                playEffect(battle_system->selectedAnime);
                if (battle_system->selectedAnime == BattleSystem::ALLY_DOOM && buffIndex == 1) {
                    float progress = glfwGetTime() - lastTimeEffect;
                    drawSpriteEffect(TEXTURE_ASSET_ID::DOOM, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, 1.f - progress * 2.f), {window_width_px / 2, 280.f}, vec2(100.f) + (float) pow(progress * 40.f, 2));
                }
            }
            //drawMeshEffect(GEOMETRY_BUFFER_ID::PEBBLE, EFFECT_ASSET_ID::PARTICLE, vec4(1), {window_width_px / 2, window_height_px / 2}, 0, vec2(60.f + 5.f * sin(4.f * (float)glfwGetTime())));

            for (auto const& rq : renderRequestsNonEntity) {
                if (rq.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT) {
                    drawMeshEffect(rq.used_geometry, rq.used_effect, rq.color, rq.pos, rq.angle, rq.scale);
                }
                else if (rq.used_texture != TEXTURE_ASSET_ID::TEXTURE_COUNT) {
                    drawSpriteEffect(rq.used_texture, rq.used_effect, rq.color, rq.pos, rq.scale);
                }
            }
            break;
        case GAME_STATE_ID::TRANSITION:
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            float elapsedTime;
            if (lastTimeTransition == 0.f)
                lastTimeTransition = (float)glfwGetTime();
            elapsedTime = (float)glfwGetTime() - lastTimeTransition;
            if (elapsedTime > 1.5f) {
                lastTimeTransition = 0.f;
                *current_game_state = GAME_STATE_ID::BATTLE;
            }
            drawSpriteEffect(TEXTURE_ASSET_ID::TRANSITION, EFFECT_ASSET_ID::EFFECT, vec4(1.f), vec2((window_width_px * 2.4) - elapsedTime * 3000, window_height_px / 2.f), vec2(window_width_px * 5, window_height_px * 2.5));
            break;
        case GAME_STATE_ID::TUTORIAL_BATTLE_ADVANCED:
            drawSpriteEffect(TEXTURE_ASSET_ID::MENU_BG, EFFECT_ASSET_ID::EFFECT, vec4(1.f), { window_width_px / 2.f, window_height_px / 2.f }, {window_width_px, window_height_px});
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
            std::string s = "Battle Tutorial " + std::to_string(battle_tutorial_index);
            ImGui::Begin(s.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::SetWindowSize(ImVec2(window_width_px* scale_x, window_height_px* scale_y));
            ImGui::Image((void*)texture_gl_handles[(intptr_t)battle_tutorials[battle_tutorial_index]], ImVec2(window_width_px* scale_x, window_height_px* scale_y));
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::End();

            createBattleTutorialButton(ImVec2(1330.f * scale_x, 31.f * scale_y));

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            break;
    }
}

void RenderSystem::saveData()
{
    // open save file for writing
    std::ofstream os("../savedata.json");
    cereal::JSONOutputArchive archive(os);

    // save player's current position
    Motion player_motion = registry.motions.get(player);
    float posX = player_motion.position.x;
    float posY = player_motion.position.y;
    archive(CEREAL_NVP(posX), CEREAL_NVP(posY));

    // save lake id
    int lake_id = registry.lakes.get(player).id;
    // save player flags
    Player player_info = registry.players.get(player);
    bool ally1_recruited = player_info.ally1_recruited;
    bool ally2_recruited = player_info.ally2_recruited;
    bool shiny_tutorial = player_info.shiny_tutorial;
    bool battle_tutorial_complete = player_info.battle_tutorial_complete;
    bool lake1_boss_defeated = player_info.lake1_boss_defeated;
    int num_fish_caught_lake1 = player_info.num_fish_caught_lake1;
    int num_fish_caught_lake2 = player_info.num_fish_caught_lake2;
    bool basic_tutorial_complete = player_info.basic_tutorial_complete;
    bool fishing_tutorial_complete = player_info.fishing_tutorial_complete;
    bool lake2_entered = player_info.lake2_entered;
    archive(CEREAL_NVP(lake_id), CEREAL_NVP(ally1_recruited), CEREAL_NVP(ally2_recruited), CEREAL_NVP(shiny_tutorial), CEREAL_NVP(battle_tutorial_complete), CEREAL_NVP(lake1_boss_defeated),
        CEREAL_NVP(num_fish_caught_lake1), CEREAL_NVP(num_fish_caught_lake2), CEREAL_NVP(basic_tutorial_complete), CEREAL_NVP(fishing_tutorial_complete), CEREAL_NVP(lake2_entered));
   
    // save camera's current position
    float cameraX = viewMatrix.mat[2][0];
    float cameraY = viewMatrix.mat[2][1];
    archive(CEREAL_NVP(cameraX), CEREAL_NVP(cameraY));

    // save current gold
    Wallet player_wallet = registry.wallet.get(player);
    int gold = player_wallet.gold;
    archive(CEREAL_NVP(gold));

    // save current durability, attack, & defense
    Entity rod = registry.fishingRods.entities[0];
    Durability dur = registry.durabilities.get(rod);
    Attack atk = registry.attacks.get(rod);
    Defense def = registry.defenses.get(rod);
    float dur_max = dur.max;
    float dur_curr = dur.current;
    float dur_upgrades = dur.num_upgrades;
    int attack = atk.damage;
    float attack_upgrades = atk.num_upgrades;
    int defense = def.value;
    archive(CEREAL_NVP(dur_max), CEREAL_NVP(dur_curr), CEREAL_NVP(dur_upgrades), CEREAL_NVP(attack), CEREAL_NVP(attack_upgrades), CEREAL_NVP(defense));

    // save fishing inventory
    uint64_t fish_inventory_size = registry.fishInventory.components.size();
    std::vector<Fish> fishes = registry.fishInventory.components;
    archive(CEREAL_NVP(fishes));

    // save fishing log
    std::vector<FishingLog> fishlog = registry.fishingLog.components;
    archive(CEREAL_NVP(fishlog));

    // save current lake
    archive(CEREAL_NVP(currently_lake_1));
    archive(CEREAL_NVP(second_lake_unlocked));
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
    // Getting size of window
    int w, h;
    glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

    // First render to the custom framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    gl_has_errors();
    // Clearing backbuffer
    glViewport(0, 0, w, h);
    glDepthRange(0.00001, 10);
    glClearColor(0, 0, 1, 1.0);
    glClearDepth(10.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
    // and alpha blending, one would have to sort
    // sprites back to front
    gl_has_errors();
    mat3 projection_2D = createProjectionMatrix();
    // Draw all textured meshes that have a position and size component
    // registry.renderRequests.entities

    registry.renderRequests.sort([ ](Entity& lhs, Entity& rhs)
    {
    	int left = registry.players.has(lhs) ? 1 : 0;
        int right = registry.players.has(rhs) ? 1 : 0;
        return left < right;
    });

    for (Entity entity : registry.renderRequests.entities)
    {
       if (registry.motions.has(entity)) {
		   if (registry.sprites.has(entity)) {
			   Sprite& sprite = registry.sprites.get(entity);
			   Motion& motion = registry.motions.get(entity);
			   TEXTURE_ASSET_ID texture_id = registry.renderRequests.get(entity).used_texture;
			   drawSpriteAnime(sprite.current_frame, sprite.rows, sprite.columns, texture_id, motion.position, motion.scale, 0.f, false);

               const GLuint used_effect_enum = (GLuint)EFFECT_ASSET_ID::TEXTURED;
               const GLuint program = (GLuint)effects[used_effect_enum];
               GLuint light_up_uloc = glGetUniformLocation(program, "light_up");
               assert(light_up_uloc >= 0);
               glUniform1i(light_up_uloc, 0);
               gl_has_errors();
		   }
		   else {
               if (registry.renderRequests.get(entity).is_visible == true) {
                   drawTexturedMesh(entity, projection_2D);
               }
		   }
       }
    }
    // Truely render to the screen
    drawToScreen();
	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors();
}

void RenderSystem::drawRoundBanner() {
    if (!battle_system->initialized)
        return;
    float alpha = 1.f;
    float time = (float)glfwGetTime();
    if (lastTimeBanner == 0.0f || time - lastTimeBanner < 0) {
        lastTimeBanner = (float)time;
    }
    float elapsedTime = time - lastTimeBanner;
    float duration = 1.3f;
    if (elapsedTime < duration) {
        alpha = elapsedTime;
    }
    if ((time - lastTimeBanner) > 1.3f) {
       lastTimeBanner = 0.0f;
       //note: make sure currMember here is not the previous acted, but the upcoming character (ie update currMember immediately when previous char's action ends)
       if (battle_system->allMembers[battle_system->currMemberIndex].actionIndex < battle_system->enemy->actionIndex) {
            battle_system->curr_battle_state = BattleSystem::StateEnum::STATE_PLAYER_TURN;
       }
       else {
           battle_system->curr_battle_state = BattleSystem::StateEnum::STATE_ENEMY_TURN;
       }
    }
    ImGui::SetNextWindowSize(ImVec2(1600.f * scale_x, 50.f * scale_y));
    ImGui::SetNextWindowPos(ImVec2(-50.f, ((float)window_height_px / 2) * scale_y));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ui_border_colour);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::Begin("Banner", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImVec2 textSize = ImGui::CalcTextSize("Round 1");
    ImVec2 textPosition = ImVec2((ImGui::GetWindowWidth() - textSize.x) * 0.5f, (ImGui::GetWindowHeight() - textSize.y) * 0.5f);

    ImGui::SetCursorPos(textPosition);
    ImGui::SetWindowFontScale(1.7f);
    ImGui::TextColored(ImVec4(0.9804f, 0.9137f, 0.7647f, alpha), "Round %s", std::to_string(battle_system->roundCounter).c_str());
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();
}

void RenderSystem::drawPlayerAnime() {
    float time = (float)glfwGetTime();
    float frameDuration = 0.3f; // Time in seconds for each frame
    int numFrames = 12;
    int currentFrame = static_cast<int>(time / frameDuration) % numFrames;
    vec2 rodPosition = { 140.f, 667.f };
    vec2 rodScale = { 0.85 * 200.f, 0.85 * 385.f };
    drawSpriteAnime(currentFrame, 3, 4, TEXTURE_ASSET_ID::ROD_IDLE, rodPosition, rodScale);

    if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_PLAYER_ACTING) {
        lastTimeText = 0;
        if (lastTimePlayer == 0.0f) {
            lastTimePlayer = (float) time;
        }
        int rodActingFrame = static_cast<int>((time - lastTimePlayer) / (frameDuration / 4)) % numFrames;

        if ((time - lastTimePlayer) > 0.8f) {
            lastTimePlayer = 0.0f;
            battle_system->curr_battle_state = BattleSystem::StateEnum::STATE_EFFECT_PLAYING;
        }
        if (battle_system->selectedAnime == BattleSystem::AnimeEnum::PLAYER_ATK) {

            if (rodActingFrame < numFrames) {
                drawSpriteAnime(rodActingFrame, 3, 4, TEXTURE_ASSET_ID::ROD_REEL, rodPosition, rodScale);
            }
        }
        else if (battle_system->selectedAnime == BattleSystem::AnimeEnum::PLAYER_HEAL) {
            if (rodActingFrame < numFrames) {
                drawSpriteAnime(rodActingFrame, 3, 4, TEXTURE_ASSET_ID::ROD_RELEASE, rodPosition, rodScale);
            }
        }
        else if (battle_system->selectedAnime == BattleSystem::AnimeEnum::ALLY_MANIFEST) {
            // do nothing, the particle spawn is handled in battle_system
        }
        else {
            // if no animation associated, go straight to effect state
            lastTimePlayer = 0.0f;
            battle_system->curr_battle_state = BattleSystem::StateEnum::STATE_EFFECT_PLAYING;
        }
    }
    else if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_PLAYER_DEFEATED)
    {
        if (lastTimePlayer == 0.0f) {
            lastTimePlayer = (float)time;
        }
        int rodActingFrame = static_cast<int>((time - lastTimePlayer) / (frameDuration / 4)) % numFrames;

        if ((time - lastTimePlayer) > 0.8f) {
            lastTimePlayer = 0.0f;
            battle_system->reset();
        }
        if (rodActingFrame < numFrames) {
            drawSpriteAnime(rodActingFrame, 3, 4, TEXTURE_ASSET_ID::ROD_DEFEATED, rodPosition, rodScale);
        }
    }

    //restore original sprite texcoord
    GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];
    const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];
    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    std::vector<TexturedVertex> textured_vertices(4);
    textured_vertices[0].position = { -1.f / 2, +1.f / 2, 0.f };
    textured_vertices[1].position = { +1.f / 2, +1.f / 2, 0.f };
    textured_vertices[2].position = { +1.f / 2, -1.f / 2, 0.f };
    textured_vertices[3].position = { -1.f / 2, -1.f / 2, 0.f };
    textured_vertices[0].texcoord = { 0.f, 1.0f };
    textured_vertices[1].texcoord = { 1.f, 1.f };
    textured_vertices[2].texcoord = { 1.f, 0.f };
    textured_vertices[3].texcoord = { 0.f, 0.f };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(textured_vertices[0]) * textured_vertices.size(), textured_vertices.data());
    gl_has_errors();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();
}

void RenderSystem::drawOnMenu() {
    Entity player = registry.players.entities[0];
    Entity fishingRod = registry.fishingRods.entities[0];
    Durability &durability = registry.durabilities.get(fishingRod);
    Attack &attack = registry.attacks.get(fishingRod);
    Wallet &wallet = registry.wallet.get(player);

    ImGui::SetCursorScreenPos(ImVec2(250.f * scale_x, 733.f * scale_y));
    ImGui::Text("%d/%d", (int)durability.current, (int)durability.max);

    ImGui::SetCursorScreenPos(ImVec2(250.f * scale_x, 785.f * scale_y));
    ImGui::Text("%d", attack.damage);

    ImGui::SetCursorScreenPos(ImVec2(250.f * scale_x, 850.f * scale_y));
    ImGui::Text("%d", wallet.gold);
}

void RenderSystem::drawBattle() {
    //draw skills table, character portrait, hp bar
	createSkillsTable();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::Begin("Battle UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetWindowSize(ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    ImGui::GetWindowDrawList()->AddImage((void*)texture_gl_handles[(intptr_t)TEXTURE_ASSET_ID::SHORE], ImVec2(0.f, 0.f), ImVec2(window_width_px * scale_x, window_height_px * scale_y), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f));
    ImGui::GetWindowDrawList()->AddImage((void*)texture_gl_handles[(intptr_t)TEXTURE_ASSET_ID::LAKE], ImVec2(0.f, 450.f * scale_y), ImVec2(window_width_px * scale_x, window_height_px * scale_y), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f));
    ImGui::Image((void*)texture_gl_handles[(intptr_t)TEXTURE_ASSET_ID::BATTLE_UI_MAIN], ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
	ImGui::End();

	//draw fishing rod
	ImGui::PushStyleColor(ImGuiCol_WindowBg, battle_yellow_colour); //yellow part of battle ui
	ImGui::PushStyleColor(ImGuiCol_Border, ui_border_colour); // brown border part
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.f); // brown border thickness
	ImGui::SetNextWindowPos(ImVec2(40.f*scale_x, 475.f*scale_y)); // set block position
	ImGui::SetNextWindowSize(ImVec2(200.f*scale_x, 385.f*scale_y)); // set block size
	ImGui::Begin("Fishing Rod", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::End();

    for (int i = 0; i < battle_system->allMembers.size(); i++) {
        float xpos = 265.f + i * 300.f;
        createCharacterPortrait(ImVec2(xpos * scale_x, 475.f * scale_y), i);
    }
	createHPBar();
	createEnemy();
    if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_ROUND_BEGIN)
        drawRoundBanner();
}

void RenderSystem::drawDmgText(const char* text, ImVec2 position, ImU32 textColor) {
    ImU32 borderColor = IM_COL32_BLACK;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(ImVec2(100.f * scale_x, 40.f * scale_y));
    ImGui::Begin("Dmg Text", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // Draw the text with a border
    ImGui::PushFont(font_effect);
    drawList->AddText(ImVec2(position.x + 2.f, position.y + 2.f), borderColor, text);
    drawList->AddText(position, textColor, text);

    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::End();
}

void RenderSystem::drawSpriteAnime(int frame, int num_rows, int num_columns, TEXTURE_ASSET_ID texture_asset_id, vec2 screen_position, vec2 scale, float darken, bool use_screen_matrix) {

    mat3 projection_2D = createProjectionMatrix();
    Transform transform;

	transform.translate(screen_position);
	//fprintf(stderr, "render motion position x: %f \n", screen_position.x);
	transform.scale(scale);
	const GLuint used_effect_enum = (GLuint)EFFECT_ASSET_ID::TEXTURED;
	const GLuint program = (GLuint)effects[used_effect_enum];

    // Setting shaders
    glUseProgram(program);
    gl_has_errors();

	const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE_SHEET];
	const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE_SHEET];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	std::vector<TexturedVertex> sprite_sheet_vertices(4);
	sprite_sheet_vertices[0].position = { -1.f / 2, +1.f / 2, 0.f };
	sprite_sheet_vertices[1].position = { +1.f / 2, +1.f / 2, 0.f };
	sprite_sheet_vertices[2].position = { +1.f / 2, -1.f / 2, 0.f };
	sprite_sheet_vertices[3].position = { -1.f / 2, -1.f / 2, 0.f };
	sprite_sheet_vertices[0].texcoord = { (1.0f / num_columns) * (frame % num_columns), (1.0f / num_rows) * ((frame / num_columns) % num_rows + 1) }; // bottom left
	sprite_sheet_vertices[1].texcoord = { (1.0f / num_columns) * (frame % num_columns + 1), (1.0f / num_rows) * ((frame / num_columns) % num_rows + 1) }; // bottom right
	sprite_sheet_vertices[2].texcoord = { (1.0f / num_columns) * (frame % num_columns + 1), (1.0f / num_rows) * ((frame / num_columns) % num_rows) }; // top right
	sprite_sheet_vertices[3].texcoord = { (1.0f / num_columns) * (frame % num_columns), (1.0f / num_rows) * ((frame / num_columns) % num_rows) }; //top left
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sprite_sheet_vertices[0]) * sprite_sheet_vertices.size(), sprite_sheet_vertices.data());
	gl_has_errors();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    GLuint darken_factor = glGetUniformLocation(program, "darken_factor");
    glUniform1f(darken_factor, darken);
    gl_has_errors();

    // Light up?
    GLint light_up_uloc = glGetUniformLocation(program, "light_up");
    assert(light_up_uloc >= 0);

    // !!! TODO A1: set the light_up shader variable using glUniform1i,
    // similar to the glUniform1f call below. The 1f or 1i specified the type, here a single int.
    if (registry.lightUp.has(player) && (texture_asset_id == TEXTURE_ASSET_ID::PLAYER_LEFT_SHEET || texture_asset_id == TEXTURE_ASSET_ID::PLAYER_RIGHT_SHEET || texture_asset_id == TEXTURE_ASSET_ID::PLAYER_UP_SHEET || texture_asset_id == TEXTURE_ASSET_ID::PLAYER_DOWN_SHEET)) {
        glUniform1i(light_up_uloc, 1);
    }
    else {
        glUniform1i(light_up_uloc, 0);
    }
    gl_has_errors();

    // inside if
    GLint in_position_loc = glGetAttribLocation(program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
    gl_has_errors();
    assert(in_texcoord_loc >= 0);

    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(TexturedVertex), (void*)0);
    gl_has_errors();

    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(
            in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
            (void*)sizeof(
                    vec3)); // note the stride to skip the preceeding vertex position

    glEnable(GL_BLEND);
    // Enabling and binding texture to slot 0
    glActiveTexture(GL_TEXTURE0);
    gl_has_errors();

	GLuint texture_id =
		texture_gl_handles[(GLuint)texture_asset_id];

    glBindTexture(GL_TEXTURE_2D, texture_id);
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(program, "fcolor");
    const vec3 color = vec3(1);
    glUniform3fv(color_uloc, 1, (float*)&color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();

    GLsizei num_indices = size / sizeof(uint16_t);
    // GLsizei num_triangles = num_indices / 3;

    GLint currProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
    // Setting uniform values to the currently bound program
    GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
    GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection_2D);
    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glm::mat3 viewScreenMatrix = glm::mat3(1.0f);
    GLuint view_loc = glGetUniformLocation(currProgram, "view");

    // rod idle depends on screen, not camera position
    if (use_screen_matrix) {
        glUniformMatrix3fv(view_loc, 1, GL_FALSE, (float*)&viewScreenMatrix);
    }
    else {
        glUniformMatrix3fv(view_loc, 1, GL_FALSE, (float*)&viewMatrix);
    }
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();

}

void RenderSystem::drawMeshEffect(GEOMETRY_BUFFER_ID geo_id, EFFECT_ASSET_ID eff_id, vec4 c, vec2 pos, float angle, vec2 scale) {
    mat3 projection_2D = createProjectionMatrix();
    Transform transform;

    transform.translate(pos);
    transform.rotate(angle);
    transform.scale(scale);
    const GLuint used_effect_enum = (GLuint)eff_id;
    const GLuint program = (GLuint)effects[used_effect_enum];

    // Setting shaders
    glUseProgram(program);
    gl_has_errors();

    const GLuint vbo = vertex_buffers[(GLuint)geo_id];
    const GLuint ibo = index_buffers[(GLuint)geo_id];

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl_has_errors();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    GLint in_position_loc = glGetAttribLocation(program, "in_position");
    GLint in_color_loc = glGetAttribLocation(program, "in_color");
    gl_has_errors();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
        sizeof(ColoredVertex), (void*)0);
    gl_has_errors();

    glEnableVertexAttribArray(in_color_loc);
    glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
        sizeof(ColoredVertex), (void*)sizeof(vec3));
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(program, "fcolor");
    const vec4 color = c;
    glUniform4fv(color_uloc, 1, (float*)&color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();

    GLsizei num_indices = size / sizeof(uint16_t);
    // GLsizei num_triangles = num_indices / 3;

    GLint currProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
    // Setting uniform values to the currently bound program
    GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
    GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection_2D);
    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glm::mat3 viewScreenMatrix = glm::mat3(1.0f);
    GLuint view_loc = glGetUniformLocation(currProgram, "view");
    // rod idle depends on screen, not camera position
    glUniformMatrix3fv(view_loc, 1, GL_FALSE, (float*)&viewScreenMatrix);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();
}


void RenderSystem::drawSpriteEffect(TEXTURE_ASSET_ID id, EFFECT_ASSET_ID eff_id, vec4 c, vec2 pos, vec2 scale) {

    mat3 projection_2D = createProjectionMatrix();
    Transform transform;

    transform.translate(pos);
    transform.scale(scale);
    const GLuint used_effect_enum = (GLuint)EFFECT_ASSET_ID::EFFECT;
    const GLuint program = (GLuint)effects[used_effect_enum];

    // Setting shaders
    glUseProgram(program);
    gl_has_errors();

    GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];
    const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SPRITE];

    // Setting vertex and index buffers
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl_has_errors();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    GLint in_position_loc = glGetAttribLocation(program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
    gl_has_errors();
    assert(in_texcoord_loc >= 0);

    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(TexturedVertex), (void*)0);
    gl_has_errors();

    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(
            in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
            (void*)sizeof(
                    vec3)); // note the stride to skip the preceeding vertex position

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Enabling and binding texture to slot 0
    glActiveTexture(GL_TEXTURE0);
    gl_has_errors();

    GLuint texture_id =
            texture_gl_handles[(GLuint)id];

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glUniform1i(glGetUniformLocation(program, "inputTexture"), 0);
    gl_has_errors();

    // Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(program, "fcolor");
    const vec4 color = c;
    glUniform4fv(color_uloc, 1, (float*)&color);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();

    GLsizei num_indices = size / sizeof(uint16_t);
    // GLsizei num_triangles = num_indices / 3;

    GLint currProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
    // Setting uniform values to the currently bound program
    GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
    GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection_2D);
    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glm::mat3 viewScreenMatrix = glm::mat3(1.0f);
    GLuint view_loc = glGetUniformLocation(currProgram, "view");
    // rod idle depends on screen, not camera position
    glUniformMatrix3fv(view_loc, 1, GL_FALSE, (float*)&viewScreenMatrix);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();

}

void RenderSystem::playEnemyAnime(BattleSystem::AnimeEnum i) {
    ImVec2 spriteSize = ImVec2(390.f, 390.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowSize(spriteSize);
    float time;
    float elapsedTime;

    TEXTURE_ASSET_ID enemy_fish;
    switch (battle_system->enemySpecies.id) {
    case (int)FISH_TEXTURE_ASSET_ID::SLEEP_NARWHAL:
        enemy_fish = TEXTURE_ASSET_ID::SLEEP_NARWHAL;
        break;
    case (int)FISH_TEXTURE_ASSET_ID::TURTLE:
        enemy_fish = TEXTURE_ASSET_ID::TURTLE;
        break;
    case (int)FISH_TEXTURE_ASSET_ID::WALRUS:
        enemy_fish = TEXTURE_ASSET_ID::WALRUS;
        break;
    case (int)FISH_TEXTURE_ASSET_ID::INKWHALE:
        enemy_fish = TEXTURE_ASSET_ID::INKWHALE;
        break;
    }

    switch (i) {
        //enemy on hit displacement
    case BattleSystem::AnimeEnum::PLAYER_ATK:
    case BattleSystem::AnimeEnum::ALLY_ATK:
    case BattleSystem::AnimeEnum::ALLY_MANIFEST:
    case BattleSystem::AnimeEnum::ALLY_CURSE:
    case BattleSystem::AnimeEnum::ALLY_DOOM:
    case BattleSystem::AnimeEnum::ALLY_DISTRACT:
        ImGui::SetNextWindowPos(ImVec2((window_width_px / 2 - spriteSize.x / 2) * scale_x + 50.f, 100.f * scale_y)); // set block position
        ImGui::Begin("Enemy", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)enemy_fish], spriteSize);
        break;
    //enemy attacking
    case BattleSystem::AnimeEnum::ENEMY_EXECUTION:
    case BattleSystem::AnimeEnum::ENEMY_DEBUFF:
    case BattleSystem::AnimeEnum::ENEMY_ATK:
        time = (float) glfwGetTime();
        if (lastTimeEnemy == 0.0f) {
            lastTimeEnemy = time;
        }
        elapsedTime = time - lastTimeEnemy;
        if (elapsedTime > (2.f * M_PI / 5.f)) {
            lastTimeEnemy = 0.0f;
            while (!renderRequestsNonEntity.empty()) {
                renderRequestsNonEntity.pop_back();
            }
            battle_system->curr_battle_state = BattleSystem::StateEnum::STATE_EFFECT_PLAYING;
        }

        ImGui::SetNextWindowPos(ImVec2(((float) window_width_px / 2 - spriteSize.x / 2) * scale_x, 55.f * scale_y * (0.5 * std::cos(elapsedTime * 5.f) + 1.5f))); // set block position
        ImGui::Begin("Enemy", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)enemy_fish], spriteSize);

        vec2 start = { window_width_px / 2, 280.f };
        vec2 end = { 130.f, 667.f };
        float m = (end.y - start.y) / (end.x - start.x);
        float angle = atan(m);
        float x = start.x - elapsedTime * 500.f;
        float y = start.y - m * (start.x - x);
        bool hasEffect = false;
        int index = -1;
        if (lastTimeEnemy != 0 && battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_name == "Throw Salmon") {
            //draw the salmon
            for (RenderRequestsNonEntity rq : renderRequestsNonEntity) {
                if (rq.id == "Salmon")
                    hasEffect = true;
                index++;
            }
            if (hasEffect) {
                renderRequestsNonEntity[index] = { "Salmon", GEOMETRY_BUFFER_ID::SALMON, TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::SALMON, vec4(1.f), {x, y}, angle, {150.f, 80.f} };
            }
            else {
                renderRequestsNonEntity.push_back({ "Salmon", GEOMETRY_BUFFER_ID::SALMON, TEXTURE_ASSET_ID::TEXTURE_COUNT, EFFECT_ASSET_ID::SALMON, vec4(1.f), {x, y}, angle, {150.f, 80.f}});
            }
        }
        else if (lastTimeEnemy != 0 && battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_name == "Execution") {
            for (RenderRequestsNonEntity rq : renderRequestsNonEntity) {
                if (rq.id == "Execution")
                    hasEffect = true;
                index++;
            }
            if (hasEffect) {
                renderRequestsNonEntity[index] = { "Execution", GEOMETRY_BUFFER_ID::GEOMETRY_COUNT, TEXTURE_ASSET_ID::BOSS_EXECUTION, EFFECT_ASSET_ID::TEXTURED, vec4(1.f), {x + 60.f, y}, angle, 0.7f * (elapsedTime + 0.3f) * vec2(400.f)};
            }
            else {
                renderRequestsNonEntity.push_back({ "Execution", GEOMETRY_BUFFER_ID::GEOMETRY_COUNT, TEXTURE_ASSET_ID::BOSS_EXECUTION, EFFECT_ASSET_ID::TEXTURED, vec4(1.f), {x + 60.f, y}, angle, 0.7f * (elapsedTime + 0.3f) * vec2(400.f) });
            }
        }
        break;
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::End();
}

void RenderSystem::playEffect(BattleSystem::AnimeEnum i) {
    if (!battle_system->initialized)
        return;
    float time = (float) glfwGetTime();
    float elapsedTime;
    float progress;
    if (lastTimeEffect == 0.0f) {
        lastTimeEffect = time;
    }
    elapsedTime = time - lastTimeEffect;
    progress = 1.f - (float)(pow(elapsedTime / 0.5, 3));
    vec2 rodPosition = { 140.f, 667.f };
    vec2 rodScale = { 0.85 * 200.f, 0.85 * 385.f };
    std::vector<TEXTURE_ASSET_ID> debuffIds = { TEXTURE_ASSET_ID::ATK_BUFF_EFFECT, TEXTURE_ASSET_ID::DEF_BUFF_EFFECT, TEXTURE_ASSET_ID::SPD_BUFF_EFFECT, };
    switch (i) {
        case BattleSystem::ALLY_MANIFEST:
        case BattleSystem::PLAYER_ATK:
            //hitting enemy
            if (progress < 0.f) {
                drawSpriteEffect(TEXTURE_ASSET_ID::ON_HIT_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, 0.f), { window_width_px / 2, 280.f }, { 200.f, 200.f });
                if (progress < -.0f) { // this check is for dmg number animation time extension
                    lastTimeEffect = 0.0f;
                    battle_system->selectedAnime = BattleSystem::NONE;
                    // last character in action check
                    battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
                }
            }
            else {
                drawSpriteEffect(TEXTURE_ASSET_ID::ON_HIT_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), {window_width_px / 2, 280.f }, { 200.f, 200.f });
            }
            break;
        case BattleSystem::PLAYER_HEAL:
            //healing
            hpFill = battle_system->currHealth / battle_system->maxHealth;
            snprintf(hpText, sizeof(hpText), "%.0f/%.0f", battle_system->currHealth, battle_system->maxHealth);
            if (progress < 0.f) {
                drawSpriteEffect(TEXTURE_ASSET_ID::HEAL_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, 0.f), { 130.f, 667.f + 20.f * 0.f }, { 200.f, 200.f });
                if (progress < -2.f) {
                    lastTimeEffect = 0.0f;
                    battle_system->selectedAnime = BattleSystem::NONE;
                    // last character in action check
                    battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
                }
            }
            else {
                drawSpriteEffect(TEXTURE_ASSET_ID::HEAL_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { 130.f, 667.f + 20.f * progress}, { 200.f, 200.f });
            }
            break;
        case BattleSystem::PLAYER_BUFF_ATK:
            if (progress < 0.f) {
                progress = 0.f;
                lastTimeEffect = 0.0f;
                battle_system->selectedAnime = BattleSystem::NONE;
                // last character in action check
                battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
            }
            //drawSpriteEffect(TEXTURE_ASSET_ID::ATK_BUFF_EFFECT, vec4(1.f, 1.f, 1.f, progress), { 135.f, 667.f + 20.f * progress }, { 200.f, 200.f });
            for (int i = 0; i < registry.partyMembers.size(); i++) {
                float xpos = 361.f + i * 300.f;
                drawSpriteEffect(TEXTURE_ASSET_ID::ATK_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { xpos, 560.f + 20.f * progress }, { 150.f, 150.f }); // eventually loop this buff effect on allMembers
            }
            break;
        case BattleSystem::PLAYER_BUFF_DEF:
            if (progress < 0.f) {
                progress = 0.f;
                lastTimeEffect = 0.0f;
                battle_system->selectedAnime = BattleSystem::NONE;
                // last character in action check
                battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
            }
            //drawSpriteEffect(TEXTURE_ASSET_ID::ATK_BUFF_EFFECT, vec4(1.f, 1.f, 1.f, progress), { 135.f, 667.f + 20.f * progress }, { 200.f, 200.f });
            drawSpriteEffect(TEXTURE_ASSET_ID::DEF_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { 130.f, 667.f + 20.f * progress }, { 200.f, 200.f }); // eventually loop this buff effect on allMembers
            break;
        case BattleSystem::ENEMY_EXECUTION:
        case BattleSystem::ENEMY_ATK:
            //hitting player
            if (progress > 0)
                drawSpriteAnime(0, 1, 1, TEXTURE_ASSET_ID::ROD_ON_HIT, rodPosition, rodScale);
            if (progress < 0.f) {
                drawSpriteEffect(TEXTURE_ASSET_ID::ON_HIT_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, 0.f), { 140.f, 667.f }, { 200.f, 200.f });
                if (progress < -2.f) {
                    lastTimeEffect = 0.0f;
                    battle_system->enemyActed = true;
                    battle_system->selectedAnime = BattleSystem::NONE;
                    battle_system->checkRoundOver(battle_system->enemy->actionIndex);
                }
            }
            else {
                drawSpriteEffect(TEXTURE_ASSET_ID::ON_HIT_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { 140.f, 667.f }, { 200.f, 200.f });
                if (battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_effect.type == EFFECT_TYPE::DEBUFF_SPD) {
                    for (int i = 0; i < registry.partyMembers.size(); i++) {
                        float xpos = 361.f + i * 300.f;
                        drawSpriteEffect(TEXTURE_ASSET_ID::SPD_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(progress), { xpos, 580.f - 20.f * progress }, { 150.f, 150.f });
                    }
                }
            }
            break;
        case BattleSystem::AnimeEnum::ENEMY_DEBUFF:
            if (progress > 0 && battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_scale != 0)
                drawSpriteAnime(0, 1, 1, TEXTURE_ASSET_ID::ROD_ON_HIT, rodPosition, rodScale);
            if (progress < 0.f) {
                if (progress < -2.f) {
                    lastTimeEffect = 0.0f;
                    battle_system->enemyActed = true;
                    battle_system->selectedAnime = BattleSystem::NONE;
                    battle_system->checkRoundOver(battle_system->enemy->actionIndex);
                }
            }
            else {
                if (battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_effect.type == EFFECT_TYPE::DEBUFF_DEF) {
                    drawSpriteEffect(TEXTURE_ASSET_ID::DEF_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(progress), { rodPosition.x, rodPosition.y - 20.f * progress }, { 200.f, 200.f });
                    if (battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_name == "Dream Transfer")
                        drawSpriteEffect(TEXTURE_ASSET_ID::ATK_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { window_width_px / 2, 280.f + 20.f * progress }, { 150.f, 150.f });
                } else if (battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_effect.type == EFFECT_TYPE::DEBUFF_SPD) {
                    for (int i = 0; i < registry.partyMembers.size(); i++) {
                        float xpos = 355.f + i * 300.f;
                        drawSpriteEffect(TEXTURE_ASSET_ID::SPD_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(progress), { xpos, 580.f - 20.f * progress }, { 150.f, 150.f });
                    }
                }
            }
            break;
        case BattleSystem::ALLY_ATK:
            if (progress < 0.f) {
                drawSpriteEffect(TEXTURE_ASSET_ID::ON_HIT_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, 0.f), { window_width_px / 2, 280.f }, { 200.f, 200.f });
                if (progress < -.0f) { // this check is for dmg number animation time extension
                    lastTimeEffect = 0.0f;
                    battle_system->selectedAnime = BattleSystem::NONE;
                    // last character in action check
                    battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
                }
            }
            else {
                drawSpriteEffect(TEXTURE_ASSET_ID::ON_HIT_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { window_width_px / 2, 280.f }, { 200.f, 200.f });
            }
            break;
        case BattleSystem::ALLY_CURSE:
            if (buffIndex > 0)
                battle_system->dmgVals.shouldDisplay = false;
            if (progress < 0.f) {
                buffIndex++;
                lastTimeEffect = time;
            }
            if (buffIndex > 2) {
                buffIndex = 0;
                lastTimeEffect = 0.0f;
                battle_system->selectedAnime = BattleSystem::NONE;
                // last character in action check
                battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
            }
            else {
                vec2 size = vec2(150.f);
                if (debuffIds[buffIndex] == TEXTURE_ASSET_ID::DEF_BUFF_EFFECT)
                    size = vec2(180.f);
                drawSpriteEffect(debuffIds[buffIndex], EFFECT_ASSET_ID::EFFECT, vec4(progress), {window_width_px / 2, 280.f - 20.f * progress }, size);
            }
            break;
        case BattleSystem::ALLY_DOOM:
            if (progress < 0.f) {
                buffIndex++;
                lastTimeEffect = time;
                lastTimeText = time;
            }
            //the parts below are pretty hard coded. Only work for this skill
            if (battle_system->allMembers[battle_system->currMemberIndex].followUpDmg.size() > 0 && buffIndex == 1) {
                battle_system->dmgVals.value = battle_system->allMembers[battle_system->currMemberIndex].followUpDmg[0];
                battle_system->createParticles(BattleSystem::ALLY_DOOM);
            }
            if (buffIndex > battle_system->allMembers[battle_system->currMemberIndex].followUpDmg.size()) {
                buffIndex = 0;
                lastTimeEffect = 0.0f;
                battle_system->selectedAnime = BattleSystem::NONE;
                // last character in action check
                battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
            }
            else if (buffIndex == 0) {
                drawSpriteEffect(TEXTURE_ASSET_ID::CURSE, EFFECT_ASSET_ID::EFFECT, vec4(1.f, 1.f, 1.f, progress), { window_width_px / 2, 130.f + (5.f / (progress + 0.5f)) }, {290.f, 290.f});
            }
            break;
        case BattleSystem::ALLY_DISTRACT:
            if (progress < 0.f) {
                if (progress < -.0f) {
                    lastTimeEffect = 0.0f;
                    battle_system->selectedAnime = BattleSystem::NONE;
                    battle_system->checkRoundOver(battle_system->allMembers[battle_system->currMemberIndex].actionIndex);
                }
            }
            else {
                drawSpriteEffect(TEXTURE_ASSET_ID::ATK_BUFF_EFFECT, EFFECT_ASSET_ID::EFFECT, vec4(progress), { window_width_px / 2, 280.f - 20.f * progress }, vec2(200.f));
            }
            break;
    }
}

mat3 RenderSystem::createProjectionMatrix()
{
    // Fake projection matrix, scales with respect to window coordinates
    float left = 0.f;
    float top = 0.f;

    gl_has_errors();
    float right = (float) window_width_px;
    float bottom = (float) window_height_px;

    float sx = 2.f / (right - left);
    float sy = 2.f / (top - bottom);
    float tx = -(right + left) / (right - left);
    float ty = -(top + bottom) / (top - bottom);
    return {{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};
}

void RenderSystem::createEnemy() {
    if (!battle_system->initialized)
        return;
    //create enemy hp bar
    ImVec2 position = ImVec2(900.f * scale_x, 350.f * scale_y);
    ImVec2 size = ImVec2(150.f * scale_x, 70.f * scale_y);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, battle_yellow_colour);
    ImGui::PushStyleColor(ImGuiCol_Border, ui_border_colour);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.f);
    ImGui::SetNextWindowPos(position); // set block position
    ImGui::SetNextWindowSize(size); // set block size
    ImGui::Begin("Enemy HP Bar", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    //ImGui::SetWindowFontScale(1.2f);
    ImGui::SetCursorPos(ImVec2(10.f * scale_x, 10.f * scale_y));
    ImGui::Text(battle_system->enemySpecies.name.c_str());
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    TEXTURE_ASSET_ID enemy_fish;
    switch (battle_system->enemySpecies.id) {
    case (int)FISH_TEXTURE_ASSET_ID::SLEEP_NARWHAL:
        enemy_fish = TEXTURE_ASSET_ID::SLEEP_NARWHAL;
        break;
    case (int)FISH_TEXTURE_ASSET_ID::TURTLE:
        enemy_fish = TEXTURE_ASSET_ID::TURTLE;
        break;
    case (int)FISH_TEXTURE_ASSET_ID::WALRUS:
        enemy_fish = TEXTURE_ASSET_ID::WALRUS;
        break;
    case (int)FISH_TEXTURE_ASSET_ID::INKWHALE:
        enemy_fish = TEXTURE_ASSET_ID::INKWHALE;
        break;
    }

    if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_EFFECT_PLAYING) {
        snprintf(hpTextEnemy, sizeof(hpTextEnemy), "%.0f/%.0f", battle_system->enemySpecies.health, battle_system->enemyMaxHealth);
        hpFillEnemy = battle_system->enemySpecies.health / battle_system->enemyMaxHealth;
        float time = (float)glfwGetTime();
        float elapsedTime;
        float progress;
        if (lastTimeText == 0.0f || time - lastTimeText < 0) {
            lastTimeText = time;
        }
        elapsedTime = time - lastTimeText;
        if (elapsedTime > 1.f) {
            lastTimeText = 0.f;
        }
        if (!battle_system->dmgVals.targetPlayer && battle_system->dmgVals.shouldDisplay == true) {
            drawDmgText(std::to_string(static_cast<int>(battle_system->dmgVals.value)).c_str(), ImVec2((950.f + 60 * elapsedTime) * scale_x, (310.f - 20 * elapsedTime + 60.f * elapsedTime * elapsedTime) * scale_y), battle_system->dmgVals.textColour);
        }
    }

    drawList->AddRectFilled(ImVec2(position.x + 10.f * scale_x, position.y + 40.f * scale_y), ImVec2(position.x + 140.f * scale_x, position.y + 60.f * scale_y), IM_COL32(59, 94, 126, 255));
    drawList->AddRectFilled(ImVec2(position.x + 10.f * scale_x, position.y + 40.f * scale_y), ImVec2(position.x + 140.f * hpFillEnemy * scale_x, position.y + 60.f * scale_y), IM_COL32(61, 235, 154, 255));
    drawList->AddText(ImVec2(position.x + 27.5f * scale_x, position.y + 36.f * scale_y), IM_COL32(0, 0, 0, 255), hpTextEnemy);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::End();

    //create sprite
    if ((battle_system->selectedAnime == BattleSystem::PLAYER_ATK || battle_system->selectedAnime == BattleSystem::ALLY_MANIFEST || 
        battle_system->selectedAnime == BattleSystem::ALLY_CURSE || battle_system->selectedAnime == BattleSystem::ALLY_DOOM || 
        battle_system->selectedAnime == BattleSystem::ALLY_DISTRACT || battle_system->selectedAnime == BattleSystem::ALLY_ATK) && 
        battle_system->curr_battle_state == BattleSystem::STATE_EFFECT_PLAYING) {
        playEnemyAnime(battle_system->selectedAnime);
    }
    else if ((battle_system->selectedAnime == BattleSystem::ENEMY_ATK || battle_system->selectedAnime == BattleSystem::ENEMY_DEBUFF || 
        battle_system->selectedAnime == BattleSystem::ENEMY_EXECUTION) && battle_system->curr_battle_state == BattleSystem::STATE_ENEMY_ACTING) {
        playEnemyAnime(battle_system->selectedAnime);
    }
    else if (battle_system->curr_battle_state == BattleSystem::STATE_ENEMY_DEFEATED) {
        float alpha = 1.f;
        float time = (float)glfwGetTime();
        if (lastTimeEnemy == 0.0f) {
            lastTimeEnemy = (float)time;
        }
        float elapsedTime = time - lastTimeEnemy;
        float duration = 2.f;
        if (elapsedTime < duration) {
            alpha -= elapsedTime;
        }
        if ((time - lastTimeEnemy) > 2.f) {
            lastTimeEnemy = 0.0f;
            battle_system->reset();
            return;
        }
        ImVec2 spriteSize = ImVec2(390.f, 390.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::SetNextWindowPos(ImVec2((window_width_px / 2 - spriteSize.x / 2) * scale_x, 75.f * scale_y)); // set block position
        ImGui::SetNextWindowSize(spriteSize); // set block size
        ImGui::Begin("Enemy", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        ImVec4 imageColor = ImVec4(1.0f, 1.0f, 1.0f, alpha);
        ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)enemy_fish], spriteSize, ImVec2(0, 0), ImVec2(1, 1), imageColor);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::End();
    }
    else
    {
        float time = (float) glfwGetTime();
        float oscillationValue = 0.25f * std::sin(2.f * M_PI / 4.f * time) + 1.5f;
        ImVec2 spriteSize = ImVec2(390.f, 390.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::SetNextWindowPos(ImVec2((window_width_px / 2 - spriteSize.x / 2) * scale_x, 55.f * oscillationValue * scale_y)); // set block position
        ImGui::SetNextWindowSize(spriteSize); // set block size
        ImGui::Begin("Enemy", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        // get the texture that does not have stars

        ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)enemy_fish], spriteSize);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::End();
    }
}

void RenderSystem::createHPBar() {
	//hp text
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::SetNextWindowPos(ImVec2(265.f * scale_x, 820.f * scale_y)); // set block position
	ImGui::SetNextWindowSize(ImVec2(80.f * scale_x, 40.f * scale_y)); // set block size
	ImGui::Begin("HP Text", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetWindowFontScale(1.8f);
	ImGui::SetCursorPos(ImVec2(5.f * scale_x, 0.f));
	ImGui::Text("HP:");
	ImGui::End();

	//hp bar
	ImVec2 position = ImVec2(365.f * scale_x, 830.f * scale_y);
	ImVec2 size = ImVec2(1095.f * scale_x, 25.f * scale_y);
	ImGui::SetNextWindowPos(position); // set block position
	ImGui::SetNextWindowSize(size); // set block size
	ImGui::Begin("HP Bar", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_EFFECT_PLAYING) {
        hpFill = battle_system->currHealth / battle_system->maxHealth;
        snprintf(hpText, sizeof(hpText), "%.0f/%.0f", battle_system->currHealth, battle_system->maxHealth);
        float time = (float)glfwGetTime();
        float elapsedTime;
        float progress;
        if (lastTimeText == 0.0f) {
            lastTimeText = time;
        }
        elapsedTime = time - lastTimeText;
        if (elapsedTime > 2.f)
            lastTimeText = 0.f;
        if (battle_system->dmgVals.targetPlayer && battle_system->dmgVals.shouldDisplay == true) {
            drawDmgText(std::to_string(static_cast<int>(battle_system->dmgVals.value)).c_str(), ImVec2((300.f + 60 * elapsedTime) * scale_x, (760.f - 20 * elapsedTime + 60.f * elapsedTime * elapsedTime) * scale_y), battle_system->dmgVals.textColour);
        }
    }
	drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), IM_COL32(59, 94, 126, 255));
	drawList->AddRectFilled(position, ImVec2(position.x + size.x * hpFill, position.y + size.y), IM_COL32(61, 235, 154, 255));
	//ImGui::SetWindowFontScale(1.2f);

	drawList->AddText(ImVec2(position.x + 10.f * scale_x, position.y), IM_COL32(0, 0, 0, 255), hpText);
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::End();
}

void RenderSystem::createCharacterPortrait(ImVec2 position, int index) {
	ImGui::PushStyleColor(ImGuiCol_WindowBg, battle_yellow_colour); //yellow part of battle ui
	ImGui::PushStyleColor(ImGuiCol_Border, ui_border_colour); // brown border part
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.f); // brown border thickness
	ImGui::SetNextWindowPos(position); // set block position
	ImGui::SetNextWindowSize(ImVec2(195.f * scale_x, 205.f * scale_y)); // set block size
    std::string text = "Character portrait" + std::to_string(index);
	ImGui::Begin(text.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    std::vector<Entity> members = registry.partyMembers.entities;
    PartyMember p = registry.partyMembers.get(members[index]);

    ImVec2 textSize = ImGui::CalcTextSize(p.name.c_str());
	//ImGui::SetWindowFontScale(1.5f);
    ImGui::SetCursorPos(ImVec2((100.f * scale_x - textSize.x * scale_x / 2.f), 4.f * scale_y));
	ImGui::Text(p.name.c_str());
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(position.x + 27.f * scale_x, position.y + 30.f * scale_y), ImVec2(position.x + 166.f * scale_x, position.y + 166.f * scale_y), IM_COL32_WHITE);
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::End();
}

void RenderSystem::drawPortraitsAnime(float xpos, TEXTURE_ASSET_ID asset_id) {
    float time = (float)glfwGetTime();
    float frameDuration = 0.7f; // Time in seconds for each frame
    int numFrames = 2;
    int currentFrame = static_cast<int>(time / frameDuration) % numFrames;
    drawSpriteAnime(currentFrame, 1, 2, asset_id, { xpos, 574.f }, { 135.f, 135.f });
}

void RenderSystem::createSkillsTable() {
    std::vector<std::string> actionNames;
    std::vector<std::string> actionDescription;

    if (battle_system->currMemberIndex != -1 && battle_system->allMembers[battle_system->currMemberIndex].skills.size() != 0) {
        for (int i = 0; i < battle_system->allMembers[battle_system->currMemberIndex].skills.size(); i++) {
            actionNames.push_back(battle_system->allMembers[battle_system->currMemberIndex].skills[i].skill_name);
            actionDescription.push_back(battle_system->allMembers[battle_system->currMemberIndex].skills[i].skill_description);
        }
    }
    else {
        return;
    }
	const int numColumns = 2; // Number of columns in the grid
	// select skill using keyboard
	//if (ImGui::IsKeyPressed(ImGuiKey_W)) {
	//	if (skillSelected[2] == true) {
	//		skillSelected[2] = false;
	//		skillSelected[0] = true;
	//	}
	//	else if (skillSelected[3] == true) {
	//		skillSelected[3] = false;
	//		skillSelected[1] = true;
	//	}
	//}
	//else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
	//	if (skillSelected[0] == true && actionNames.size() > 2) {
	//		skillSelected[0] = false;
	//		skillSelected[2] = true;
	//	}
	//	else if (skillSelected[1] == true && actionNames.size() > 3) {
	//		skillSelected[1] = false;
	//		skillSelected[3] = true;
	//	}
	//}
	//else if (ImGui::IsKeyPressed(ImGuiKey_A)) {
	//	if (skillSelected[1] == true) {
	//		skillSelected[1] = false;
	//		skillSelected[0] = true;
	//	}
	//	else if (skillSelected[3] == true) {
	//		skillSelected[3] = false;
	//		skillSelected[2] = true;
	//	}
	//}
	//else if (ImGui::IsKeyPressed(ImGuiKey_D)) {
	//	if (skillSelected[0] == true && actionNames.size() > 1) {
	//		skillSelected[0] = false;
	//		skillSelected[1] = true;
	//	}
	//	else if (skillSelected[2] == true && actionNames.size() > 3) {
	//		skillSelected[2] = false;
	//		skillSelected[3] = true;
	//	}
	//}
 //   else if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
 //       if (battle_system->curr_battle_state == BattleSystem::StateEnum::STATE_PLAYER_TURN) {
 //           for (int i = 0; i < actionNames.size(); i++) {
 //               if (skillSelected[i]) {
 //                   battle_system->selectedSkill = i;
 //                   //std::cout << "Skill " << ("%d\n", i+1) << " was selected!" << std::endl;
 //                   //std::cout << battle_system->currMember.stats.attack << std::endl;
 //               }
 //           }
 //       }
 //   }

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
	ImGui::SetNextWindowPos(ImVec2(270.f * scale_x, 705.f * scale_y)); // set block position
	ImGui::SetNextWindowSize(ImVec2(1183.f * scale_x, 105.f * scale_y)); // set block size
	ImGui::Begin("Skills Options", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	for (int i = 0; i < actionNames.size(); ++i) {
		if (i % numColumns != 0) {
			ImGui::SameLine(); // Start a new row
		}
        //for select skill using keyboard
		/*if (skillSelected[i]) {
			ImGui::PushStyleColor(ImGuiCol_Button, battle_blue_highlighted);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, battle_blue_highlighted);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, battle_blue_highlighted);
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button, battle_blue_colour);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, battle_blue_colour);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, battle_blue_colour);
		}*/

		ImGui::PushID(i);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_Button, battle_blue_colour);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, battle_blue_highlighted);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, battle_blue_highlighted);
		//ImGui::SetWindowFontScale(1.2f);
		if (ImGui::Button(actionNames[i].c_str(), ImVec2(595.f * scale_x, 48.f * scale_y))) {
			// this is for clicks, do nothing
            if (battle_system->curr_battle_state == BattleSystem::STATE_PLAYER_TURN) {
                battle_system->selectedSkill = i;
                //std::cout << "Skill " << ("%d\n", i+1) << " was selected!" << std::endl;
                //std::cout << battle_system->currMember.stats.attack << std::endl;
            }
		}
        ImGui::PopStyleColor(3);
		ImGui::PopID();
		ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4.f);

        if (ImGui::IsItemHovered()) {
            ImGui::SetNextWindowSize(ImVec2(650.f, 80.f));
            ImGui::BeginTooltip();
            ImGui::SetWindowFontScale(0.7f);
            ImGui::TextWrapped(actionDescription[i].c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopStyleVar(2);
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);
	ImGui::End();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, battle_blue_colour); //blue part of battle ui
    ImGui::PushStyleColor(ImGuiCol_Border, border_blue_colour); // blue border part
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.0f); // blue border thickness
    ImGui::SetNextWindowPos(ImVec2(265.f * scale_x, 700.f * scale_y)); // set block position
    ImGui::SetNextWindowSize(ImVec2(1195.f * scale_x, 110.f * scale_y)); // set block size
    ImGui::Begin("Skills Table", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::End();

	createBattleLog();
}

void RenderSystem::createBattleLog() {
    if (!battle_system->initialized)
        return;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, battle_blue_colour); //blue part of battle ui
	ImGui::PushStyleColor(ImGuiCol_Border, border_blue_colour); // blue border part
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.f); // blue border thickness
	ImGui::SetNextWindowPos(ImVec2(20.f * scale_x, 20.f * scale_y)); // set block position
	ImGui::SetNextWindowSize(ImVec2(1195.f * scale_x, 50.f * scale_y)); // set block size
	ImGui::Begin("Battle Log", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetWindowFontScale(1.2f);
	ImGui::SetCursorPos(ImVec2(5.f * scale_x, 5.f * scale_y));
    switch (battle_system->curr_battle_state) {
        case BattleSystem::StateEnum::STATE_PLAYER_TURN:
            battleLogText = battle_system->allMembers[battle_system->currMemberIndex].name + "'s turn";
            break;
        case BattleSystem::StateEnum::STATE_PLAYER_ACTING:
            battleLogText = battle_system->allMembers[battle_system->currMemberIndex].name + " is acting...";
            break;
        case BattleSystem::StateEnum::STATE_ENEMY_ACTING:
            if (battle_system->enemySelectedSkill != -1)
                battleLogText = battle_system->enemySpecies.name + " uses " + battle_system->enemy->skills[battle_system->enemySelectedSkill].skill_name;
            break;
        case BattleSystem::StateEnum::STATE_ROUND_BEGIN:
            battleLogText = "new round begins...";
            break;
        case BattleSystem::StateEnum::STATE_ENEMY_DEFEATED:
            battleLogText = battle_system->enemySpecies.name + " has been defeated!";
            break;
        case BattleSystem::StateEnum::STATE_PLAYER_DEFEATED:
            battleLogText = "The fishing line has been broken!";
            break;
    }
    ImGui::Text(battleLogText.c_str());
    /*ImGui::SetCursorPos(ImVec2(5.f * scale_x, 25.f * scale_y));
    ImGui::Text(battleLogText.c_str());*/
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::End();
}

void RenderSystem::createMainUIButtonWindow(GAME_STATE_ID id, const ImVec2& position) {
    // set window transparent, no padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetCursorScreenPos(position);
    //ImGui::SetNextWindowPos(position); // set button position
    //ImGui::SetNextWindowSize(ImVec2(100.f * scale_x, 90.f * scale_y)); // set button size
    ImGui::PushID((int)id);

    if (ImGui::InvisibleButton("##Button", ImVec2(100.f * scale_x, 90.f * scale_y))) {
        ImVec2 mousePos = ImGui::GetMousePos();
        std::cout << (int)id << " X: " << mousePos.x << ", Y: " << mousePos.y << std::endl;
        *current_game_state = id;

        // TODO: later switch to a function open ui that accepts button id - use *current_game_state
    }
    bool isHovered = ImGui::IsItemHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    if (isHovered) {
        ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + 100.f * scale_x, position.y + 90.f * scale_y), backgroundColor, 4.0f);
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopID();
}

// Draw common menu items such as buttons and text
void RenderSystem::drawMenuItems()
{
    RenderSystem::drawOnMenu();
    // SAVE, FISH, PARTY, SHOP, SETTINGS
    createMainUIButtonWindow(GAME_STATE_ID::SAVE, ImVec2(869.f * scale_x, 762.f * scale_y));
    createMainUIButtonWindow(GAME_STATE_ID::INVENTORY, ImVec2(993.f * scale_x, 762.f * scale_y));
    createMainUIButtonWindow(GAME_STATE_ID::PARTY, ImVec2(1117.f * scale_x, 762.f * scale_y));
    createMainUIButtonWindow(GAME_STATE_ID::SHOP, ImVec2(1240.f * scale_x, 762.f * scale_y));
    createMainUIButtonWindow(GAME_STATE_ID::SETTINGS, ImVec2(1364.f * scale_x, 762.f * scale_y));
}

// Draw common menu textures
void RenderSystem::drawMenu()
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::Begin("Main UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetWindowSize(ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    // This is the wooden background image
    ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::MENU_BG], ImVec2(0.f, 0.f), ImVec2(window_width_px * scale_x, window_height_px * scale_y), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f));
    ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::MAINUI], ImVec2(window_width_px * scale_x, window_height_px * scale_y));

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

// Draw inventory UI
void RenderSystem::drawInventory()
{
    drawMenu();
    drawMenuItems();

    ImGui::SetCursorPos(ImVec2(100.f * scale_x, 50.f * scale_y));

    ImGuiStyle *style = &ImGui::GetStyle();

    // brute forcing buttons to have different color when pressed
    if (show_inventory == true)
    {
        inventory_button_active = true;
    }
    else
    {
        inventory_button_active = false;
    }

    if (inventory_button_active == true)
        ImGui::PushStyleColor(ImGuiCol_Button, style->Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button("Inventory", ImVec2(150.f * scale_x, 40.f * scale_y)))
    {
        show_inventory = true;
        show_fishing_log = false;
    }
    if (inventory_button_active == true)
        ImGui::PopStyleColor();

    ImGui::SameLine();

    if (inventory_button_active == false)
        ImGui::PushStyleColor(ImGuiCol_Button, style->Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button("Fishing Log", ImVec2(150.f * scale_x, 40.f * scale_y)))
    {
        show_fishing_log = true;
        show_inventory = false;
    }
    if (inventory_button_active == false)
        ImGui::PopStyleColor();

    ImGui::SetNextWindowPos(ImVec2(100.f * scale_x, 100.f * scale_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.5f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.0f);

    int num_columns = 12;
    float inventory_width = 1300.f * scale_x;
    float inventory_height = 550.f * scale_y;
    // 60.f appears to be the offset when factoring in drawing the dearimgui window, table, and cells of inventory
    // this isn't the most accurate tho...
    float offset = 60.f;
    float item_size = (inventory_width - offset) / (float) num_columns;
    if (show_inventory == true)
    {
        ImGui::Begin("Inventory UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::SetWindowSize(ImVec2(1300.f * scale_x, 550.f * scale_y));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(50, 10));
        ImGui::BeginTabBar("Inventory", 2);

        // loop through fishes in inventory
        if (ImGui::BeginTabItem("FISH"))
        {
            if (ImGui::BeginTable("Fish Inventory", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
            {
                ImGui::PushFont(font_small);
                int inventoryIndex = 0;
                auto &fish_inventory = registry.fishInventory;
                for (int row = 0; row < 6; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < num_columns; column++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        if (inventoryIndex < fish_inventory.size())
                        {
                            Fish &fish = fish_inventory.components[inventoryIndex];
                            ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));
                            FishSpecies species = id_to_fish_species.at(fish.species_id);
                            // different species should have different textures
                            ImGui::Image((void *)(intptr_t)fish_texture_gl_handles[(int)fish.species_id], ImVec2(item_size, item_size));

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip(species.name.c_str());
                            }
                            inventoryIndex++;
                        }
                        else
                        {
                            // ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + 103.f, p.y + 102.f), ImVec2(0, 0), ImVec2(1, 1));
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(item_size, item_size));
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Empty");
                            }
                        }
                    }
                }
                ImGui::PopFont();
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("LURES"))
        {
            if (ImGui::BeginTable("Lure Inventory", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
            {
                EquipLure& equipped = registry.luresEquipped.components[0]; // could be empty lure, name == ""
                // .get(fishingRod);
                int inventoryIndex = 0;
                auto &inventory = registry.lures;
                for (int row = 0; row < 6; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < num_columns; column++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        if (inventoryIndex < inventory.size())
                        {
                            Lure &lure = inventory.components[inventoryIndex];
                            if (inventoryIndex == equipped.lureIndex) {
                                ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL_SELECTED], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));
                            } else {
                                ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));
                            }
                            // different species should have different textures
                            // TODO: need actual lure textures
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[lure.texture_id], ImVec2(item_size, item_size));

                            if (ImGui::IsItemClicked()) {
                                if (lure.numOwned > 0) {
                                    if (equipped.lureIndex == inventoryIndex) {
                                        equipped.lureIndex = -1;
                                    } else {
                                        equipped.lureIndex = inventoryIndex;
                                    }
                                }
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip((lure.name + ": " + std::to_string(lure.numOwned)).c_str());
                            }
                            inventoryIndex++;
                        }
                        else
                        {
                            // ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + 103.f, p.y + 102.f), ImVec2(0, 0), ImVec2(1, 1));
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(item_size, item_size));
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Empty");
                            }
                        }
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("GIFT"))
        {
            if (ImGui::BeginTable("Gift Inventory", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
            {
                ImGui::PushFont(font_small);
                int inventoryIndex = 0;
                auto &inventory = registry.giftInventory;
                for (int row = 0; row < 6; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < num_columns; column++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        // This commented out code is how to set the background of the cell
                        /*ImVec2 p = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + 104.f, p.y + 102.f), ImVec2(0, 0), ImVec2(1, 1));*/
                        if (inventoryIndex < inventory.size())
                        {
                            Gift &gift = inventory.components[inventoryIndex];
                            ImVec2 p = ImGui::GetCursorScreenPos();
                            ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));
                            GiftType species = id_to_gift_type.at(gift.type);
                            // different species should have different textures
                            ImGui::Image((void *)(intptr_t)fish_texture_gl_handles[(int)species.id], ImVec2(item_size, item_size));

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip(species.name.c_str());
                            }

                            inventoryIndex++;
                        }
                        else
                        {
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(item_size, item_size));
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Empty");
                            }
                        }
                    }
                }
                ImGui::PopFont();
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::PopStyleVar();
    }
    if (show_fishing_log == true)
    {
        ImGui::Begin("Inventory UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::SetWindowSize(ImVec2(1300.f * scale_x, 550.f * scale_y));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(50, 10));
        ImGui::BeginTabBar("Fish Log", 2);

        if (ImGui::BeginTabItem("ALL"))
        {
            if (ImGui::BeginTable("All Fish", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
            {
                ImGui::PushFont(font_small);
                std::map<int, int> fishSpeciesCount;
                auto &fishingLog = registry.fishingLog;
                for (uint i = 0; i < fishingLog.size(); i++)
                {
                    FishingLog &fish = fishingLog.components[i];
                    ++fishSpeciesCount[fish.species_id];
                }

                for (int row = 0; row < 6; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < 12; column++)
                    {
                        int cell_count = row * 12 + column;
                        ImGui::TableSetColumnIndex(column);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));

                        if (fishSpeciesCount.find(cell_count) == fishSpeciesCount.end())
                        {
                            // not found
                            // ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(103.f, 102.f));
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::CHINOOK_SHADOW], ImVec2(item_size, item_size));
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("???");
                            }
                        }
                        else
                        {
                            // found
                            FishSpecies species = id_to_fish_species.at(cell_count);
                            // different species should have different textures
                            ImGui::Image((void *)(intptr_t)fish_texture_gl_handles[(int)cell_count], ImVec2(item_size, item_size));

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip(species.name.c_str());
                            }
                        }
                    }
                }
                ImGui::PopFont();
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("LAKE 1"))
        {
            if (ImGui::BeginTable("Lake 1 Fish", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
            {
                ImGui::PushFont(font_small);
                std::map<int, int> fishSpeciesCount;
                auto &inventory = registry.fishingLog;
                for (uint i = 0; i < inventory.size(); i++)
                {
                    FishingLog &fish = inventory.components[i];
                    if (fish.lake_id == 1)
                    { // TODO: unhardcode this
                        ++fishSpeciesCount[fish.species_id];
                    }
                }
                for (int row = 0; row < 6; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < num_columns; column++)
                    {
                        // ImGui::TableSetColumnIndex(column);
                        // // This commented out code is how to set the background of the cell
                        // /*ImVec2 p = ImGui::GetCursorScreenPos();
                        // ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + 104.f, p.y + 102.f), ImVec2(0, 0), ImVec2(1, 1));*/
                        // ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(103.f, 102.f));
                        int cell_count = row * num_columns + column;
                        ImGui::TableSetColumnIndex(column);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));

                        if (fishSpeciesCount.find(cell_count) == fishSpeciesCount.end())
                        {
                            // not found
                            // ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(103.f, 102.f));
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::CHINOOK_SHADOW], ImVec2(item_size, item_size));
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("???");
                            }
                        }
                        else
                        {
                            // found
                            FishSpecies species = id_to_fish_species.at(cell_count);
                            // different species should have different textures
                            ImGui::Image((void *)(intptr_t)fish_texture_gl_handles[(int)cell_count], ImVec2(item_size, item_size));

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip(species.name.c_str());
                            }
                        }
                    }
                }
                ImGui::PopFont();
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("LAKE 2"))
        {
            if (ImGui::BeginTable("Lake 2 Fish", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
            {
                ImGui::PushFont(font_small);
                std::map<int, int> fishSpeciesCount;
                auto &inventory = registry.fishingLog;
                for (uint i = 0; i < inventory.size(); i++)
                {
                    FishingLog &fish = inventory.components[i];
                    if (fish.lake_id == 2)
                    { // TODO: unhardcode this
                        ++fishSpeciesCount[fish.species_id];
                    }
                }
                for (int row = 0; row < 6; row++)
                {
                    ImGui::TableNextRow();
                    for (int column = 0; column < num_columns; column++)
                    {
                        // ImGui::TableSetColumnIndex(column);
                        // // This commented out code is how to set the background of the cell
                        // /*ImVec2 p = ImGui::GetCursorScreenPos();
                        // ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + 104.f, p.y + 102.f), ImVec2(0, 0), ImVec2(1, 1));*/
                        // ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(103.f, 102.f));
                        int cell_count = row * num_columns + column;
                        ImGui::TableSetColumnIndex(column);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], p, ImVec2(p.x + item_size, p.y + item_size), ImVec2(0, 0), ImVec2(1, 1));

                        if (fishSpeciesCount.find(cell_count) == fishSpeciesCount.end())
                        {
                            // not found
                            // ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::ITEM_CELL], ImVec2(103.f, 102.f));
                            ImGui::Image((void *)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::CHINOOK_SHADOW], ImVec2(item_size, item_size));
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("???");
                            }
                        }
                        else
                        {
                            // found
                            FishSpecies species = id_to_fish_species.at(cell_count);
                            // different species should have different textures
                            ImGui::Image((void *)(intptr_t)fish_texture_gl_handles[(int)cell_count], ImVec2(item_size, item_size));

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip(species.name.c_str());
                            }
                        }
                    }
                }
                ImGui::PopFont();
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::PopStyleVar();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    drawExitButton();
    // ends the main UI window
    ImGui::End();
}

// draws the Shop UI
void RenderSystem::drawShop()
{
    Entity player = registry.players.entities[0];
    Entity fishingRod = registry.fishingRods.entities[0];
    json shopkeep_dialogue = dialogue["shopkeeper"];

    drawMenu();
    drawMenuItems();

    ImGui::SetNextWindowPos(ImVec2(100* scale_x, 50 * scale_y));
    ImGui::SetNextWindowSize(ImVec2(850.f * scale_x, 600.f * scale_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.5f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.0f);

    ImGui::Begin("Shop UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(50, 10));

    ImGui::BeginTabBar("Shop", 3);
    if (ImGui::BeginTabItem("Fishing Rods"))
    {
        if (ImGui::BeginTable("Rods", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
        {
            const char* messages[] = {
                    "Durability +5",
                    "Attack     +5", };
            const float durabilityIncrease[] = { 5.f, 0.f };
            const int attackIncrease[] = { 0, 5 };
            const int price[] = { 14, 16 };
            int numFishingRodOptions = sizeof(attackIncrease)/sizeof(attackIncrease[0]);
            for (int row = 0; row < numFishingRodOptions; row++)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 10));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.f);

                ImGui::TableNextRow(0, 100.f * scale_y);
                ImGui::TableSetColumnIndex(0);
                setDrawCursorScreenPos(ImVec2(10.f, 10.f));
                std::string buyMessage(messages[row]);
                ImGui::Text(buyMessage.c_str(), price[row]);
                setDrawCursorScreenPos(ImVec2(10.f, 0.f));
                ImGui::Text("%d gold", price[row]);
                // Put button to the left of shop window (hard coded)
                setDrawCursorScreenPos(ImVec2(730.f, -40.f));

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                Wallet& wallet = registry.wallet.get(player);
                Durability& durability = registry.durabilities.get(fishingRod);
                Attack& attack = registry.attacks.get(fishingRod);

                bool disable = false;
                if (row == 0) {
                    disable = durability.num_upgrades == durability.max_upgrade;
                }
                else if (row == 1) {
                    disable = attack.num_upgrades == attack.max_upgrade;
                }
                ImGui::BeginDisabled(disable);
                std::string buttonName = "BUY##" + std::to_string(row);
                if (disable) {
                    buttonName = "MAX##" + std::to_string(row);
                }
                if (ImGui::Button(buttonName.c_str(), ImVec2(70.f * scale_x, 30.f * scale_y)))
                {
                    Wallet& wallet = registry.wallet.get(player);
                    if (wallet.gold < price[row])
                    {
                        shop_state = SHOP_STATE::BUY_NO_MONEY;
                        sound_system->playSound(sound_system->denied);
                        // handle not enough gold
                    } else {
                        shop_state = SHOP_STATE::BUY;
                        sound_system->playSound(sound_system->chaching);
                        wallet.gold -= price[row];
                        // replace current fishing rod with this
                        durability.max += durabilityIncrease[row];
                        durability.current += durabilityIncrease[row];
                        if (row == 0) {
                            durability.num_upgrades++;
                        }

                        attack.damage += attackIncrease[row];
                        if (row == 1) {
                            attack.num_upgrades++;
                        }
                    }
                }
                ImGui::EndDisabled();
                ImGui::PopStyleVar(3);
            }
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Fishing Lures"))
    {
        if (ImGui::BeginTable("Lures", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
        {
            // const char* messages[] = {
            //         "Lure #1 - Catch rate up +5%",
            //         "Lure #2 - Catch rate up +10%", };
            // const Lure lures[] = { { "Lure #1" }, { "Lure #1", { 10.f } } };
            // const Entity lureEntities[] = { registry.lures.entities[0], registry.lures.entities[1] };

            // const float charmIncrease[] = { 5.f, 10.f };
            // const int price[] = { 3, 10 };
            for (int row = 0; row < registry.lures.size(); row++)
            {
                Lure& lure = registry.lures.components[row];
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 10));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.f);

                ImGui::TableNextRow(0, 100.f * scale_y);
                ImGui::TableSetColumnIndex(0);

                setDrawCursorScreenPos(ImVec2(10.f, 10.f));
                std::string buyMessage = lure.description + std::string(" - 10 pack");
                ImGui::TextUnformatted(buyMessage.c_str());
                setDrawCursorScreenPos(ImVec2(10.f, 0.f));
                ImGui::Text("%d gold", lure.price);
                // Put button to the left of shop window (hard coded)
                setDrawCursorScreenPos(ImVec2(730.f, -40.f));

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

                std::string buttonName = "BUY##" + std::to_string(row);
                if (ImGui::Button(buttonName.c_str(), ImVec2(70.f * scale_x, 30.f * scale_y)))
                {
                    Wallet& wallet = registry.wallet.get(player);
                    if (wallet.gold < lure.price)
                    {
                        shop_state = SHOP_STATE::BUY_NO_MONEY;
                        sound_system->playSound(sound_system->denied);
                        // TODO: handle not enough gold
                    } else {
                        shop_state = SHOP_STATE::BUY;
                        sound_system->playSound(sound_system->chaching);
                        wallet.gold -= lure.price;
                        lure.numOwned += 10;
                    }
                }
                ImGui::PopStyleVar(3);
            }
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("SELL"))
    {
        if (ImGui::BeginTable("Sellable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PreciseWidths))
        {
            auto &inventory = registry.fishInventory;
            for (uint row = 0; row < inventory.size(); row++)
            {
                Fish &fish = inventory.components[row];
                Entity& fish_entity = inventory.entities[row];
                FishSpecies species = id_to_fish_species.at(fish.species_id);
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 10));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.f);
                ImGui::TableNextRow(0, 100.f * scale_y);
                ImGui::TableSetColumnIndex(0);
                setDrawCursorScreenPos(ImVec2(10.f, 10.f));
                ImGui::Text("#%d. %s", row + 1, species.name.c_str());
                setDrawCursorScreenPos(ImVec2(10.f, 0.f));
                ImGui::Text("%s gold", std::to_string(species.price).c_str());
                // Put button to the left of shop window (hard coded -- will need to adjust based on whats drawn)
                setDrawCursorScreenPos(ImVec2(730.f, -40.f));

                std::string buttonName = "SELL##" + std::to_string(row);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                if (ImGui::Button(buttonName.c_str(), ImVec2(70.f * scale_x, 30.f * scale_y)))
                {
                    shop_state = SHOP_STATE::SELL;
                    sound_system->playSound(sound_system->sell);
                    // remove fish from inventory and add money to wallet B)
                    registry.fishInventory.remove(fish_entity); // TODO: add money to wallet, remove
                    Wallet& wallet = registry.wallet.get(player);
                    wallet.gold += species.price;
                }
                ImGui::PopStyleVar(3);
            }
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::PopStyleVar();

    ImGui::End();

    // Image of shopkeeper and text on the right of the shop UI
    ImGui::SetCursorScreenPos(ImVec2(985.f * scale_x, 40.f * scale_y));
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::SHOPKEEPER], p, ImVec2(p.x + 400.f * scale_x, p.y + 400.f * scale_y), ImVec2(0, 0), ImVec2(1, 1));

    ImGui::SetNextWindowPos(ImVec2(975.f * scale_x, 400.f * scale_y));
    ImGui::SetNextWindowSize(ImVec2(425.f * scale_x, 250.f * scale_y));
    ImGui::Begin("Shop Text", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
    //ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(239.f / 255.f, 225.f / 255.f, 178.f / 255.f, 1.00f));
    ImGui::BeginChild("Shop Text", ImVec2(412.f * scale_x, 240.f * scale_y), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    std::string shop_text;
    switch (shop_state) {
        case SHOP_STATE::WELCOME:
            shop_text = shopkeep_dialogue.value("welcome", "error");
            break;
        case SHOP_STATE::BUY:
            shop_text = shopkeep_dialogue.value("buy", "error");
            break;
        case SHOP_STATE::SELL:
            shop_text = shopkeep_dialogue.value("sell", "error");
            break;
        case SHOP_STATE::BUY_NO_MONEY:
            shop_text = shopkeep_dialogue.value("broke", "error");
            break;
    }
    ImGui::TextWrapped(shop_text.c_str());
    ImGui::EndChild();
    //ImGui::PopStyleColor();
    ImGui::End();

    ImGui::PopStyleVar(2);
    drawExitButton();
    // ends the main UI window
    ImGui::End();
}

void RenderSystem::drawExitButton() {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30);
    ImGui::PushFont(font_large);
    ImGui::SetCursorScreenPos(ImVec2((window_width_px - 80.f) * scale_x, 40.f * scale_y));
    if (ImGui::Button("X", ImVec2(45.f * scale_x, 45.f * scale_y)))
    {
        *current_game_state = GAME_STATE_ID::WORLD;
    }
    ImGui::PopFont();
    ImGui::PopStyleVar();
}

void RenderSystem::drawParty() {
    drawMenu();
    drawMenuItems();

    ImGui::SetNextWindowPos(ImVec2(100.f * scale_x, 50.f * scale_y));
    ImGui::SetNextWindowSize(ImVec2(850.f * scale_x, 600.f * scale_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.5f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.0f);

    ImGui::Begin("Party UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    auto& party_container = registry.partyMembers;
    int num_party = party_container.size();

    if (ImGui::BeginTable("Party Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
    {
        for (int row = 0; row < num_party; row++)
        {
            PartyMember& party_member = party_container.components[row];

            ImGui::TableNextRow(0, 140.f * scale_y);
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(row);

            // invisible button ends up overlapping with any party member buttons, needs to be shortened to 700 for non-MC
            float button_width;
            if (row == 0) {
                button_width = 850.f;
            }
            else {
                button_width = 700.f;
            }

            if (ImGui::InvisibleButton("", ImVec2(button_width * scale_x, 140.f * scale_y), 0)) {
                selected_character_detail = row;
            }
            setDrawCursorScreenPos(ImVec2(0.f, -140.f));
            if (ImGui::IsItemHovered() || selected_character_detail == row) {
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 850.f * scale_x, p.y + 135.f * scale_y), IM_COL32(235, 202, 125, 255));
            }
            setDrawCursorScreenPos(ImVec2(8.f, 8.f));

            ImGui::PushID(row);
            ImGui::BeginChild("PortraitBox", ImVec2(120.f*scale_x, 120.f*scale_y), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)party_member.menu_texture_id], ImVec2(107.f * scale_x, 107.f * scale_y));
            ImGui::EndChild();
            ImGui::PopID();

            setDrawCursorScreenPos(ImVec2(140.f, -120.f));

            ImGui::PopID();
            ImGui::Text(party_member.name.c_str());

            // party members have the talk and gift buttons
            if (row != 0) {
                setDrawCursorScreenPos(ImVec2(700.f, -5.f));
                //Put button to the left of shop window (hard coded)
                //setDrawCursorScreenPos(ImVec2(700.f, 0));
                ImGui::PushFont(font_small);
                ImGui::PushID(row);
                if (ImGui::Button("Talk", ImVec2(100.f * scale_x, 30.f * scale_y))) {
                    // Open a dialogue
                    
                    Dialogue& dialogue = registry.dialogues.emplace(player);
                    std::string cutscene_id = "ally_" + std::to_string(row) + "_talk_1";
                    if (row == 1 && registry.players.get(player).lake1_boss_defeated) {
                        cutscene_id = "ally_" + std::to_string(row) + "_talk_2";
                    }
                    dialogue.cutscene_id = cutscene_id;
                    dialogue.next_game_state = GAME_STATE_ID::PARTY;
                    *current_game_state = GAME_STATE_ID::CUTSCENE;
                }
                ImGui::PopID();

                //setDrawCursorScreenPos(ImVec2(700.f, 5.f));

                //ImGui::PushID(row);
                //if (ImGui::Button("Gift", ImVec2(100.f * scale_x, 30.f * scale_y))) {
                //    // Give gift to member
                //}
                //ImGui::PopID();
                ImGui::PopFont();
            }
        }
        ImGui::EndTable();
    }
    // Pop up for adding members to party
    //ImGui::SetNextWindowPos(ImVec2((1500.f/2.f - 250.f) * scale_x, 100.f * scale_y));
    //ImGui::SetNextWindowSize(ImVec2(500.0f * scale_x, 600.0f * scale_y));
    //if (show_party_list == true) {
    //    ImGui::OpenPopup("Party Select", ImGuiPopupFlags_NoOpenOverExistingPopup);
    //}
    //if (ImGui::BeginPopupModal("Party Select", NULL, ImGuiWindowFlags_NoResize)) {
    //    drawPartyListPopup(selected_character_add);
    //    ImGui::EndPopup();
    //}

    drawCharacterDetail(selected_character_detail);

    ImGui::End();

    ImGui::PopStyleVar(2);
    drawExitButton();
    ImGui::End();
}

void RenderSystem::drawSettingsMain(const char* id, TEXTURE_ASSET_ID tex_id) {
    // Draw main UI
    // Note: Don't call drawMenu() here, it breaks something in this function.
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Main UI2", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetWindowSize(ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::MAINUI], ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    drawMenuItems();

    // Draw main tutorial screen
    ImGui::SetNextWindowPos(ImVec2(10 * scale_x, 10 * scale_y));
    ImGui::SetNextWindowSize(ImVec2(((float)window_width_px - 25.f) * scale_x, ((float)window_height_px - 190.f) * scale_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)tex_id], ImVec2(window_width_px * scale_x - 25.f * scale_x, window_height_px * scale_y - 190.f * scale_y));


    // Draw exit button
    createTutorialExitButton("Exit Button 1", "##ExitButton1", ImVec2(1385 * scale_x, 25 * scale_y));
    ImGui::End();

    ImGui::PopStyleVar(3);

    // End for main ui
    ImGui::End();
    ImGui::PopStyleColor();


}

// Function for drawing the various tutorial screens (besides the main one)
void RenderSystem::drawTutorialScreen(const char* id, const int texture_id) {
    // Draw main UI
    // Note: Don't call drawMenu() here, it breaks something in this function.
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Main UI3", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetWindowSize(ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::MAINUI], ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    drawMenuItems();

    // Draw tutorial screen
    ImGui::SetNextWindowPos(ImVec2(13.5f * scale_x, 13.5f * scale_y));
    ImGui::SetNextWindowSize(ImVec2(1470.f * scale_x, 705.f * scale_y));
    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Image((void*)(intptr_t)texture_id, ImVec2(1470 * scale_x,705 * scale_y));
    ImGui::End();

    // End for main ui
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    createTutorialExitButton("Exit Button 2", "##ExitButton2",ImVec2(1385 * scale_x, 25 * scale_y));
    createTutorialBackButton("Back Button", "##BackButton",ImVec2(0, 0), GAME_STATE_ID::TUTORIAL);

}

void RenderSystem::createTutorialButtonWindow(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state) {

    // set window transparent, no padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(position); // set button position
    ImGui::SetNextWindowSize(ImVec2(1110.f * scale_x, 100.f * scale_y)); // set button size
    ImGui::PushID(id);

    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    bool isHovered = ImGui::IsWindowHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + 1110.f * scale_x, position.y + 100.f * scale_y), backgroundColor, 4.0f);
    bool buttonPressed = ImGui::InvisibleButton(button_id, ImVec2(1110.f * scale_x, 100.f * scale_y));
    if (buttonPressed) {
        if (game_state == GAME_STATE_ID::TUTORIAL_BASIC) {
            *current_game_state = GAME_STATE_ID::TUTORIAL_BASIC;
        } else if (game_state == GAME_STATE_ID::TUTORIAL_FISH) {
            *current_game_state = GAME_STATE_ID::TUTORIAL_FISH;
        } else if (game_state == GAME_STATE_ID::TUTORIAL_BATTLE) {
            *current_game_state = GAME_STATE_ID::TUTORIAL_BATTLE;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::PopID();
}

void RenderSystem::createSettingsButtonWindow(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state)
{
    // set window transparent, no padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(position); // set button position
    ImGui::SetNextWindowSize(ImVec2(920.f * scale_x, 100.f * scale_y)); // set button size
    ImGui::PushID(id);

    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    bool isHovered = ImGui::IsWindowHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + 920.f * scale_x, position.y + 100.f * scale_y), backgroundColor, 4.0f);
    bool buttonPressed = ImGui::InvisibleButton(button_id, ImVec2(920.f * scale_x, 100.f * scale_y));
    if (buttonPressed) {
        if (game_state == GAME_STATE_ID::SAVE) {
            *current_game_state = GAME_STATE_ID::SAVE;
        } else if (game_state == GAME_STATE_ID::LOAD) {
            *current_game_state = GAME_STATE_ID::LOAD;
        } else if (game_state == GAME_STATE_ID::TUTORIAL) {
            *current_game_state = GAME_STATE_ID::TUTORIAL;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::PopID();

}

void RenderSystem::createTutorialExitButton(const char* id, const char* button_id, const ImVec2& position) {
    // set window transparent, no padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(position); // set button position
    ImGui::SetNextWindowSize(ImVec2(50.f * scale_x, 50.f * scale_y)); // set button size
    ImGui::PushID(id);

    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    bool isHovered = ImGui::IsWindowHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(position.x + 25.f * scale_x, position.y + 25.f * scale_y), 25.f * scale_x, backgroundColor, 10);
    bool buttonPressed = ImGui::InvisibleButton(button_id, ImVec2(50.f * scale_x, 50.f * scale_y));
    if (buttonPressed) {
        *current_game_state = GAME_STATE_ID::WORLD;
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::PopID();
}

void RenderSystem::createTutorialBackButton(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID state_id) {
    // set window transparent, no padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(position); // set button position
    ImGui::SetNextWindowSize(ImVec2(200.f * scale_x, 100.f * scale_y)); // set button size
    ImGui::PushID(id);

    // Vertices for back button polygon
    ImVec2 points[] = {
            ImVec2(45 * scale_x, 50 * scale_y), // Left-most point
            ImVec2(75 * scale_x, 30 * scale_y), // Top-left corner
            ImVec2(175 * scale_x, 30 * scale_y), // Top-right corner
            ImVec2(175 * scale_x, 75 * scale_y), // Bottom-right corner
            ImVec2(75 * scale_x, 75 * scale_y) // Bottom-left corner
    };

    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    bool isHovered = ImGui::IsWindowHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    ImGui::GetWindowDrawList()->AddConvexPolyFilled(points, 5, backgroundColor);
    bool buttonPressed = ImGui::InvisibleButton(button_id, ImVec2(500.f * scale_x, 150.f * scale_y));
    if (buttonPressed) {
        if (state_id == GAME_STATE_ID::TUTORIAL) {
            *current_game_state = GAME_STATE_ID::TUTORIAL;
        } else if (state_id == GAME_STATE_ID::SETTINGS) {
            *current_game_state = GAME_STATE_ID::SETTINGS;
        }

    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::PopID();
}

void RenderSystem::createSaveButtons(const char* id, const char* button_id, const ImVec2& position, GAME_STATE_ID game_state)
{
    // set window transparent, no padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(position); // set button position
    ImGui::SetNextWindowSize(ImVec2(235.f * scale_x, 65.f * scale_y)); // set button size
    ImGui::PushID(id);

    ImGui::Begin(id, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    bool isHovered = ImGui::IsWindowHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + 235.f * scale_x, position.y + 65.f * scale_y), backgroundColor, 4.0f);
    bool buttonPressed = ImGui::InvisibleButton(button_id, ImVec2(235.f * scale_x, 65.f * scale_y));
    if (buttonPressed) {
        if (game_state == GAME_STATE_ID::SAVE_DONE) {
            *current_game_state = GAME_STATE_ID::SAVE_DONE;
        } else if (game_state == GAME_STATE_ID::SETTINGS) {
            *current_game_state = GAME_STATE_ID::SETTINGS;
        } else if (game_state == GAME_STATE_ID::LOAD) {
            *current_game_state = GAME_STATE_ID::LOAD;
        } else if (game_state == GAME_STATE_ID::WORLD) {
            *current_game_state = GAME_STATE_ID::WORLD;
        } else if (game_state == GAME_STATE_ID::LOAD_SAVE) {
            *current_game_state = GAME_STATE_ID::LOAD_SAVE;
        } else if (game_state == GAME_STATE_ID::DELETE_SAVE_DONE) {
            *current_game_state = GAME_STATE_ID::DELETE_SAVE_DONE;
        } else if (game_state == GAME_STATE_ID::TELEPORT_1_DONE) {
            *current_game_state = GAME_STATE_ID::TELEPORT_1_DONE;
        } else if (game_state == GAME_STATE_ID::TELEPORT_2_DONE) {
            *current_game_state = GAME_STATE_ID::TELEPORT_2_DONE;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    ImGui::PopID();
}

void RenderSystem::createBattleTutorialButton(const ImVec2& position) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // remove window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::SetNextWindowPos(position); // set button position
    ImGui::SetNextWindowSize(ImVec2(115.f * scale_x, 50.f * scale_y)); // set button size

    //std::string s = "Battle Tutorial Button Window " + std::to_string(battle_tutorial_index);
    ImGui::Begin("Battle Tutorial Button Window", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    bool isHovered = ImGui::IsWindowHovered();
    ImU32 backgroundColor = isHovered ? IM_COL32(155, 103, 60, 100) : IM_COL32(50, 50, 50, 0);
    ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + 115.f * scale_x, position.y + 50.f * scale_y), backgroundColor, 4.0f);
    //s = "Battle Tutorial Button " + std::to_string(battle_tutorial_index);
    if (ImGui::InvisibleButton("Battle Tutorial Button", ImVec2(115.f * scale_x, 50.f * scale_y))) {
        if (battle_tutorial_index < battle_tutorials.size() - 1) {
            battle_tutorial_index++;
        }
        else {
            registry.players.get(player).battle_tutorial_complete = true;
            *current_game_state = GAME_STATE_ID::BATTLE;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

// SCALE IS SET IN THIS FUNCTION, NO NEED TO ADD SCALE TO OFFSETS IF USING THIS FUNCTION
// Uses the Dear ImGui API to get the draw cursor and set the next offset
void RenderSystem::setDrawCursorScreenPos(ImVec2 offset)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(p.x + offset.x * scale_x, p.y + offset.y * scale_y));
}

void RenderSystem::drawCharacterDetail(int selectedCharacter) {
    // Character Detail Window
    auto& party_container = registry.partyMembers;
    PartyMember& party_member = party_container.components[selectedCharacter];

    ImGui::SetNextWindowPos(ImVec2(990.f * scale_x, 50.f * scale_y));
    ImGui::SetNextWindowSize(ImVec2(410.0f * scale_x, 600.0f * scale_y));
    ImGui::Begin("Character Detail", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
    setDrawCursorScreenPos(ImVec2(75.f, 10.f));
    ImGui::Image((void*)(intptr_t)texture_gl_handles[(int)party_member.menu_texture_id], ImVec2(250.f * scale_x, 250.f * scale_y));

    ImGui::SetNextWindowPos(ImVec2(1000 * scale_x, 320 * scale_y));

    // Draw detail image background for party member 1 if party member 1 was added
    if (partyMemberOneAdded && selectedCharacter == 1) {
        ImGui::SetNextWindowSize(ImVec2(390.f * scale_x, 320.f * scale_y));
        ImGui::Begin("Character Detail Background", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
        ImGui::Image((void*)(intptr_t)27, ImVec2(380 * scale_x, 320 * scale_y));
        ImGui::End();
    } else {
        // Draw the default character detail window
        ImGui::BeginChild("Character Description", ImVec2(390.0f * scale_x, 320.0f * scale_y), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::TextWrapped("Name: %s", party_member.name.c_str());
        ImGui::TextWrapped("%s", party_member.description.c_str());

        ImGui::TextWrapped("\nAttack: %d", party_member.stats.attack);
        ImGui::TextWrapped("Speed: %d", party_member.stats.speed);

        ImGui::TextWrapped("\nSkills:");
        ImGui::PushFont(font_small);
        ImGui::TextWrapped("%s", party_member.skills[0].skill_description.c_str());
        ImGui::TextWrapped("\n%s", party_member.skills[1].skill_description.c_str());
        ImGui::TextWrapped("\n%s", party_member.skills[2].skill_description.c_str());
        if (selectedCharacter == 0) {
            ImGui::TextWrapped("\n%s", party_member.skills[3].skill_description.c_str());
        }
        ImGui::PopFont();

        ImGui::EndChild();
    }
    ImGui::End();
}

// might not be necessary anymore, keeping it here jic
void RenderSystem::drawPartyListPopup(int selectedCharacter) {
    ImGui::Text("WOAH!!");
    if (ImGui::BeginTable("Party List", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        for (int row = 0; row < 4; row++)
        {
            if (row == 0) {
                // first row is a visual of the player character
                ImGui::TableNextRow(0, 90.f * scale_y);
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("\n \n YOU!!");
            } // Party Member Rows
            else {

                // this is for empty slot
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.f);
                ImGui::TableNextRow(0, 90.f * scale_y);
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("\n EMPTY", row);

                //Put button to the left of party window (hard coded -- will need to adjust based on whats drawn)
                setDrawCursorScreenPos(ImVec2(380.f, 0.f));

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                if (ImGui::Button("ADD", ImVec2(70.f * scale_x, 30.f * scale_y))) {
                    // add the selected character to this row
                    // if this slot is already occupied, swap with selected character, something like
                    // **pseudo code:**
                    // temp_slot = this_row.characterID
                    // this_row.characterID = selected_character
                    // selected_character = temp slot
                }
                ImGui::PopStyleVar(2);
            }
        }
        ImGui::EndTable();

        setDrawCursorScreenPos(ImVec2(0, 40.f));

        ImGui::BeginChild("Party Select", ImVec2(485.f * scale_x, 90.f * scale_y), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text("\n \n Party Member %d!!", selectedCharacter);
        ImGui::EndChild();
    }

    setDrawCursorScreenPos(ImVec2(195, 10.f));
    if (ImGui::Button("Back", ImVec2(100.f * scale_x, 30.f * scale_y))) {
        show_party_list = false;
        ImGui::CloseCurrentPopup();
    }
}

// draws portraits and text related to dialogue cutscenes
// currently set up to have MC on the left and an ally on the right. So only two people can talk (and one of them is the MC) at the moment.
// don't forget to change the current_game_state to CUTSCENE whenever you initiate a dialogue
void RenderSystem::drawDialogueCutscene() {
    const char* name = "name";
    const char* text = "text";
    const char* frame_mc = "frame_mc";
    const char* frame_ally = "frame_ally";
    // should only be one dialogue at a time
    Dialogue& dialogue_request = registry.dialogues.components[0];
    Entity& player = registry.dialogues.entities[0];
    json dialogue_info = dialogue[dialogue_request.cutscene_id.c_str()];
    json conversation = dialogue_info["conversation"];
    json popup_info = dialogue_info["popup"];
    int num_lines = dialogue_info.value("num_lines", 0);

    TEXTURE_ASSET_ID mc_texture = TEXTURE_ASSET_ID::MC_DIALOGUE;
    TEXTURE_ASSET_ID ally_texture = (TEXTURE_ASSET_ID) dialogue_info.value("texture_ally", 0);
    char text_id[10];
    sprintf(text_id, "line_%d", dialogue_request.current_line);
    json text_info = conversation[text_id];

    TEXTURE_ASSET_ID bg_texture = (TEXTURE_ASSET_ID)text_info.value("background", 0);

    vec2 portrait_scale = { 600, 600 };

    float mc_active;
    float ally_active;
    if (text_info.value(name, "Error") == "Jonah" || text_info.value(name, "Error") == "???") {
        mc_active = 0.f;
        ally_active = 0.4f;
    }
    else {
        mc_active = 0.4f;
        ally_active = 0.f;
    }

    if (bg_texture != (TEXTURE_ASSET_ID)0) {
        drawSpriteAnime(0, 1, 1, bg_texture, { window_width_px / 2, window_height_px / 2 }, { window_width_px, window_height_px });
    }
    else {
        drawSpriteAnime(text_info.value(frame_mc, 0), 1, 5, mc_texture, { 250, window_height_px / 2 - 75 }, portrait_scale, mc_active);
        if (ally_texture != (TEXTURE_ASSET_ID)0) {
            drawSpriteAnime(text_info.value(frame_ally, 0), 1, 2, ally_texture, { window_width_px - 300, window_height_px / 2 - 75 }, portrait_scale, ally_active);
        }
    }

    // ImGui stuff
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.5f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 5.0f);
    ImGui::SetNextWindowPos(ImVec2(0.f, (window_height_px - 300.f) * scale_y));
    ImGui::SetNextWindowSize(ImVec2(window_width_px * scale_x, 300.0f * scale_y));
    ImGui::Begin(text_info.value(name, "ERROR").c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);
    setDrawCursorScreenPos(ImVec2(10.f, 10.f));
    ImGui::TextWrapped(text_info.value(text, "error").c_str());

    ImGui::SetCursorScreenPos(ImVec2((window_width_px - 160)*scale_x, (window_height_px - 80)*scale_y));
    if (dialogue_request.current_line == num_lines - 1) {
        if (ImGui::Button("End", ImVec2(100.f * scale_x, 40.f * scale_y))) {
            if (registry.dialogues.get(player).cutscene_id.compare(std::string("ally_1_recruit")) == 0) {
                registry.players.get(player).ally1_recruited = true;
            }
            else if (registry.dialogues.get(player).cutscene_id.compare(std::string("ally_2_recruit")) == 0) {
                registry.players.get(player).ally2_recruited = true;
            }
            registry.dialogues.remove(player);
            if (!popup_info.is_null()) {
                *current_game_state = GAME_STATE_ID::CUTSCENE_TRANSITION;
                popup = popup_info;
            }
            else {
                // end cutscene and go back to next state
                *current_game_state = dialogue_request.next_game_state;
            }
        }
    }
    else {
        if (ImGui::Button("Next", ImVec2(100.f * scale_x, 40.f * scale_y))) {
            dialogue_request.current_line++;
        }
    }

    ImGui::PopStyleVar(2);
    ImGui::End();

    // TBH idk why, but without this, it doubles the darken effect if ally is darkened (the last texture drawn that uses the effect)
    const GLuint used_effect_enum = (GLuint)EFFECT_ASSET_ID::TEXTURED;
    const GLuint program = (GLuint)effects[used_effect_enum];
    GLuint darken_factor = glGetUniformLocation(program, "darken_factor");
    glUniform1f(darken_factor, 0.f);
    gl_has_errors();
}

void RenderSystem::drawStartMenu() {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::Begin("Start Menu", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetWindowSize(ImVec2(window_width_px * scale_x, window_height_px * scale_y));
    // This is the wooden background image
    ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_gl_handles[(int)TEXTURE_ASSET_ID::TITLE], ImVec2(0.f, 0.f), ImVec2(window_width_px * scale_x, window_height_px * scale_y), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f));

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 6.0f);
    ImGui::SetNextWindowPos(ImVec2((window_width_px/2 - 150.f) * scale_x, (window_height_px/2 + 160.f) * scale_y));
    ImGui::BeginChild("Options", ImVec2(300.0f * scale_x, 220.0f * scale_y), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    std::ifstream is("../savedata.json", std::ios::ate);

    setDrawCursorScreenPos(ImVec2(50.f, 35.f));
    if (ImGui::Button("New Game", ImVec2(200.f * scale_x, 40.f * scale_y))) {
        Dialogue& dialogue = registry.dialogues.emplace(player);
        dialogue.cutscene_id = "introduction";
        *current_game_state = GAME_STATE_ID::CUTSCENE;
        dialogue.next_game_state = GAME_STATE_ID::TUTORIAL;
    }

    setDrawCursorScreenPos(ImVec2(50.f, 10.f));
    ImGui::BeginDisabled(is.tellg() == 0);
    if (ImGui::Button("Continue", ImVec2(200.f * scale_x, 40.f * scale_y))) {
        if (is.tellg() == 0) { // check if save data exists
            *current_game_state = GAME_STATE_ID::LOAD_FAIL;
        }
        else {
            should_restart_game = true;
        }
    }
    setDrawCursorScreenPos(ImVec2(50.f, 10.f));
    ImGui::EndDisabled();
    if (ImGui::Button("Quit", ImVec2(200.f * scale_x, 40.f * scale_y))) {
        exit(EXIT_SUCCESS);
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();
}
