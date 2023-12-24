#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
    std::vector<TexturedVertex> lakeMesh; // For defining lake boundary
    std::vector<vec2> lakeEdges; // For defining lake edges
	void step(float elapsed_ms);

	PhysicsSystem()
	{
	}
};