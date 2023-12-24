// internal
#include "render_system.hpp"

#include <array>
#include <fstream>

#include "../ext/stb_image/stb_image.h"

// This creates circular header inclusion, that is quite bad.
#include "tiny_ecs_registry.hpp"
#include "battle_system.hpp"

// stlib
#include <iostream>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

// Create BG mesh
std::vector<TexturedVertex> createBGMesh()
{
    std::vector<TexturedVertex> vertices;

    // Define the vertices for a rectangle covering the entire game window
    vertices.push_back({{-1234.0f, -1022.0f, 0.0f}, {0.0f, 0.0f}}); // Top-left corner
    vertices.push_back({{1234.0f, -1022.0f, 0.0f}, {1.0f, 0.0f}});  // Top-right corner
    vertices.push_back({{1234.0f, 1022.0f, 0.0f}, {1.0f, 1.0f}}); // Bottom-right corner
    vertices.push_back({{-1234.0f, 1022.0f, 0.0f}, {0.0f, 1.0f}}); // Bottom-left corner

    return vertices;
}

// Define BG indices
std::vector<uint16_t> createBGIndices()
{
    // Define the indices for two triangles forming a rectangle.
    // The indices 0, 1, 2 form the first triangle (top-left to top-right to bottom-right),
    // and the indices 0, 2, 3 form the second triangle (top-left to bottom-right to bottom-left).
    std::vector<uint16_t> indices = {0, 1, 2, 0, 2, 3};

    return indices;
}

// Create tutorial screen BG
// TODO: Not using this currently; will probably remove later
std::vector<TexturedVertex> createTutorialScreenMesh()
{
    std::vector<TexturedVertex> vertices;

    // Define the vertices for a rectangle tutorial screen
    vertices.push_back({{-25.0f, 35.0f, 0.0f}, {0.0f, 1.0f}}); // Top-left corner
    vertices.push_back({{25.0f, 35.0f, 0.0f}, {1.0f, 1.0f}});  // Top-right corner
    vertices.push_back({{25.0f, -35.0f, 0.0f}, {1.0f, 0.0f}}); // Bottom-right corner
    vertices.push_back({{-25.0f, -35.0f, 0.0f}, {0.0f, 0.0f}}); // Bottom-left corner

    return vertices;
}

// The indices will form triangles from the vertices.
// TODO: Not using this currently; will probably remove later
std::vector<uint16_t> createTutorialScreenIndices()
{
    std::vector<uint16_t> indices = {0, 1, 2, 0, 2, 3};

    return indices;
}

// Create lake (represents playable area)
// Used tutorial from: https://stackoverflow.com/questions/54828017/c-create-random-shaped-blob-objects
std::vector<TexturedVertex> createLakeMesh()
{
    std::vector<TexturedVertex> vertices;

    const int numPoints = 100;

    const int N = 8; // Number of waves

    // Define arrays for amplitudes and phases
    float amps[N];
    float phases[N];

    for (int i = 0; i < N; ++i)
    {
        amps[i] = float(rand() % 300) / 100.f; // Random amplitude between 0 and 1
        phases[i] = float(rand() % 630) / 100.f; // Random phase between 0 and 2π
    }

    for (int i = 0; i < numPoints; ++i)
    {
        float alpha = float(i) * (2 * M_PI / numPoints);
        float radius = 80.0f;

        for (int j = 0; j < N; ++j)
        {
            radius += amps[j] * cos((j + 1) * alpha + phases[j]);
        }

        float x = cos(alpha) * radius * 1.3f;
        float y = sin(alpha) * radius;

        vertices.push_back({{x, y, 0.0f}, {x + 0.5f, y + 0.5f}});
    }

    return vertices;
}

