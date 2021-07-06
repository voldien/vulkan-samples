#include "FPSCounter.h"
#include "Importer/ImageImport.h"
#include "VKHelper.h"
#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <VKWindow.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/vdpau.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

class AVVideoPlaybackWindow : public VKWindow {
  private:
	static const int nrVideoFrames = 2;
	int nthVideoFrame = 0;

	std::array<VkImage, nrVideoFrames> videoFrames;
	//std::array<VkImageView, nrVideoFrames> videoImageViews;
	std::array<VkDeviceMemory, nrVideoFrames> videoFrameMemory;

	std::array<VkBuffer, nrVideoFrames> videoStagingFrames;
	std::array<VkDeviceMemory, nrVideoFrames> videoStagingFrameMemory;

	FPSCounter<float> fpsCounter;

	/*  */
	struct AVFormatContext *pformatCtx;
	struct AVCodecContext *pVideoCtx;
	struct AVCodecContext *pAudioCtx;

	/*  */
	int videoStream;
	int audioStream;
	int video_width;
	int video_height;
	/*  */
	struct AVFrame *frame;
	struct AVFrame *frameoutput;
	struct SwsContext *sws_ctx;

	unsigned int flag;
	double video_clock;
	double frame_timer;
	double frame_last_pts;
	double frame_last_delay;

	std::string path;

  public:
	AVVideoPlaybackWindow(const char *path, std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->path = path;
	}
	~AVVideoPlaybackWindow(void) {
		avformat_close_input(&pformatCtx);
		// av_packet_free(&pPacket);
		av_frame_free(&frame);
		avcodec_free_context(&pVideoCtx);
	}

	virtual void Release(void) override {

		for (int i = 0; i < nrVideoFrames; i++) {
			//vkDestroyImageView(getDevice(), videoImageViews[i], nullptr);
			vkDestroyImage(getDevice(), videoFrames[i], nullptr);
			vkFreeMemory(getDevice(), videoFrameMemory[i], nullptr);
			vkFreeMemory(getDevice(), videoStagingFrameMemory[i], nullptr);
			vkDestroyBuffer(getDevice(), videoStagingFrames[i], nullptr);
		}
	}

