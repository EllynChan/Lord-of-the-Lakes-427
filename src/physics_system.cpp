// internal
#include "physics_system.hpp"
#include "world_system.hpp"
#include <iostream>

Transform viewMatrix; // For camera

// Checks if two vectors are intersecting (for player-lake boundary detection).
// Used tutorial from: https://stackoverflow.com/questions/217578/how-can-i-determine-whether-a-2d-point-is-within-a-polygon
int areIntersecting(float rayX1, float rayY1, float rayX2, float rayY2,
                    float edgeX1, float edgeY1, float edgeX2, float edgeY2) {
    // Convert ray to a line of infinite length.
    float a1 = rayY2 - rayY1;
    float b1 = rayX1 - rayX2;
    float c1 = (rayX2 * rayY1) - (rayX1 * rayY2);

    // Insert (X1, Y1) and (X2, Y2) of edge into the above linear equation.
    float d1 = (a1 * edgeX1) + (b1 * edgeY1) + c1;
    float d2 = (a1 * edgeX2) + (b1 * edgeY2) + c1;

    // Check if d1 and d2 are on the same side; if so, then no intersection is possible.
    if (d1 > 0 && d2 > 0) return 0;
    if (d1 < 0 && d2 < 0) return 0;

    // Repeat the same test the other way around.
    // Convert edge to a line of infinite length.
    float a2 = edgeY2 - edgeY1;
    float b2 = edgeX1 - edgeX2;
    float c2 = (edgeX2 * edgeY1) - (edgeX1 * edgeY2);

    d1 = (a2 * rayX1) + (b2 * rayY1) + c2;
    d2 = (a2 * rayX2) + (b2 * rayY2) + c2;

    if (d1 > 0 && d2 > 0) return 0;
    if (d1 < 0 && d2 < 0) return 0;

    // Check if the ray and edge are collinear (intersect in any number of points from 0 to infinite).
    if  ((a1 * b2) - (a2 * b1) == 0.0f) return 2;

    // If they are not collinear, they intersect in exactly one point.
    return 1;
}

// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
    vec2 v1 = motion1.velocity;
	vec2 v2 = motion2.velocity;
	vec2 p1 = motion1.position;
	vec2 p2 = motion2.position;

	// if they are moving away from each other, no collision
	if (dot(v1 - v2, p1 - p2) > 0) return false;

	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	float dist = sqrt(dist_squared);
	float radius1 = min(abs(motion1.scale.x), abs(motion1.scale.y)) / 2.f;
	float radius2 = min(abs(motion2.scale.x), abs(motion2.scale.y)) / 2.f;
	if (dist < radius1 + radius2) return true;
	return false;
	// vec2 dp = motion1.position - motion2.position;
	// float dist_squared = dot(dp,dp);
	// const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	// const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	// const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	// const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	// const float r_squared = max(other_r_squared, my_r_squared);
	// if (dist_squared < r_squared)
	// 	return true;
	// return false;
}

