#include "common.hpp"
#include <VKWindow.h>

/**
 * 
 */
class VideoPlaybackWindow : public VKWindow {
  public:
};

int main(int argc, const char **argv) {

	VulkanCore core(argc, argv);
	VideoPlaybackWindow window(core);

	window.run();
	return EXIT_SUCCESS;
}