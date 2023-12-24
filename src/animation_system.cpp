// internal
#include "animation_system.hpp"

void AnimationSystem::step(float elapsed_ms)
{
	auto& sprite_container = registry.sprites;
	// find sprite information and set current frame

	//fprintf(stderr, "elapsed ms: %f, step seconds: %f \n", elapsed_ms, step_seconds);
	double time = glfwGetTime();
	for (uint i = 0; i < sprite_container.size(); i++) {

		Sprite& sprite = sprite_container.components[i];

		double frameDuration = sprite.frame_duration; // Time in seconds for each frame
		if (sprite.last_time == 0.0f) {
			sprite.last_time = (float)time;
		}
		int currentFrameTime = static_cast<int>((time - sprite.last_time) / (frameDuration)) % sprite.num_frames + sprite.start_frame;

		if ((time - sprite.last_time) > 1.2f) {
			sprite.last_time = 0.0f;
		}

		//fprintf(stderr, "elapsed ms: %f, current frame time: %d \n", elapsed_ms, currentFrameTime);
		if (currentFrameTime < sprite.end_frame) {
			sprite.current_frame = currentFrameTime;
			//fprintf(stderr, "sprite current frame: %d \n", sprite.current_frame);
		}
		else if (!sprite.loop) {
			//fprintf(stderr, "end loop \n");
			sprite.current_frame = sprite.start_frame;
			sprite.end_frame = sprite.start_frame;
			sprite.num_frames = 1;
			sprite.last_time = 0.0f;
		}
	}
	(void)elapsed_ms; // placeholder to silence unused warning until implemented
}