	void loadVideo(const char *path) {
		int result;

		this->pformatCtx = avformat_alloc_context();
		if (!pformatCtx) {
			throw std::runtime_error("ERROR could not allocate memory for Format Context");
		}
		// Determine the input-format:
		this->pformatCtx->iformat = av_find_input_format(path);

		result = avformat_open_input(&this->pformatCtx, path, nullptr, nullptr);
		if (result != 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw std::runtime_error(fmt::format("Failed to open input : %s", buf));
		}

		if ((result = avformat_find_stream_info(this->pformatCtx, nullptr)) < 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw std::runtime_error(fmt::format("Failed to retrieve info from stream info : %s", buf));
		}
		struct AVStream *video_st = nullptr;
		struct AVStream *audio_st = nullptr;

		/*	Get video codecs.	*/
		for (int x = 0; x < this->pformatCtx->nb_streams; x++) {
			AVStream *stream = this->pformatCtx->streams[x];

			/*  */
			if (stream->codecpar == nullptr)
				continue;

			switch (stream->codecpar->codec_type) {
			case AVMEDIA_TYPE_AUDIO:
				this->audioStream = x;
				audio_st = stream;
				break;
			case AVMEDIA_TYPE_SUBTITLE:
				break;
			case AVMEDIA_TYPE_VIDEO:
				this->videoStream = x;
				video_st = stream;
				break;
			}
		}

		/*  Get selected codec parameters. */
		if (!video_st)
			throw std::runtime_error("Failed to find video stream.");

		if (audio_st) {
			AVCodecParameters *pAudioCodecParam = audio_st->codecpar;

			/*  Create audio clip.  */
			AVCodec *audioCodec = avcodec_find_decoder(pAudioCodecParam->codec_id);
			this->pAudioCtx = avcodec_alloc_context3(audioCodec);
			if (!this->pAudioCtx)
				throw std::runtime_error("Failed to create audio decode context");

			result = avcodec_parameters_to_context(this->pAudioCtx, pAudioCodecParam);
			if (result < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw std::runtime_error(fmt::format("Failed to set codec parameters : {}", buf));
			}

			result = avcodec_open2(this->pAudioCtx, audioCodec, nullptr);
			if (result < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw std::runtime_error(fmt::format("Failed to retrieve info from stream info : {}", buf));
			}
		}

		AVCodecParameters *pVideoCodecParam = video_st->codecpar;

		AVCodec *pVideoCodec = avcodec_find_decoder(pVideoCodecParam->codec_id);
		if (pVideoCodec == nullptr)
			throw std::runtime_error("failed to find decoder");
		this->pVideoCtx = avcodec_alloc_context3(pVideoCodec);
		if (this->pVideoCtx == nullptr)
			throw std::runtime_error("Failed to allocate video decoder context");

		// AV_PIX_FMT_FLAG_RGB
		/*  Modify the target pixel format. */
		// this->pVideoCtx->get_format = get_format;
		//	pVideoCodecParam->format = AV_PIX_FMT_BGR24;
		//	pVideoCodecParam->codec_tag = avcodec_pix_fmt_to_codec_tag(AV_PIX_FMT_BGR24);
		//	pVideoCodecParam->color_space = AVCOL_SPC_RGB;
		result = avcodec_parameters_to_context(this->pVideoCtx, pVideoCodecParam);
		if (result < 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw std::runtime_error(fmt::format("Failed to set codec parameters : {}", buf));
		}
		// av_find_best_pix_fmt_of_2
		// avcodec_default_get_format()

		if ((result = avcodec_open2(this->pVideoCtx, pVideoCodec, nullptr)) != 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw std::runtime_error(fmt::format("Failed to retrieve info from stream info : {}", buf));
		}

		video_width = this->pVideoCtx->width;
		video_height = this->pVideoCtx->height;

		this->frame = av_frame_alloc();
		this->frameoutput = av_frame_alloc();
		int m_bufferSize =
			av_image_get_buffer_size(AV_PIX_FMT_BGRA, this->pVideoCtx->width, this->pVideoCtx->height, 4);
		void *m_buffer = (uint8_t *)av_malloc(m_bufferSize);
		av_image_alloc(this->frameoutput->data, this->frameoutput->linesize, this->pVideoCtx->width,
					   this->pVideoCtx->height, AV_PIX_FMT_BGRA, 4);

		if (this->frame == nullptr)
			throw std::runtime_error(fmt::format("Failed to allocate frame"));

		// m_bufferSize = avpicture_get_size(PIX_FMT_RGB24, width, height);
		// m_buffer = (uint8_t *)av_malloc(m_bufferSize);

		// m_bufferSize = avpicture_get_size(PIX_FMT_RGB24, width, height);
		// m_buffer = (uint8_t *)av_malloc(m_bufferSize);

		// AVPacket *pPacket = av_packet_alloc();
		this->sws_ctx = sws_getContext(this->pVideoCtx->width, this->pVideoCtx->height, this->pVideoCtx->pix_fmt,
									   this->pVideoCtx->width, this->pVideoCtx->height, AV_PIX_FMT_BGRA, SWS_BICUBIC,
									   nullptr, nullptr, nullptr);

		this->frame_timer = av_gettime() / 1000000.0;
	}

	virtual void Initialize(void) override {
		loadVideo(path.c_str());
		/*	*/
		for (unsigned int i = 0; i < videoFrames.size(); i++) {

			VKHelper::createBuffer(getDevice(), video_width * video_height * 3,
											  getLogicalDevice()->getPhysicalDevice(0)->getMemoryProperties(),
								  VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
								  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
								  videoStagingFrames[i], videoStagingFrameMemory[i]);

			VKHelper::createImage(
				getDevice(), video_width, video_height, 1, VK_FORMAT_R8G8B8_SRGB, VK_IMAGE_TILING_LINEAR,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, getLogicalDevice()->getPhysicalDevice(0)->getMemoryProperties(),
				videoFrames[i], videoFrameMemory[i]);
		}
		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		nthVideoFrame = 0;

		for (size_t i = 0; i < getCommandBuffers().size(); i++) {
			VkCommandBuffer cmd = getCommandBuffers()[i];

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));
			VkBufferImageCopy imageCopyRegion{};
			imageCopyRegion.bufferOffset = 0;
			imageCopyRegion.bufferRowLength = video_width;
			imageCopyRegion.bufferRowLength = video_height;
			imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.imageSubresource.layerCount = 1;
			imageCopyRegion.imageExtent.width = width;
			imageCopyRegion.imageExtent.height = height;
			imageCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(cmd, videoStagingFrames[nthVideoFrame], videoFrames[nthVideoFrame],
						   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, &imageCopyRegion);

			// VKHelper::transitionImageLayout(cmd, videoFrames[nthVideoFrame], VK_IMAGE_LAYOUT_UNDEFINED,
			// 								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkImageBlit blitRegion {};
			blitRegion.srcOffsets[1].x = video_width;
			blitRegion.srcOffsets[1].y = video_height;
			blitRegion.srcOffsets[1].z = 1;
			blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcSubresource.mipLevel = 0;
			blitRegion.dstOffsets[1].x = width;
			blitRegion.dstOffsets[1].y = height;
			blitRegion.dstOffsets[1].z = 1;
			blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstSubresource.mipLevel = 0;

