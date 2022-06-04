#include <FPSCounter.h>
#include <Importer/ImageImport.h>
#include <OpenALAudioInterface.h>
#include <VKSampleWindow.h>
#include <VKWindow.h>
#include <VksCommon.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
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

class AVVideoPlayback : public VKWindow {
  private:
	static const int nrVideoFrames = 3;
	int nthVideoFrame = 0;
	int frameSize;

	/*	Decoded video frames.	*/
	std::array<VkImage, nrVideoFrames> videoFrames;
	std::array<VkDeviceMemory, nrVideoFrames> videoFrameMemory;

	/*	Stagning frames.	*/
	VkBuffer videoStagingFrames;
	// TODO merge memory.
	VkDeviceMemory videoStagingFrameMemory;
	size_t videoStagingSize;
	std::array<void *, nrVideoFrames> mapMemory;
	std::shared_ptr<fragcore::OpenALAudioInterface> audioInterface;

	/*  */
	struct AVFormatContext *pformatCtx = nullptr;
	struct AVCodecContext *pVideoCtx = nullptr;
	struct AVCodecContext *pAudioCtx = nullptr;

	/*  */
	int videoStream;
	int audioStream;
	size_t video_width;
	size_t video_height;

	size_t audio_sample_rate;
	size_t audio_bit_rate;
	size_t audio_channel;

	/*  */
	struct AVFrame *frame = nullptr;
	struct AVFrame *frameoutput = nullptr;
	struct SwsContext *sws_ctx = nullptr;

	unsigned int flag;
	double video_clock;
	double frame_timer;
	double frame_last_pts;
	double frame_last_delay;

	std::string path;

  public:
	AVVideoPlayback(std::shared_ptr<VulkanCore> &core, std::shared_ptr<VKDevice> &device)
		: VKWindow(core, device, -1, -1, -1, -1) {
		this->setTitle(fmt::format("VideoPlayback {}", path));
		this->path = "asset/video.mp4";
		this->show();
	}
	virtual ~AVVideoPlayback() {
		if (this->frame)
			av_frame_free(&this->frame);
		avcodec_free_context(&this->pAudioCtx);
		avcodec_free_context(&this->pVideoCtx);

		avformat_close_input(&this->pformatCtx);
		avformat_free_context(this->pformatCtx);
	}

	virtual void release() override {

		for (size_t i = 0; i < nrVideoFrames; i++) {
			vkDestroyImage(this->getDevice(), videoFrames[i], nullptr);
			vkFreeMemory(this->getDevice(), videoFrameMemory[i], nullptr);
		}
		vkFreeMemory(this->getDevice(), videoStagingFrameMemory, nullptr);
		vkDestroyBuffer(this->getDevice(), videoStagingFrames, nullptr);
	}

	void loadVideo(const char *path) {
		int result;

		this->pformatCtx = avformat_alloc_context();
		if (!pformatCtx) {
			throw cxxexcept::RuntimeException("Failed to allocate memory for the 'AVFormatContext'");
		}
		// Determine the input-format:
		this->pformatCtx->iformat = av_find_input_format(path);

		result = avformat_open_input(&this->pformatCtx, path, nullptr, nullptr);
		if (result != 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw cxxexcept::RuntimeException("Failed to open input : %s", buf);
		}

		if ((result = avformat_find_stream_info(this->pformatCtx, nullptr)) < 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
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
			throw cxxexcept::RuntimeException("Failed to find a video stream in {}.", path);

		if (audio_st) {
			AVCodecParameters *pAudioCodecParam = audio_st->codecpar;

			/*  Create audio clip.  */
			AVCodec *audioCodec = avcodec_find_decoder(pAudioCodecParam->codec_id);
			this->pAudioCtx = avcodec_alloc_context3(audioCodec);
			if (!this->pAudioCtx)
				throw cxxexcept::RuntimeException("Failed to create audio decode context");

			result = avcodec_parameters_to_context(this->pAudioCtx, pAudioCodecParam);
			if (result < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to set codec parameters : {}", buf);
			}

			result = avcodec_open2(this->pAudioCtx, audioCodec, nullptr);
			if (result < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
			}

			this->audio_bit_rate = pAudioCodecParam->bit_rate;
			this->audio_sample_rate = pAudioCodecParam->sample_rate;
			this->audio_channel = pAudioCodecParam->channels;
		}

		AVCodecParameters *pVideoCodecParam = video_st->codecpar;

		/*	*/
		AVCodec *pVideoCodec = avcodec_find_decoder(pVideoCodecParam->codec_id);
		if (pVideoCodec == nullptr)
			throw cxxexcept::RuntimeException("failed to find decoder");
		this->pVideoCtx = avcodec_alloc_context3(pVideoCodec);
		if (this->pVideoCtx == nullptr)
			throw cxxexcept::RuntimeException("Failed to allocate video decoder context");

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
			throw cxxexcept::RuntimeException("Failed to set codec parameters : {}", buf);
		}
		// av_find_best_pix_fmt_of_2
		// avcodec_default_get_format()

		if ((result = avcodec_open2(this->pVideoCtx, pVideoCodec, nullptr)) != 0) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
		}

