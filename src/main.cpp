
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "physics_system.hpp"
#include "render_system.hpp"
#include "sound_system.hpp"
#include "world_system.hpp"
#include "battle_system.hpp"
#include "animation_system.hpp"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// Global systems
	WorldSystem world_system;
	RenderSystem render_system;
	SoundSystem sound_system;
	PhysicsSystem physics_system;
	BattleSystem battle_system;
	AnimationSystem animation_system;
	GAME_STATE_ID current_game_state = GAME_STATE_ID::START_MENU;
	
	// Initializing window
	GLFWwindow* window = world_system.create_window();
	if (!window) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}



	// initialize the main systems
	bool sound = sound_system.init(&current_game_state);
	if (!sound) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}
	render_system.init(window, &current_game_state, &battle_system, &sound_system);
	world_system.init(&render_system, &sound_system, &current_game_state);

    physics_system.lakeMesh = render_system.lakeMesh; // Add lakeMesh to physics_system
    physics_system.lakeEdges = render_system.lakeEdges; // Add lakeEdges to physics_system

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	render_system.initFonts();
	
	// Set ImGui Style
	// note: I'm dividing colours by 255.f because I'm getting RGB values from my painting software and ImVec4 colours are from 0...1
	ImGuiStyle* style = &ImGui::GetStyle();
	style->WindowRounding = 5.0f;
	style->TabBorderSize = 4.0f;
	style->PopupBorderSize = 2.0f;
	style->FrameRounding = 5.0f;
	style->FrameBorderSize = 4.0f;
	style->ChildRounding = 4.0f;
	style->ChildBorderSize = 2.0f;
	style->CellPadding = ImVec2(0, 0);
	style->Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(186.f / 255.f, 139.f / 255.f, 97.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(201.f / 255.f, 163.f / 255.f, 113.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(235.f / 255.f, 202.f / 255.f, 125.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(216.f / 255.f, 152.f / 255.f, 85.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(167.f / 255.f, 109.f / 255.f, 37.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(167.f / 255.f, 109.f / 255.f, 37.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ChildBg] = ImVec4(228.f / 255.f, 185.f / 255.f, 128.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(198.f / 255.f, 144.f / 255.f, 87.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(167.f / 255.f, 109.f / 255.f, 37.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(83.f / 255.f, 57.f / 255.f, 24.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(113.f / 255.f, 77.f / 255.f, 33.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(235.f / 255.f, 202.f / 255.f, 125.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_Border] = ImVec4(91.f / 255.f, 67.f / 255.f, 34.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_Tab] = ImVec4(186.f / 255.f, 139.f / 255.f, 97.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_TabHovered] = ImVec4(235.f / 255.f, 202.f / 255.f, 125.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_TabActive] = ImVec4(228.f / 255.f, 185.f / 255.f, 128.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_TableBorderStrong] = ImVec4(91.f / 255.f, 67.f / 255.f, 34.f / 255.f, 1.00f);   // Table outer and header borders (prefer using Alpha=1.0 here)
	style->Colors[ImGuiCol_TableBorderLight] = ImVec4(91.f / 255.f, 67.f / 255.f, 34.f / 255.f, 1.00f);     // Table inner borders (prefer using Alpha=1.0 here)
	style->Colors[ImGuiCol_TableRowBg] = ImVec4(228.f / 255.f, 185.f / 255.f, 128.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_TableRowBgAlt] = ImVec4(228.f / 255.f, 185.f / 255.f, 128.f / 255.f, 1.00f);
	style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.f, 0.f, 0.f, 0.5f);
	//style->Colors[ImGuiCol_TableRowBg] = ImVec4(0,0,0, 1.00f);

	battle_system.start(&current_game_state, &sound_system);
	// variable timestep loop
	auto t = Clock::now();
	while (!world_system.is_over()) {
        if (render_system.should_restart_game) {
            render_system.should_restart_game = false;
            current_game_state = GAME_STATE_ID::WORLD;
            world_system.should_load_save = true;
            world_system.restart_game();
        }
		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;
		sound_system.update();
		if (current_game_state == GAME_STATE_ID::WORLD) {
			world_system.step(elapsed_ms);
			physics_system.step(elapsed_ms);
			// place it in world state only, i think the vertex buffers currently clash with the battle drawSpriteAnime()
			animation_system.step(elapsed_ms);
		}

		if (current_game_state == GAME_STATE_ID::BATTLE) {
			battle_system.update(elapsed_ms);
		}
		//printf("%d: \n", current_game_state);
		world_system.handle_collisions();

		render_system.draw();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	return EXIT_SUCCESS;
}