			vkCmdBlitImage(cmd, videoFrames[nthVideoFrame], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   getSwapChainImages()[i], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, &blitRegion,
						   VK_FILTER_NEAREST);
			VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_GENERAL,
											VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
			nthVideoFrame = (nthVideoFrame + 1) % nrVideoFrames;
		}
		nthVideoFrame = 0;
	}

	virtual void draw(void) override {
		// TODO relocate.
		// TODO add audio decoder.
		AVPacket *packet = av_packet_alloc();
		if (!packet) {
			throw std::runtime_error("failed to allocated memory for AVPacket");
		}

		int res, result;
		res = av_seek_frame(this->pformatCtx, this->videoStream, 60000, AVSEEK_FLAG_FRAME);
		// while (true) {

		res = av_read_frame(this->pformatCtx, packet);
		if (res == 0) {
			if (packet->stream_index == this->videoStream) {
				result = avcodec_send_packet(this->pVideoCtx, packet);
				if (result < 0) {
					char buf[AV_ERROR_MAX_STRING_SIZE];
					av_strerror(result, buf, sizeof(buf));
					throw std::runtime_error(fmt::format("Failed to send packet for decoding picture frame : {}", buf));
				}

				while (result >= 0) {
					result = avcodec_receive_frame(this->pVideoCtx, this->frame);
					if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
						break;
					if (result < 0) {
						char buf[AV_ERROR_MAX_STRING_SIZE];
						av_strerror(result, buf, sizeof(buf));
						throw std::runtime_error(fmt::format(" : {}", buf));
					}
					if (this->frame->format != AV_PIX_FMT_BGRA) {

						if (this->frame->format == AV_PIX_FMT_YUV420P) {
							this->frame->data[0] =
								this->frame->data[0] + this->frame->linesize[0] * (this->pVideoCtx->height - 1);
							this->frame->data[1] =
								this->frame->data[1] + this->frame->linesize[0] * this->pVideoCtx->height / 4 - 1;
							this->frame->data[2] =
								this->frame->data[2] + this->frame->linesize[0] * this->pVideoCtx->height / 4 - 1;

							this->frame->linesize[0] *= -1;
							this->frame->linesize[1] *= -1;
							this->frame->linesize[2] *= -1;
							sws_scale(this->sws_ctx, this->frame->data, this->frame->linesize, 0, this->frame->height,
									  this->frameoutput->data, this->frameoutput->linesize);
							/*	Upload the image to staging.	*/
							void *_data;
							VKS_VALIDATE(vkMapMemory(getDevice(), videoStagingFrameMemory[nthVideoFrame], 0,
													 video_width * video_height * 3, 0, &_data));
							memcpy(_data, this->frameoutput->data, video_width * video_height * 3);
							vkUnmapMemory(getDevice(), videoStagingFrameMemory[nthVideoFrame]);
							VKS_VALIDATE(vkDeviceWaitIdle(getDevice()));
							nthVideoFrame = (nthVideoFrame + 1) % nrVideoFrames;
						}
					}
				}
			}
			if (packet->stream_index == this->audioStream) {
				// result = avcodec_send_packet(this->pAudioCtx, packet);
				// if (result < 0) {
				// 	char buf[AV_ERROR_MAX_STRING_SIZE];
				// 	av_strerror(result, buf, sizeof(buf));
				// 	throw std::runtime_error(
				// 		fmt::format("Failed to send packet for decoding audio frame : %s", buf));
				// }

				// while (result >= 0) {
				// 	result = avcodec_receive_frame(this->pAudioCtx, this->frame);
				// 	if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
				// 		break;
				// 	if (result < 0) {
				// 		char buf[AV_ERROR_MAX_STRING_SIZE];
				// 		av_strerror(result, buf, sizeof(buf));
				// 		throw std::runtime_error(fmt::format(" : %s", buf));
				// 	}

				// 	av_get_channel_layout_nb_channels(this->frame->channel_layout);
				// 	this->frame->format != AV_SAMPLE_FMT_S16P;
				// 	this->frame->channel_layout;
				// 	// clip->setData(this->frame->extended_data[0], this->frame->linesize[0], 0);
				// }
			}

			printf("duration %f\n", (float)packet->duration);
		}
		av_packet_unref(packet);
		av_packet_free(&packet);
		//}
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_device_extensions = {};
	try {
		std::shared_ptr<VulkanCore> core = std::make_shared<VulkanCore>();
		std::vector<std::shared_ptr<PhysicalDevice>> devices = core->createPhysicalDevices();
		printf("%s\n", devices[0]->getDeviceName());
		std::shared_ptr<VKDevice> d = std::make_shared<VKDevice>(devices);
		AVVideoPlaybackWindow window(argv[1], core, d);

		window.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}