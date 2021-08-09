#include "FPSCounter.h"
#include "Importer/ImageImport.h"
#include "VksCommon.h"
#include <SDL2/SDL.h>
#include <VKWindow.h>

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
	int frameSize;

	/*	Decoded video frames.	*/
	std::array<VkImage, nrVideoFrames> videoFrames;
	std::array<VkDeviceMemory, nrVideoFrames> videoFrameMemory;

	/*	Stagning frames.	*/
	std::array<VkBuffer, nrVideoFrames> videoStagingFrames;
	std::array<VkDeviceMemory, nrVideoFrames> videoStagingFrameMemory;
	std::array<void *, nrVideoFrames> mapMemory;
	FPSCounter<float> fpsCounter;

	/*  */
	struct AVFormatContext *pformatCtx;
	struct AVCodecContext *pVideoCtx;
	struct AVCodecContext *pAudioCtx;

	/*  */
	int videoStream;
	int audioStream;
	unsigned int video_width;
	unsigned int video_height;
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
		this->setTitle(fmt::format("VideoPlayback {}", path));
		this->path = path;
	}
	~AVVideoPlaybackWindow(void) {
		av_frame_free(&frame);
		avcodec_free_context(&pAudioCtx);
		avcodec_free_context(&pVideoCtx);

		avformat_close_input(&pformatCtx);
		avformat_free_context(pformatCtx);
	}

	virtual void Release(void) override {

		for (int i = 0; i < nrVideoFrames; i++) {
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
			throw std::runtime_error("Failed to allocate memory for the 'AVFormatContext'");
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
			throw std::runtime_error(fmt::format("Failed to retrieve info from stream info : {}", buf));
		}

		struct AVStream *video_st = nullptr;
		struct AVStream *audio_st = nullptr;

		/*	Get video codecs.	*/
		for (unsigned int x = 0; x < this->pformatCtx->nb_streams; x++) {
			AVStream *stream = this->pformatCtx->streams[x];

			/*  */
			if (stream->codecpar) {

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
		}

		/*  Get selected codec parameters. */
		if (!video_st)
			throw std::runtime_error(fmt::format("Failed to find a video stream in {}.", path));

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

		/*	*/
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

		if (this->frame == nullptr || this->frameoutput == nullptr)
			throw std::runtime_error(fmt::format("Failed to allocate frame"));

		int m_bufferSize =
			av_image_get_buffer_size(AV_PIX_FMT_RGBA, this->pVideoCtx->width, this->pVideoCtx->height, 4);
		av_image_alloc(this->frameoutput->data, this->frameoutput->linesize, this->pVideoCtx->width,
					   this->pVideoCtx->height, AV_PIX_FMT_RGBA, 4);

		// AVPacket *pPacket = av_packet_alloc();
		this->sws_ctx = sws_getContext(this->pVideoCtx->width, this->pVideoCtx->height, this->pVideoCtx->pix_fmt,
									   this->pVideoCtx->width, this->pVideoCtx->height, AV_PIX_FMT_RGBA, SWS_BICUBIC,
									   nullptr, nullptr, nullptr);

		this->frame_timer = av_gettime() / 1000000.0;
	}

	virtual void Initialize(void) override {
		loadVideo(path.c_str());

		/*	*/
		for (unsigned int i = 0; i < videoFrames.size(); i++) {

			VKHelper::createBuffer(getDevice(), video_width * video_height * 4,
								   getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(),
								   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
								   videoStagingFrames[i], videoStagingFrameMemory[i]);

			VKS_VALIDATE(vkMapMemory(getDevice(), videoStagingFrameMemory[i], 0, video_width * video_height * 4, 0,
									 &mapMemory[i]));

			VKHelper::createImage(
				getDevice(), video_width, video_height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(), videoFrames[i], videoFrameMemory[i]);
		}
		onResize(width(), height());
	}

	virtual void onResize(int width, int height) override {

		nthVideoFrame = 0;

		for (size_t i = 0; i < getNrCommandBuffers(); i++) {
			VkCommandBuffer cmd = getCommandBuffers(i);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));
			VkBufferImageCopy imageCopyRegion{};
			imageCopyRegion.bufferOffset = 0;
			imageCopyRegion.bufferRowLength = 0;
			imageCopyRegion.bufferImageHeight = 0;
			imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.imageSubresource.layerCount = 1;
			imageCopyRegion.imageExtent.width = video_width;
			imageCopyRegion.imageExtent.height = video_height;
			imageCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(cmd, videoStagingFrames[nthVideoFrame], videoFrames[nthVideoFrame],
								   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

			VKHelper::transitionImageLayout(cmd, videoFrames[nthVideoFrame], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkImageBlit blitRegion{};
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
						   getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
						   VK_FILTER_NEAREST);
			VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
			nthVideoFrame = (nthVideoFrame + 1) % nrVideoFrames;
		}
		nthVideoFrame = 0;
	}

	virtual void draw(void) override {
		AVPacket *packet = av_packet_alloc();
		if (!packet) {
			throw std::runtime_error("failed to allocated memory for AVPacket");
		}

		int res, result;
		// res = av_seek_frame(this->pformatCtx, this->videoStream, 60000, AVSEEK_FLAG_FRAME);

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
						memcpy(mapMemory[nthVideoFrame], this->frameoutput->data[0], video_width * video_height * 4);
						VkMappedMemoryRange stagingRange{};
						stagingRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
						stagingRange.memory = videoStagingFrameMemory[nthVideoFrame];
						stagingRange.offset = 0;
						stagingRange.size = video_width * video_height * 4;
						VKS_VALIDATE(vkFlushMappedMemoryRanges(getDevice(), 1, &stagingRange));
						VKS_VALIDATE(vkDeviceWaitIdle(getDevice()));
						nthVideoFrame = (nthVideoFrame + 1) % nrVideoFrames;
					}
				}
			}
			if (packet->stream_index == this->audioStream) {
			}
		}
		av_packet_unref(packet);
		av_packet_free(&packet);
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