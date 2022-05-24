#pragma once
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include <flythrough_camera.h>
#include <glm/glm.hpp>

class CameraController {
  public:
	CameraController() = default;

	void update(float delta) {

		const Uint8 *state = SDL_GetKeyboardState(NULL);

		bool w = state[SDL_SCANCODE_W];
		bool a = state[SDL_SCANCODE_A];
		bool s = state[SDL_SCANCODE_S];
		bool d = state[SDL_SCANCODE_D];
		bool alt = state[SDL_SCANCODE_LALT];
		// mouse movement.
		SDL_PumpEvents(); // make sure we have the latest mouse state.

		int buttons = SDL_GetMouseState(&x, &y);

		float xDiff = -(xprev - x) * xspeed;
		float yDiff = -(yprev - y) * yspeed;
		xprev = x;
		yprev = y;
		if (!alt) {
			flythrough_camera_update(&pos[0], &look[0], &up[0], &view[0][0], delta, 100.0f * 1, 0.5f * activated, fov,
									 xDiff, yDiff, w, a, s, d, 0, 0, 0);
		}
	}
	const glm::mat4 &getViewMatrix() const noexcept { return this->view; }

	const glm::vec3 getPosition() const noexcept { return this->pos; }

  private:
	float fov = 80.0f;
	float speed = 100;
	float activated = 1.0f;
	float xspeed = 0.5f;
	float yspeed = 0.5f;

	int x, y, xprev, yprev;

	glm::mat4 view;
	glm::vec3 pos = {0.0f, 0.0f, 0.0f};
	glm::vec3 look = {0.0f, 0.0f, 1.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
};