// The indices will form triangles from the vertices.
// For a fan-shaped arrangement, connect all vertices to the center point (index 0) to create triangles.
std::vector<uint16_t> createLakeIndices()
{
    std::vector<uint16_t> indices;
    const uint16_t numPoints = 100;  // Same as in createLakeMesh

    for (uint16_t i = 1; i < numPoints; ++i)
    {
        indices.push_back(0);  // Center vertex
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    // Connect the last vertex to the first vertex to close the loop
    indices.push_back(0);
    indices.push_back(numPoints - 1);
    indices.push_back(1);

    return indices;
}

// Create shore mesh
// TODO: Not using this currently; will probably remove later
std::vector<TexturedVertex> createShoreMesh()
{
    std::vector<TexturedVertex> vertices;

    // Define the vertices for a rectangle covering the entire game window
    // Note: Can replace x and y values with (window_width_px / 2) and (window_height_px/ 2), respectively
    vertices.push_back({{-750.0f, -450.0f, 0.0f}, {0.0f, 1.0f}}); // Top-left corner
    vertices.push_back({{750.0f, -450.0f, 0.0f}, {1.0f, 1.0f}});  // Top-right corner
    vertices.push_back({{750.f, 450.f, 0.0f}, {1.0f, 0.0f}}); // Bottom-right corner
    vertices.push_back({{-750.f, 450.0f, 0.0f}, {0.0f, 0.0f}}); // Bottom-left corner

    return vertices;
}

// Define shore indices
// TODO: Not using this currently; will probably remove later
std::vector<uint16_t> createShoreIndices()
{
    // Define the indices for two triangles forming a rectangle.
    // The indices 0, 1, 2 form the first triangle (top-left to top-right to bottom-right),
    // and the indices 0, 2, 3 form the second triangle (top-left to bottom-right to bottom-left).
    std::vector<uint16_t> indices = {0, 1, 2, 0, 2, 3};

    return indices;
}

// World initialization
bool RenderSystem::init(GLFWwindow* window_arg, GAME_STATE_ID* game_state_arg, BattleSystem* battle_system, SoundSystem* sound_system_arg)
{
	this->window = window_arg;
	this->current_game_state = game_state_arg;
	this->sound_system = sound_system_arg;
	this->battle_system = battle_system;
	
	for (int i = 0; i < 100; ++i) {
		particles.push_back(Particle());
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // vsync

	int device_width, device_height;
	glfwGetWindowSize(window_arg, &device_width, &device_height);
	scale_x = ((float)device_width / (float)window_width_px);
	scale_y = ((float)device_height / (float)window_height_px);

	// Load OpenGL function pointers
	const int is_fine = gl3w_init();
	assert(is_fine == 0);

	// Create a frame buffer
	frame_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// For some high DPI displays (ex. Retina Display on Macbooks)
	// https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value
	int frame_buffer_width_px, frame_buffer_height_px;
	glfwGetFramebufferSize(window, &frame_buffer_width_px, &frame_buffer_height_px);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	if (frame_buffer_width_px != window_width_px)
	{
		printf("WARNING: retina display! https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value\n");
		printf("glfwGetFramebufferSize = %d,%d\n", frame_buffer_width_px, frame_buffer_height_px);
		printf("window width_height = %d,%d\n", window_width_px, window_height_px);
	}

	// Hint: Ask your TA for how to setup pretty OpenGL error callbacks.
	// This can not be done in mac os, so do not enable
	// it unless you are on Linux or Windows. You will need to change the window creation
	// code to use OpenGL 4.3 (not suported on mac) and add additional .h and .cpp
	// glDebugMessageCallback((GLDEBUGPROC)errorCallback, nullptr);

	// We are not really using VAO's but without at least one bound we will crash in
	// some systems.
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	gl_has_errors();

	initScreenTexture();
    initializeGlTextures();
	initializeGlEffects();
	initializeGlGeometryBuffers();

	return true;
}

void RenderSystem::initializeGlTexturesFromPaths(
	std::array<std::string, texture_count> texture_paths,
	std::array<GLuint,texture_count>& texture_gl_handles,
	std::array<ivec2, texture_count>& texture_dimensions)
{
    glGenTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());

    for(uint i = 0; i < texture_paths.size(); i++)
    {
		const std::string& path = texture_paths[i];
		if (path.empty())
			break;

		ivec2& dimensions = texture_dimensions[i];

		stbi_uc* data;
		data = stbi_load(path.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

		if (data == NULL)
		{
			const std::string message = "Could not load the file " + path + ".";
			fprintf(stderr, "%s", message.c_str());
			assert(false);
		}
		glBindTexture(GL_TEXTURE_2D, texture_gl_handles[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl_has_errors();
		stbi_image_free(data);
    }
	gl_has_errors();
};

void RenderSystem::initializeGlTextures()
{
	initializeGlTexturesFromPaths(texture_paths, texture_gl_handles, texture_dimensions);
	initializeGlTexturesFromPaths(fish_texture_paths, fish_texture_gl_handles, fish_texture_dimensions);
}

void RenderSystem::initializeGlEffects()
{
	for(uint i = 0; i < effect_paths.size(); i++)
	{
		const std::string vertex_shader_name = effect_paths[i] + ".vs.glsl";
		const std::string fragment_shader_name = effect_paths[i] + ".fs.glsl";

		bool is_valid = loadEffectFromFile(vertex_shader_name, fragment_shader_name, effects[i]);
		assert(is_valid && (GLuint)effects[i] != 0);
	}
}

// One could merge the following two functions as a template function...
template <class T>
void RenderSystem::bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(uint)gid]);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(uint)gid]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
	gl_has_errors();
}

