#ifndef _VKS_SAMPLE_COMMON_TIME_H_
#define _VKS_SAMPLE_COMMON_TIME_H_ 1
#include <SDL2/SDL_timer.h>

namespace vkscommon {

	class Time {
	  public:
		Time(void) {
			this->_private_level_startup = SDL_GetPerformanceCounter();

			this->timeResolution = SDL_GetPerformanceFrequency();
		}

		void start(void) { this->ticks = SDL_GetPerformanceCounter(); }

		float getElapsed() const noexcept {
			return (float)(SDL_GetPerformanceCounter() - this->_private_level_startup) / (float)this->timeResolution;
		}
		float deltaTime() const noexcept { return (float)delta_data / (float)this->timeResolution; }
		void update() {
			delta_data = SDL_GetPerformanceCounter() - ticks;
			ticks = SDL_GetPerformanceCounter();
		}

	  private:
		/*  */
		long int ticks;
		float scale;
		float fixed;

		/*	TODO clean up later by relocating it to the time class.*/
		float gc_fdelta;
		float delta_data;
		// unsigned int nDeltaTime = sizeof(delta_data) / sizeof(delta_data[0]);
		unsigned int idelta;

		/*  */

		unsigned long timeResolution;
		unsigned long _private_level_startup;
	};

} // namespace vkscommon

#endif