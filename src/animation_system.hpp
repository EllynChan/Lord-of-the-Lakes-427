#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>

#include "render_system.hpp"

class AnimationSystem
{
public:
	void step(float elapsed_ms);
};