void RenderSystem::initializeGlMeshes()
{
	for (uint i = 0; i < mesh_paths.size(); i++)
	{
		// Initialize meshes
		GEOMETRY_BUFFER_ID geom_index = mesh_paths[i].first;
		std::string name = mesh_paths[i].second;
		Mesh::loadFromOBJFile(name,
			meshes[(int)geom_index].vertices,
			meshes[(int)geom_index].vertex_indices,
			meshes[(int)geom_index].original_size);

		bindVBOandIBO(geom_index,
			meshes[(int)geom_index].vertices,
			meshes[(int)geom_index].vertex_indices);
	}
}

void RenderSystem::initializeGlGeometryBuffers()
{
	// Vertex Buffer creation.
	glGenBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	// Index Buffer creation.
	glGenBuffers((GLsizei)index_buffers.size(), index_buffers.data());

	// Index and Vertex buffer data initialization.
	initializeGlMeshes();

	//////////////////////////
	// Initialize sprite
	// The position corresponds to the center of the texture.
	std::vector<TexturedVertex> textured_vertices(4);
	textured_vertices[0].position = { -1.f/2, +1.f/2, 0.f };
	textured_vertices[1].position = { +1.f/2, +1.f/2, 0.f };
	textured_vertices[2].position = { +1.f/2, -1.f/2, 0.f };
	textured_vertices[3].position = { -1.f/2, -1.f/2, 0.f };
	textured_vertices[0].texcoord = { 0.f, 1.f };
	textured_vertices[1].texcoord = { 1.f, 1.f };
	textured_vertices[2].texcoord = { 1.f, 0.f };
	textured_vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> textured_indices = { 0, 3, 1, 1, 3, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SPRITE, textured_vertices, textured_indices);

	////////////////////////
	// Initialize pebble
	std::vector<ColoredVertex> pebble_vertices;
	std::vector<uint16_t> pebble_indices;
	constexpr float z = -0.1f;
	constexpr int NUM_TRIANGLES = 2;

	/*for (int i = 0; i < NUM_TRIANGLES; i++) {
		const float t = float(i) * M_PI * 2.f / float(NUM_TRIANGLES - 1);
		pebble_vertices.push_back({});
		pebble_vertices.back().position = { 0.5 * cos(t), 0.5 * sin(t), z };
		pebble_vertices.back().color = { 1, 1, 1 };
	}
	pebble_vertices.push_back({});
	pebble_vertices.back().position = { 0, 0, 0 };
	pebble_vertices.back().color = { 1, 1, 1 };
	for (int i = 0; i < NUM_TRIANGLES; i++) {
		pebble_indices.push_back((uint16_t)i);
		pebble_indices.push_back((uint16_t)((i + 1) % NUM_TRIANGLES));
		pebble_indices.push_back((uint16_t)NUM_TRIANGLES);
	}*/

	//pebble_vertices.push_back({ {-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f} });  // Bottom-left vertex
	//pebble_vertices.push_back({ {0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f} });   // Bottom-right vertex
	//pebble_vertices.push_back({ {0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f} });    // Top-right vertex
	//pebble_vertices.push_back({ {-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f} });   // Top-left vertex

	//// Define indices for the rectangle (two triangles)
	//pebble_indices.push_back(0); // Bottom-left
	//pebble_indices.push_back(1); // Bottom-right
	//pebble_indices.push_back(2); // Top-right

	//pebble_indices.push_back(2); // Bottom-left
	//pebble_indices.push_back(3); // Top-right
	//pebble_indices.push_back(0); // Top-left

	pebble_vertices.push_back({ {0, 0, 0}, {1.0f, 1.0f, 1.0f} });

	// Define outer points of the star
	for (float angle = 0.0f; angle < 360.0f; angle += 144.0f) {
		float outerX = 0.5 * cos(glm::radians(angle));
		float outerY = 0.5 * sin(glm::radians(angle));
		pebble_vertices.push_back({ {outerX, outerY, 0}, {1.0f, 1.0f, 1.0f} });
	}
	for (uint16_t i = 1; i < 5; ++i) {
		pebble_indices.push_back(i);
	}
	pebble_indices.push_back(1);
	int geom_index = (int)GEOMETRY_BUFFER_ID::PEBBLE;
	meshes[geom_index].vertices = pebble_vertices;
	meshes[geom_index].vertex_indices = pebble_indices;
	bindVBOandIBO(GEOMETRY_BUFFER_ID::PEBBLE, meshes[geom_index].vertices, meshes[geom_index].vertex_indices);
	//////////////////////////////////
	// Initialize debug line
	std::vector<ColoredVertex> line_vertices;
	std::vector<uint16_t> line_indices;

	constexpr float depth = 0.5f;
	constexpr vec3 red = { 0.8,0.1,0.1 };

	// Corner points
	line_vertices = {
		{{-0.5,-0.5, depth}, red},
		{{-0.5, 0.5, depth}, red},
		{{ 0.5, 0.5, depth}, red},
		{{ 0.5,-0.5, depth}, red},
	};

	// Two triangles
	line_indices = {0, 1, 3, 1, 2, 3};

	geom_index = (int)GEOMETRY_BUFFER_ID::DEBUG_LINE;
	meshes[geom_index].vertices = line_vertices;
	meshes[geom_index].vertex_indices = line_indices;
	bindVBOandIBO(GEOMETRY_BUFFER_ID::DEBUG_LINE, line_vertices, line_indices);

	///////////////////////////////////////////////////////
	// Initialize screen triangle (yes, triangle, not quad; its more efficient).
	std::vector<vec3> screen_vertices(3);
	screen_vertices[0] = { -1, -6, 0.f };
	screen_vertices[1] = { 6, -1, 0.f };
	screen_vertices[2] = { -1, 6, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> screen_indices = { 0, 1, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE, screen_vertices, screen_indices);

    ////////////////////////////////////////////////////////
    // Initialize lake
    lakeMesh = createLakeMesh();
    std::vector<uint16_t> lake_indices = createLakeIndices();
    bindVBOandIBO(GEOMETRY_BUFFER_ID::LAKE, lakeMesh, lake_indices);

    ///////////////////////////////////////////////////////
    // Initialize BG
    std::vector<TexturedVertex> bg_vertices = createBGMesh();
    std::vector<uint16_t> bg_indices = createBGIndices();
    bindVBOandIBO(GEOMETRY_BUFFER_ID::BACKGROUND, bg_vertices, bg_indices);

	//////////////////////////
	// Initialize sprite sheet
	// The position corresponds to the center of the texture.
	std::vector<TexturedVertex> sprite_sheet_vertices(4);
	sprite_sheet_vertices[0].position = { -1.f / 2, +1.f / 2, 0.f };
	sprite_sheet_vertices[1].position = { +1.f / 2, +1.f / 2, 0.f };
	sprite_sheet_vertices[2].position = { +1.f / 2, -1.f / 2, 0.f };
	sprite_sheet_vertices[3].position = { -1.f / 2, -1.f / 2, 0.f };
	sprite_sheet_vertices[0].texcoord = { 0.f, 1.f };
	sprite_sheet_vertices[1].texcoord = { 1.f, 1.f };
	sprite_sheet_vertices[2].texcoord = { 1.f, 0.f };
	sprite_sheet_vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SPRITE_SHEET, sprite_sheet_vertices, textured_indices);

}

