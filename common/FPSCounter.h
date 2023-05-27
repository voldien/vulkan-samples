#pragma once
#include <stddef.h>

/**
 * @brief
 *
 * @tparam T
 */
template <typename T = double> class FPSCounter {
  public:
	FPSCounter(int nrFPSSample = 50, long int timeResolution = 1000000000) {
		this->fpsSample = nrFPSSample;
		this->timeResolution = timeResolution;

		this->averageFPS = 0;
		this->totalFPS = 0;
	}

	void enabled(bool status) {}

	void incrementFPS(long int timeSample) noexcept {

		if (totalFPS % fpsSample == 0) {
			/*  Compute number average FPS.  */
			long int deltaTime = timeSample - prevTimeSample;
			T sec = static_cast<T>(deltaTime) / static_cast<T>(timeResolution);
			averageFPS = static_cast<T>(fpsSample) / sec;
			prevTimeSample = timeSample;
		}
		totalFPS++;
	}

	void update(float elapsedTime) {
		if (totalFPS % fpsSample == 0) {
		}
		totalFPS++;
	}

	unsigned int getFPS() const noexcept { return averageFPS; }

  protected:
	void internal_update(long int timeSample) {
		if (totalFPS % fpsSample == 0) {
			/*  Compute number average FPS.  */
			long int deltaTime = timeSample - prevTimeSample;
			T sec = static_cast<T>(deltaTime) / static_cast<T>(timeResolution);
			averageFPS = static_cast<T>(fpsSample) / sec;
			prevTimeSample = timeSample;

			this->totalFPS = 0;
		}
	}

  private:
	size_t totalFPS;
	int fpsSample;
	unsigned int averageFPS;
	long int prevTimeSample;
	long int timeResolution;
};
