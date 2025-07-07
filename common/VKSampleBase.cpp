#include "VKSampleBase.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace vksample;

VKSampleSessionBase::VKSampleSessionBase(std::shared_ptr<fvkcore::VulkanCore> &core,
										 std::shared_ptr<fvkcore::VKDevice> &device)
	: core(core), device(device) {
	this->loadDefaultQueue();
	this->getTimer().start();

	/* Create logger	*/
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_level(spdlog::level::trace);
	stdout_sink->set_color_mode(spdlog::color_mode::always);
	stdout_sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
	stdout_sink->set_pattern("%g:%# [%^%l%$] %v");

	/*	*/
	this->logger = new spdlog::logger("vulkan-sample", {stdout_sink});
	this->logger->set_level(spdlog::level::trace);
}