RenderSystem::~RenderSystem()
{
	// Don't need to free gl resources since they last for as long as the program,
	// but it's polite to clean after yourself.
	glDeleteBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	glDeleteBuffers((GLsizei)index_buffers.size(), index_buffers.data());
	glDeleteTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());
	glDeleteTextures((GLsizei)fish_texture_gl_handles.size(), fish_texture_gl_handles.data());
	glDeleteTextures(1, &off_screen_render_buffer_color);
	glDeleteRenderbuffers(1, &off_screen_render_buffer_depth);
	gl_has_errors();

	for(uint i = 0; i < effect_count; i++) {
		glDeleteProgram(effects[i]);
	}
	// delete allocated resources
	glDeleteFramebuffers(1, &frame_buffer);
	gl_has_errors();

	// remove all entities created by the render system
	while (registry.renderRequests.entities.size() > 0)
	    registry.remove_all_components_of(registry.renderRequests.entities.back());
}

// Initialize the screen texture from a standard sprite
bool RenderSystem::initScreenTexture()
{
	registry.screenStates.emplace(screen_state_entity);

	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &framebuffer_width, &framebuffer_height);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	glGenTextures(1, &off_screen_render_buffer_color);
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();

	glGenRenderbuffers(1, &off_screen_render_buffer_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, off_screen_render_buffer_depth);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, off_screen_render_buffer_color, 0);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebuffer_width, framebuffer_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, off_screen_render_buffer_depth);
	gl_has_errors();

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	return true;
}