void PhysicsSystem::step(float elapsed_ms)
{
	// Move fish based on how much time has passed, this is to (partially) avoid
	// having entities move at different speed based on the machine.
	auto& motion_container = registry.motions;
	for(uint i = 0; i < motion_container.size(); i++)
	{
		// update motion.position based on step_seconds and motion.velocity
		Motion& motion = motion_container.components[i];
		Entity entity = motion_container.entities[i];
		float step_seconds = elapsed_ms / 1000.f;
        //fprintf(stderr, "elapsed ms: %f, step seconds: %f \n", elapsed_ms, step_seconds);

        // For boundary checking between player and the lake, used a tutorial from:
        // https://stackoverflow.com/questions/217578/how-can-i-determine-whether-a-2d-point-is-within-a-polygon
        if (registry.players.has(entity)) {
            // Check if any of the 4 points that define the bounding box of the player collide with the lake boundary.
            int intersections_top_left = 0;
            int intersections_bottom_left = 0;
            int intersections_top_right = 0;
            int intersections_bottom_right = 0;

            // Iterate over all lake edges
            for (int vertex = 0; vertex < lakeMesh.size(); vertex++) {
                float edgeX1 = (lakeMesh[vertex].position.x * 10.f + (float) window_width_px / 2.f);
                float edgeY1 = (lakeMesh[vertex].position.y * 10.f + (float) window_height_px / 2.f);
                float edgeX2;
                float edgeY2;
                if (vertex + 1 == lakeMesh.size()) { // Loop back to the first vertex if at last vertex.
                    edgeX2 = (lakeMesh[0].position.x * 10.f + (float) window_width_px / 2.f);
                    edgeY2 = (lakeMesh[0].position.y * 10.f + (float) window_height_px / 2.f);
                } else {
                    edgeX2 = (lakeMesh[vertex + 1].position.x * 10.f + (float) window_width_px / 2.f);
                    edgeY2 = (lakeMesh[vertex + 1].position.y * 10.f + (float) window_height_px / 2.f);
                }
                // Cast ray that starts from the end of the map and goes to the player's new position
                float rayX1 = 3000.f;
                float rayY1 = 2000.f;
                float rayX2 = motion.position.x + motion.velocity.x * step_seconds;
                float rayY2 = motion.position.y + motion.velocity.y * step_seconds;
                float offset = 60.0f;
                if (areIntersecting(rayX1, rayY1, rayX2 - offset, rayY2, edgeX1, edgeY1, edgeX2, edgeY2) == 1) {
                    intersections_top_left++;
                } if (areIntersecting(rayX1, rayY1, rayX2 - offset, rayY2 + offset, edgeX1, edgeY1, edgeX2, edgeY2) == 1) {
                    intersections_bottom_left++;
                } if (areIntersecting(rayX1, rayY1, rayX2 + offset, rayY2, edgeX1, edgeY1, edgeX2, edgeY2) == 1) {
                    intersections_top_right++;
                } if (areIntersecting(rayX1, rayY1, rayX2 + offset, rayY2 + offset, edgeX1, edgeY1, edgeX2, edgeY2) == 1) {
                    intersections_bottom_right++;
                }
            }
            if (((intersections_top_left & 1) == 1) && ((intersections_bottom_left & 1) == 1) &&
                    ((intersections_top_right & 1) == 1) && ((intersections_bottom_right & 1) == 1)) {
                // Inside polygon
                motion.position.x = motion.position.x + motion.velocity.x * step_seconds;
                motion.position.y = motion.position.y + motion.velocity.y * step_seconds;
                //fprintf(stderr, "physics motion position x: %f \n", motion.position.x);
            } else {
                // Outside polygon
            }
        } else {
            // Move all non-player entities.
            motion.position.x = motion.position.x + motion.velocity.x * step_seconds;
            motion.position.y = motion.position.y + motion.velocity.y * step_seconds;

            // Restrict the movement of fish shadows to within a box within the lake.
            // This is cheaper than doing collision detection against all 99 lake edges.
            if (registry.fishShadows.has(entity)) {
                motion.position.x = std::max(0.f, std::min(motion.position.x, 1300.f));
                motion.position.y = std::max(-100.0f, std::min(motion.position.y, 800.f));
            }
        }

        // Move camera.
        // Camera is centered on player's position if the player isn't near an edge.
        // If near an edge, the camera will stay there until the player moves away from the edge.
        if (registry.players.has(entity) && !(motion.velocity.x == 0 && motion.velocity.y == 0)) {
            float maxX = 1984.f; // map width
            float maxY = 1472.f; // map height

            // Calculate the new camera position
            float newCameraX = motion.position.x - (window_width_px * 0.5f);
            float newCameraY = motion.position.y - (window_height_px * 0.5f);

            // Perform boundary checks
            newCameraX = clamp(newCameraX, window_width_px - maxX, maxX - window_width_px);
            newCameraY = clamp(newCameraY, window_height_px - maxY, maxY - window_height_px);

            // Update camera matrix with new camera coordinates
            viewMatrix.mat[2][0] = -newCameraX;
            viewMatrix.mat[2][1] = -newCameraY;

        }

	}
	// Check for collisions between all moving entities
	for(uint i = 0; i < motion_container.components.size(); i++)
	{
		Motion& motion_i = motion_container.components[i];
		Entity entity_i = motion_container.entities[i];

		// note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
		for(uint j = i+1; j < motion_container.components.size(); j++)
		{
			Motion& motion_j = motion_container.components[j];
			if (collides(motion_i, motion_j))
			{
				Entity entity_j = motion_container.entities[j];
				// Create a collisions event
				// We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
				registry.collisions.emplace_with_duplicates(entity_i, entity_j);
				registry.collisions.emplace_with_duplicates(entity_j, entity_i);
			}
		}
	}
}