		video_width = this->pVideoCtx->width;
		video_height = this->pVideoCtx->height;

		this->frame = av_frame_alloc();
		this->frameoutput = av_frame_alloc();

		if (this->frame == nullptr || this->frameoutput == nullptr)
			throw cxxexcept::RuntimeException("Failed to allocate frame");

		size_t m_bufferSize =
			av_image_get_buffer_size(AV_PIX_FMT_RGBA, this->pVideoCtx->width, this->pVideoCtx->height, 4);
		av_image_alloc(this->frameoutput->data, this->frameoutput->linesize, this->pVideoCtx->width,
					   this->pVideoCtx->height, AV_PIX_FMT_RGBA, 4);

		// AVPacket *pPacket = av_packet_alloc();
		this->sws_ctx = sws_getContext(this->pVideoCtx->width, this->pVideoCtx->height, this->pVideoCtx->pix_fmt,
									   this->pVideoCtx->width, this->pVideoCtx->height, AV_PIX_FMT_RGBA, SWS_BICUBIC,
									   nullptr, nullptr, nullptr);

		this->frame_timer = av_gettime() / 1000000.0;
	}
	fragcore::AudioClip *clip;
	fragcore::AudioListener *listener;
	fragcore::AudioSource *audioSource;

	virtual void Initialize() override {

		this->audioInterface = std::make_shared<fragcore::OpenALAudioInterface>(nullptr);

		fragcore::AudioListenerDesc list_desc = {.position = fragcore::Vector3(0, 0, 0),
												 .rotation = fragcore::Quaternion::Identity()};
		list_desc.position = fragcore::Vector3::Zero();
		listener = audioInterface->createAudioListener(&list_desc);
		listener->setVolume(1.0f);
		fragcore::AudioSourceDesc source_desc = {};
		source_desc.position = fragcore::Vector3::Zero();
		audioSource = audioInterface->createAudioSource(&source_desc);

		fragcore::AudioClipDesc clip_desc = {};
		clip_desc.decoder = nullptr;
		clip_desc.samples = audio_sample_rate;
		clip_desc.sampleRate = audio_bit_rate;
		clip_desc.format = fragcore::AudioFormat::eStero;
		clip_desc.datamode = fragcore::AudioDataMode::Streaming;

		// this->clip = audioInterface->createAudioClip(&clip_desc);
		// this->audioSource->setClip(this->clip);

		loadVideo(path.c_str());

		this->videoStagingSize = (video_width * video_height * 4);

		VKHelper::createBuffer(this->getDevice(), this->videoStagingSize * this->nrVideoFrames,
							   this->getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(),
							   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							   videoStagingFrames, videoStagingFrameMemory);

		/*	*/
		for (size_t i = 0; i < this->videoFrames.size(); i++) {

			VKS_VALIDATE(vkMapMemory(getDevice(), this->videoStagingFrameMemory,
									 (i % this->nrVideoFrames) * this->videoStagingSize, this->videoStagingSize, 0,
									 &mapMemory[i]));

			VKHelper::createImage(
				getDevice(), video_width, video_height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				getVKDevice()->getPhysicalDevice(0)->getMemoryProperties(), videoFrames[i], videoFrameMemory[i]);
		}
		onResize(width(), height());

		// this->audioSource->play();
	}

	virtual void onResize(int width, int height) override {

		nthVideoFrame = 0;

		for (size_t i = 0; i < this->getNrCommandBuffers(); i++) {
			VkCommandBuffer cmd = this->getCommandBuffers(i);

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;

			VKS_VALIDATE(vkBeginCommandBuffer(cmd, &beginInfo));
			VkBufferImageCopy imageCopyRegion{};
			imageCopyRegion.bufferOffset = this->videoStagingSize * (i % nrVideoFrames);
			imageCopyRegion.bufferRowLength = 0;
			imageCopyRegion.bufferImageHeight = 0;
			imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.imageSubresource.layerCount = 1;
			imageCopyRegion.imageExtent.width = video_width;
			imageCopyRegion.imageExtent.height = video_height;
			imageCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(cmd, videoStagingFrames, videoFrames[nthVideoFrame],
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

			VKHelper::transitionImageLayout(cmd, getSwapChainImages()[i], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VKS_VALIDATE(vkEndCommandBuffer(cmd));
			nthVideoFrame = (nthVideoFrame + 1) % nrVideoFrames;
		}
		nthVideoFrame = 0;
	}

	virtual void draw() override {
		AVPacket *packet = av_packet_alloc();
		if (!packet) {
			throw cxxexcept::RuntimeException("failed to allocated memory for AVPacket");
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
					throw cxxexcept::RuntimeException("Failed to send packet for decoding picture frame : {}", buf);
				}

				while (result >= 0) {
					result = avcodec_receive_frame(this->pVideoCtx, this->frame);
					if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
						break;
					}
					if (result < 0) {
						char buf[AV_ERROR_MAX_STRING_SIZE];
						av_strerror(result, buf, sizeof(buf));
						throw cxxexcept::RuntimeException(" : {}", buf);
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
						memcpy(mapMemory[nthVideoFrame], this->frameoutput->data[0], this->videoStagingSize);

						VkMappedMemoryRange stagingRange{};
						stagingRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
						stagingRange.memory = videoStagingFrameMemory;
						stagingRange.offset = (this->nthVideoFrame % this->nrVideoFrames) * this->videoStagingSize;
						stagingRange.size = this->videoStagingSize;
						VKS_VALIDATE(vkFlushMappedMemoryRanges(getDevice(), 1, &stagingRange));

						VKS_VALIDATE(vkDeviceWaitIdle(getDevice()));
						this->nthVideoFrame = (this->nthVideoFrame + 1) % this->nrVideoFrames;
					}
				}
			} else if (packet->stream_index == this->audioStream) {
				result = avcodec_send_packet(this->pAudioCtx, packet);
				if (result < 0) {
					char buf[AV_ERROR_MAX_STRING_SIZE];
					av_strerror(result, buf, sizeof(buf));
					throw cxxexcept::RuntimeException("Failed to send packet for decoding audio frame : {}", buf);
				}

				while (result >= 0) {
					result = avcodec_receive_frame(this->pAudioCtx, this->frame);
					if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
						break;
					if (result < 0) {
						char buf[AV_ERROR_MAX_STRING_SIZE];
						av_strerror(result, buf, sizeof(buf));
						throw cxxexcept::RuntimeException(" : {}", buf);
					}
					int data_size = av_get_bytes_per_sample(pAudioCtx->sample_fmt);

					av_get_channel_layout_nb_channels(this->frame->channel_layout);
					this->frame->format != AV_SAMPLE_FMT_S16P;
					this->frame->channel_layout;

					/*	Assign new audio data.	*/
					for (int i = 0; i < frame->nb_samples; i++)
						for (int ch = 0; ch < pAudioCtx->channels; ch++)
							continue;
					// clip->setData(this->frame->data[0], data_size, 0);
				}
				// this->audioSource->play();
			}
		}
		av_packet_unref(packet);
		av_packet_free(&packet);
	}

	virtual void update() {}
};

int main(int argc, const char **argv) {

	std::unordered_map<const char *, bool> required_instance_extensions = {};
	std::unordered_map<const char *, bool> required_device_extensions = {};
	// TODO add custom argument options for adding path of the texture and what type.

	try {
		VKSampleWindow<AVVideoPlayback> skybox(argc, argv, required_device_extensions, {},
											   required_instance_extensions);
		skybox.run();
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}