// #include "ImGuiModule.h"
// #include "ImGuiDebuggerOverlay.h"
// #include "LainModule.h"
// #include "RendererResourcePool.h"
// #include "SDL_video.h"
// #include "ShaderLoader.h"
// #include "VKWindow.h"
// #include "backends/imgui_impl_sdl2.h"
// #include "backends/imgui_impl_vulkan.h"
// #include "engine.h"
// #include "vulkan/vulkan_core.h"

// static ImGui_ImplVulkanH_Window g_MainWindowData;

// static void check_vk_result(VkResult err) {
// 	if (err == VK_SUCCESS) {
// 		return;
// 	}
// 	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
// 	if (err < 0) {
// 		abort();
// 	}
// }

// ImGuiModule::ImGuiModule() { this->setName("ImGui Module"); }

// void ImGuiModule::onInitialization(LainEngine *engine) {
// 	LainModule::onInitialization(engine);
// 	/*  */

// 	// Create Framebuffers
// 	SDL_Window *window = (SDL_Window *)engine->windows[0]->getNativeInternalPtr();
// 	VkSurfaceKHR surface = engine->windows[0]->getSurface();
// 	int width = 0, height = 0;

// 	ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;
// 	wd->Surface = surface;

// 	// Setup Dear ImGui context
// 	IMGUI_CHECKVERSION();
// 	ImGui::CreateContext();
// 	ImGuiIO &io = ImGui::GetIO();
// 	(void)io;
// 	io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
// 	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
// 	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

// 	// Setup Dear ImGui style
// 	ImGui::StyleColorsDark();

// 	ImGui_ImplVulkan_InitInfo init_info = {};
// 	init_info.ApiVersion = VK_API_VERSION_1_3; // Pass in your value of VkApplicationInfo::apiVersion,
// 	// otherwise will default to header version.
// 	init_info.Instance = engine->getRenderingCore().core->getHandle();
// 	init_info.PhysicalDevice = engine->getRenderingCore().ldevice->getPhysicalDevice(0)->getHandle();
// 	init_info.Device = engine->getRenderingCore().ldevice->getHandle();
// 	init_info.QueueFamily = engine->getRenderingCore().graphics_queue_node_index;
// 	init_info.Queue = engine->getRenderingCore().graphic_queue;

// 	init_info.RenderPass = engine->defaultFrameBuffer.renderpass;
// 	init_info.Subpass = 0;
// 	init_info.MinImageCount = 2;
// 	init_info.ImageCount = 2;
// 	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
// 	init_info.Allocator = engine->getRenderingCore().getAllocatorCallback();
// 	init_info.CheckVkResultFn = check_vk_result;

// 	// Setup scaling
// 	float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
// 	ImGuiStyle &style = ImGui::GetStyle();
// 	style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style changing
// 									 // this requires resetting Style + calling this again)
// 	style.FontScaleDpi = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
// 	// 								 // unnecessary. We leave both here for documentation purpose)

// 	init_info.PipelineCache = engine->getRenderingCore().getShaderLoader().getPipelineCache();
// 	init_info.DescriptorPool = engine->getResourcePool().getDescriptorPool(0);

// 	// Setup Platform/Renderer backends
// 	if (!ImGui_ImplSDL2_InitForVulkan(window)) {
// 		engine->getLogger().error("Failed to init SDL with Vulkan");
// 	}

// 	if (!ImGui_ImplVulkan_Init(&init_info)) {
// 		engine->getLogger().error("Failed to init ImGui Vulkan");
// 	}

// 	this->renderPipe = new ImGuiRenderPipeline(*engine);
// }