void RenderSystem::initFonts() {
	// initialize fonts
	ImGuiIO& io = ImGui::GetIO();

	// first font loaded is the default font that will be used everywhere
	font_default = io.Fonts->AddFontFromFileTTF(font_path("BadComic-Regular.ttf").c_str(), 32); 
	font_small = io.Fonts->AddFontFromFileTTF(font_path("BadComic-Regular.ttf").c_str(), 28);
	font_large = io.Fonts->AddFontFromFileTTF(font_path("BadComic-Regular.ttf").c_str(), 36);
	font_effect = io.Fonts->AddFontFromFileTTF(font_path("Carre.ttf").c_str(), 40.f);

	//json test
	std::ifstream f(data_path() + "/dialogue.json");
	dialogue = json::parse(f);
}

bool gl_compile_shader(GLuint shader)
{
	glCompileShader(shader);
	gl_has_errors();
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		GLint log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> log(log_len);
		glGetShaderInfoLog(shader, log_len, &log_len, log.data());
		glDeleteShader(shader);

		gl_has_errors();

		fprintf(stderr, "GLSL: %s", log.data());
		return false;
	}

	return true;
}

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program)
{
	// Opening files
	std::ifstream vs_is(vs_path);
	std::ifstream fs_is(fs_path);
	if (!vs_is.good() || !fs_is.good())
	{
		fprintf(stderr, "Failed to load shader files %s, %s", vs_path.c_str(), fs_path.c_str());
		assert(false);
		return false;
	}

	// Reading sources
	std::stringstream vs_ss, fs_ss;
	vs_ss << vs_is.rdbuf();
	fs_ss << fs_is.rdbuf();
	std::string vs_str = vs_ss.str();
	std::string fs_str = fs_ss.str();
	const char* vs_src = vs_str.c_str();
	const char* fs_src = fs_str.c_str();
	GLsizei vs_len = (GLsizei)vs_str.size();
	GLsizei fs_len = (GLsizei)fs_str.size();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vs_src, &vs_len);
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fs_src, &fs_len);
	gl_has_errors();

	// Compiling
	if (!gl_compile_shader(vertex))
	{
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}
	if (!gl_compile_shader(fragment))
	{
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}

	// Linking
	out_program = glCreateProgram();
	glAttachShader(out_program, vertex);
	glAttachShader(out_program, fragment);
	glLinkProgram(out_program);
	gl_has_errors();

	{
		GLint is_linked = GL_FALSE;
		glGetProgramiv(out_program, GL_LINK_STATUS, &is_linked);
		if (is_linked == GL_FALSE)
		{
			GLint log_len;
			glGetProgramiv(out_program, GL_INFO_LOG_LENGTH, &log_len);
			std::vector<char> log(log_len);
			glGetProgramInfoLog(out_program, log_len, &log_len, log.data());
			gl_has_errors();

			fprintf(stderr, "Link error: %s", log.data());
			assert(false);
			return false;
		}
	}

	// No need to carry this around. Keeping these objects is only useful if we recycle
	// the same shaders over and over, which we don't, so no need and this is simpler.
	glDetachShader(out_program, vertex);
	glDetachShader(out_program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	gl_has_errors();

	return true;
}
