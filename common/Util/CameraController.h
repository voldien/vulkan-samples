#pragma once
#include <SDL2/SDL_keyboard.h>
#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include <flythrough_camera.h>
#include <glm/glm.hpp>

class CameraController {
  public:
	CameraController(void) = default;

	void update(float delta) {
		const Uint8 *state = SDL_GetKeyboardState(NULL);
		bool w = state[SDLK_w];
		bool a = state[SDLK_a];
		bool s = state[SDLK_s];
		bool d = state[SDLK_d];

		float xDiff = 0.01f;
		float yDiff = 0.01f;
		flythrough_camera_update(&pos[0], &look[0], &up[0], &view[0][0], delta, 100.0f * 1, 0.5f * activated, fov,
								 xDiff, yDiff, w, a, s, d, 0, 0, 0);
	}
	const glm::mat4 &getViewMatrix(void) const noexcept { return this->view; }

  private:
	float fov = 80.0f;
	float speed = 100;
	float activated = 1.0f;

	glm::mat4 view;
	glm::vec3 pos = {0.0f, 0.0f, 0.0f};
	glm::vec3 look = {0.0f, 0.0f, 1.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
};
