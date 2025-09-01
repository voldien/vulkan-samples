#include "Importer/ImageImport.h"
#include "Scene/CameraController.h"
#include "Util/ShaderLoader.h"
#include "VKDataStructure.h"
#include "VKSample.h"
#include "vulkan/vulkan_core.h"
#include <Util/IOUtil.h>
#include <Util/PipelineLayoutUtil.h>
#include <VKWindow.h>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace vksample {

	/**
	 *
	 */
	class VectorField2D : public VKBaseSampleWindow {
	  private:
		using Motion = struct motion_t {
			glm::vec2 normalizedPos{}; /*  Position in pixel space.    */
			glm::vec2 velocity{};	   /*  direction and magnitude of mouse movement.  */
			float radius = 10.0f;	   /*  Radius of incluense, also the pressure of input.    */
			float amplitude = 1.0;
			float noise = 0;
			float pad2 = 0;
		};

		using ParticleSetting = struct particle_setting_t {
			glm::uvec4 particleBox = glm::uvec4(256, 256, 1, 0);
			glm::uvec4 vectorfieldbox = glm::uvec4(32, 32, 32, 0); // Dummy

			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
			float strength = 1.0f;

			float density = 1.0f;
			uint32_t nrparticles{};
			float spriteSize = 0.25f;
			float dragMag = 0.1f;
		};

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};
			glm::vec4 color = glm::vec4(1, 0.1, 0.1, 1);

			/*	*/
			ParticleSetting particleSetting;
			Motion motion;

			/*	*/
			float delta{};

		} uniformStageBuffer;

		using Particle = struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity, mass	*/
		};

		using VectorForce = struct vector_force_t {
			glm::vec3 position; /*	*/
			glm::vec3 force;	/*	*/
		};

		UBOObject ParticleBuffer{};
		MeshObject particlesMesh;

		/*	*/
		UBOObject UniformBuffer{};
		std::vector<void *> mapMemory;
		size_t UniformParamMemSize = sizeof(uniformStageBuffer);

		/*	*/
		VkSampler sampler = VK_NULL_HANDLE;
		Texture texture{};

		/*	Compute pipeline.	*/

		ComputePipeline particleResetCompute{};
		ComputePipeline particleSimulationCompute{};
		ComputePipeline particleMotionForceCompute{};
		GraphicPipeline *particleGraphic{};
		GraphicPipeline *vectorFieldGraphic{};

		// VkPipeline particleSim = VK_NULL_HANDLE;
		// VkPipelineLayout particleSimLayout = VK_NULL_HANDLE;
		// VkDescriptorSetLayout particleComputeDescriptorSetLayout = VK_NULL_HANDLE;
		// std::vector<VkDescriptorSet> particleComputeDescriptorSets;

		// /*	Graphic pipelines.	*/
		// VkPipeline particleGraphicPipeline = VK_NULL_HANDLE;
		// VkPipelineLayout particleGraphicLayout = VK_NULL_HANDLE;
		// VkDescriptorSetLayout particleGraphicDescriptorSetLayout = VK_NULL_HANDLE;
		// std::vector<VkDescriptorSet> particleGraphicDescriptorSets;

		/*	*/
		CameraController cameraController;

		std::array<size_t, 3> localWorkGroupSizeReset{};
		std::array<size_t, 3> localWorkGroupSizeSimulation{};
		std::array<size_t, 3> localWorkGroupSizeMotion{};

		std::vector<VkDescriptorSet> particleComputeDescriptorSets;
		std::vector<VkDescriptorSet> particleGraphicDescriptorSets;

		const size_t particle_multiple_count = 8;

		/*	*/
		size_t nrParticles = 0;
		const size_t nrParticleBuffers = 2;
		size_t ParticleMemorySize = 0;

		/*	Particle.	*/
		const std::string particleVertexShaderPath = "Shaders/vectorfield/particle.vert.spv";
		const std::string particleGeometryShaderPath = "Shaders/vectorfield/particle.geom.spv";
		const std::string particleFragmentShaderPath = "Shaders/vectorfield/particle.frag.spv";

		const std::string particleInitComputeShaderPath = "Shaders/vectorfield/init_particle2D.comp.spv";
		/*	Particle Simulation in Vector Field.	*/
		const std::string particleSimulationComputeShaderPath = "Shaders/vectorfield/particle2D.comp.spv";
		/*	Particle Simulation in Vector Field.	*/
		const std::string particleMotionForceComputeShaderPath = "Shaders/vectorfield/apply_force_2D.comp.spv";

		/*	Motion vector graphic shader.	*/
		const std::string vectorFieldVertexShaderPath = "Shaders/vectorfield/vectorField.vert.spv";
		const std::string vectorFieldGeometryShaderPath = "Shaders/vectorfield/motion2D.geom.spv";
		const std::string vectorFieldFragmentPath = "Shaders/vectorfield/vectorField.frag.spv";

	  public:
		VectorField2D(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
			: VKBaseSampleWindow(core, device, -1, -1, -1, -1) {
			this->setTitle("VectorField2D");
			this->show();
		}

		~VectorField2D() override = default;

		void release() override { vkDestroySampler(getDevice(), sampler, nullptr); }

		void initDescriptor() {

			// TODO fix descriptor set allocation.
			std::vector<VkDescriptorSetLayout> layouts(getSwapChainImageCount(),
													   particleSimulationCompute.setLayout[0]);
			// for (int i = 0; i < getSwapChainImageCount(); i++) {
			//	layouts.push_back(particleSimulationCompute.setLayout);
			// }

			VkDescriptorSetAllocateInfo allocdescInfo{};
			allocdescInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocdescInfo.descriptorPool = getDescriptorPool();
			allocdescInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
			allocdescInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> descSet(allocdescInfo.descriptorSetCount, VK_NULL_HANDLE);

			/*	Allocate descriptor set for both compute and graphic pipeline.	*/
			VKS_VALIDATE(vkAllocateDescriptorSets(this->getDevice(), &allocdescInfo, descSet.data()));

			/*	*/
			particleComputeDescriptorSets.resize(getSwapChainImageCount());
			particleGraphicDescriptorSets.resize(getSwapChainImageCount());

			/*	*/
			for (int i = 0; i < getSwapChainImageCount(); i++) {
				particleComputeDescriptorSets[i] = descSet[i];
			}
			for (int i = 0; i < getSwapChainImageCount(); i++) {
				particleGraphicDescriptorSets[i] = descSet[getSwapChainImageCount() + i];
			}

			assert(particleComputeDescriptorSets.size() == getSwapChainImageCount());
			assert(particleGraphicDescriptorSets.size() == getSwapChainImageCount());

			for (size_t index = 0; index < particleComputeDescriptorSets.size(); index++) {
				std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

				VkDescriptorBufferInfo bufferReadParticleInfo{};
				bufferReadParticleInfo.buffer = ParticleBuffer.buffer;
				bufferReadParticleInfo.offset = (index % nrParticleBuffers) * ParticleMemorySize;
				bufferReadParticleInfo.range = ParticleMemorySize;

				VkDescriptorBufferInfo bufferWriteParticleInfo{};
				bufferWriteParticleInfo.buffer = ParticleBuffer.buffer;
				bufferWriteParticleInfo.offset = ((index + 1) % nrParticleBuffers) * ParticleMemorySize;
				bufferWriteParticleInfo.range = ParticleMemorySize;

				VkDescriptorBufferInfo bufferUniformInfo{};
				bufferUniformInfo.buffer = UniformBuffer.buffer;
				bufferUniformInfo.offset = (index % getSwapChainImageCount()) * this->UniformParamMemSize;
				bufferUniformInfo.range = UniformParamMemSize;

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = particleComputeDescriptorSets[index];
				descriptorWrites[0].dstBinding = 1;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = nullptr;
				descriptorWrites[0].pBufferInfo = &bufferReadParticleInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = particleComputeDescriptorSets[index];
				descriptorWrites[1].dstBinding = 2;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pImageInfo = nullptr;
				descriptorWrites[1].pBufferInfo = &bufferWriteParticleInfo;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = particleComputeDescriptorSets[index];
				descriptorWrites[2].dstBinding = 0;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = nullptr;
				descriptorWrites[2].pBufferInfo = &bufferUniformInfo;

				vkUpdateDescriptorSets(this->getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
									   descriptorWrites.data(), 0, nullptr);
			}

			for (size_t index = 0; index < particleGraphicDescriptorSets.size(); index++) {
				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = UniformBuffer.buffer;
				bufferInfo.offset = index * UniformParamMemSize;
				bufferInfo.range = UniformParamMemSize;

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = VK_NULL_HANDLE;
				imageInfo.sampler = sampler;

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = particleGraphicDescriptorSets[index];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = nullptr;
				descriptorWrites[0].pBufferInfo = &bufferInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = particleGraphicDescriptorSets[index];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = nullptr;
				descriptorWrites[1].pImageInfo = &imageInfo;

				vkUpdateDescriptorSets(getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			}
		}

		void Initialize() override {

			/*	*/
			const std::string particleTexturePath = this->getResult()["texture"].as<std::string>();

			/*	Load and create Texture.	*/
			{
				ImageImporter imageImporter(this->getFileSystem(), *this);

				imageImporter.loadTexture2D(particleTexturePath.c_str(), texture, ColorSpace::RawLinear);

				this->texture.imageView = VKHelper::createImageView(getDevice(), this->texture.image,
																	VK_IMAGE_VIEW_TYPE_2D, this->texture.internalformat,
																	VK_IMAGE_ASPECT_COLOR_BIT, this->texture.mipLevels);

				VKHelper::createSampler(getDevice(), sampler, 0);
			}

			/*	Loader Shader Pipelines.	*/
			{
				ShaderLoader shaderLoader(*this);

				const auto vertParticleShaderCode =
					fragcore::IOUtil::readFileData<uint32_t>(this->particleVertexShaderPath, this->getFileSystem());
				const auto geomParticleShaderCode =
					fragcore::IOUtil::readFileData<uint32_t>(this->particleGeometryShaderPath, this->getFileSystem());
				const auto fragParticleShaderCode =
					fragcore::IOUtil::readFileData<uint32_t>(this->particleFragmentShaderPath, this->getFileSystem());

				const auto vertVectorFieldShaderCode =
					fragcore::IOUtil::readFileData<uint32_t>(this->vectorFieldVertexShaderPath, this->getFileSystem());
				const auto geomVectorFieldShaderCode = fragcore::IOUtil::readFileData<uint32_t>(
					this->vectorFieldGeometryShaderPath, this->getFileSystem());
				const auto fragVectorFieldShaderCode =
					fragcore::IOUtil::readFileData<uint32_t>(this->vectorFieldFragmentPath, this->getFileSystem());

				this->particleGraphic = shaderLoader.loadGraphicProgram(
					&vertParticleShaderCode, &fragParticleShaderCode, &geomParticleShaderCode);
				this->vectorFieldGraphic = shaderLoader.loadGraphicProgram(
					&vertVectorFieldShaderCode, &geomVectorFieldShaderCode, &fragVectorFieldShaderCode);

				/*	Compute Pipelines.	*/
				const std::vector<uint32_t> particle_reset_compute_binary = fragcore::IOUtil::readFileData<uint32_t>(
					this->particleInitComputeShaderPath, this->getFileSystem());
				this->localWorkGroupSizeReset = PipelineLayoutUtil::getLocalSize(particle_reset_compute_binary).value();
				this->particleResetCompute = shaderLoader.loadComputeProgram(&particle_reset_compute_binary);

				const std::vector<uint32_t> particle_simulation_compute_binary =
					fragcore::IOUtil::readFileData<uint32_t>(this->particleSimulationComputeShaderPath,
															 this->getFileSystem());
				this->localWorkGroupSizeSimulation =
					PipelineLayoutUtil::getLocalSize(particle_simulation_compute_binary).value();
				this->particleSimulationCompute = shaderLoader.loadComputeProgram(&particle_simulation_compute_binary);

				const std::vector<uint32_t> particle_motion_compute_binary = fragcore::IOUtil::readFileData<uint32_t>(
					this->particleMotionForceComputeShaderPath, this->getFileSystem());
				this->localWorkGroupSizeMotion =
					PipelineLayoutUtil::getLocalSize(particle_motion_compute_binary).value();
				this->particleMotionForceCompute = shaderLoader.loadComputeProgram(&particle_motion_compute_binary);
			}

			/*	Allocate and setup Uniform Buffers.	*/
			{
				this->UniformParamMemSize =
					Math::align(sizeof(uniformStageBuffer),
								this->getPhysicalDevice()->getDeviceLimits().minUniformBufferOffsetAlignment);

				const VkDeviceSize particleUniformBufferSize = this->UniformParamMemSize * getSwapChainImageCount();

				/*	Create pipelines.	*/
				VkBufferUsageFlags usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

				VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
												 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
												 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

				this->allocateBuffer(particleUniformBufferSize, usageFlag, memFlags, ParticleBuffer.buffer,
									 ParticleBuffer.memory);

				uint8_t *_data = nullptr;
				VKS_VALIDATE(vkMapMemory(this->getDevice(), UniformBuffer.memory, this->UniformParamMemSize,
										 this->UniformParamMemSize, 0, (void **)&_data));
				/*	Allocate memory.	*/
				for (size_t index = 0; index < this->getSwapChainImageCount(); index++) {
					mapMemory.push_back((void *)&_data[this->UniformParamMemSize * index]);
				}
			}

			/*	Create particle buffer, on local device memory only.	*/
			{
				VkBufferUsageFlags usageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

				VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

				this->allocateBuffer(ParticleMemorySize * nrParticleBuffers, usageFlag, memFlags, ParticleBuffer.buffer,
									 ParticleBuffer.memory);
			}

			this->initDescriptor();

			this->onResize(width(), height());
		}

		void onResize(int width, int height) override {

			VKS_VALIDATE(vkQueueWaitIdle(getDefaultTransferQueue()));
			VKS_VALIDATE(vkQueueWaitIdle(getDefaultGraphicQueue()));

			/*	*/
			for (uint32_t index = 0; index < this->getNrCommandBuffers(); index++) {
				VkCommandBuffer cmd = getCommandBuffers(index);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;

				VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));

				/*	Update particles.	*/

				// TODO validate memory barrier, being not read for rendering.
				VKHelper::bufferBarrier(cmd, 0, VK_ACCESS_SHADER_WRITE_BIT, ParticleBuffer.buffer, ParticleMemorySize,
										(index % nrParticleBuffers) * ParticleMemorySize,
										VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->particleSimulationCompute.pipeline);
				// Bind descriptor
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->particleSimulationCompute.layout, 0,
										1, &this->particleComputeDescriptorSets[index], 0, nullptr);

				/*	Update particle simulation.	*/
				// vkCmdDispatch(cmd, nrParticles / localInvokation, 1, 1);

				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = getDefaultRenderPass();
				renderPassInfo.framebuffer = getFrameBuffer(index);
				renderPassInfo.renderArea.offset = {0, 0};
				renderPassInfo.renderArea.extent.width = width;
				renderPassInfo.renderArea.extent.height = height;

				std::array<VkClearValue, 2> clearValues{};
				clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
				clearValues[1].depthStencil = {1.0f, 0};
				renderPassInfo.clearValueCount = clearValues.size();
				renderPassInfo.pClearValues = clearValues.data();

				// Barrier between compute and vertex
				// TODO validate memory barrier. make sure that the particles has been updated before being read.
				VKHelper::bufferBarrier(cmd, 0, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, ParticleBuffer.buffer,
										ParticleMemorySize, (index % nrParticleBuffers) * ParticleMemorySize,
										VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

				vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = {
					.x = 0, .y = 0, .width = (float)width, .height = (float)height, .minDepth = 0, .maxDepth = 1.0f};
				vkCmdSetViewport(cmd, 0, 1, &viewport);

				/*	Setup particle rendering pipeline.	*/
				// vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicPipeline);
				// VkDeviceSize offsets[] = {(index % nrParticleBuffers) * ParticleMemorySize};
				// vkCmdBindVertexBuffers(cmd, 0, 1, &particleBuffer, offsets);
				// vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, particleGraphicLayout, 0, 1,
				// 						&particleGraphicDescriptorSets[index], 0, nullptr);

				// Draw particles
				// vkCmdDraw(cmd, nrParticles, 1, 0, 0);

				vkCmdEndRenderPass(cmd);

				VKS_VALIDATE(vkEndCommandBuffer(cmd));
			}

			this->cameraController.setAspect((float)width / (float)height);
		}

		void draw() override {}

		void update() override {
			this->cameraController.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				const float xHalf = this->uniformStageBuffer.particleSetting.particleBox.x / 2.f;
				const float yHalf = this->uniformStageBuffer.particleSetting.particleBox.y / 2.f;
				glm::mat4 proj = glm::ortho(-xHalf, xHalf, -yHalf, yHalf, -10.0f, 10.0f);

				glm::mat4 viewMatrix = glm::translate(glm::vec3(-xHalf, -yHalf, 0));
				/*	*/
				this->uniformStageBuffer.proj = proj;

				this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = viewMatrix;
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

				if (this->getInput().getMousePressed(Input::MouseButton::LEFT_BUTTON)) {
					int x = 0, y = 0;
					this->getInput().getMousePosition(&x, &y);
					this->uniformStageBuffer.motion.normalizedPos =
						glm::vec2(1, 1) - (glm::vec2(x, y) / glm::vec2(this->width(), this->height()));
					this->uniformStageBuffer.motion.normalizedPos.x =
						1.0f - this->uniformStageBuffer.motion.normalizedPos.x;
				}
			}

			/*	Update Memory.	*/
			/*	Copy uniform memory.	*/
			memcpy(this->mapMemory[getCurrentFrameIndex()], &uniformStageBuffer, sizeof(uniformStageBuffer));
		}
	};

	/*	*/
	class VectorFieldSample : public VKSample<VectorField2D> {
	  public:
		VectorFieldSample() : VKSample<VectorField2D>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Particle Texture Path",
					cxxopts::value<std::string>()->default_value("asset/particle-cell.png"));
		}
	};
} // namespace vksample

int main(int argc, const char **argv) {
	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};

	// TODO add custom argument options for adding path of the texture and what type.

	try {
		vksample::VectorFieldSample vectorField2D;
		vectorField2D.run(argc, argv, required_device_extensions, {}, required_instance_extensions);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}