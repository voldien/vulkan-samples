#ifndef _VKSCOMMON_IWINDOW_H_
#define _VKSCOMMON_IWINDOW_H_ 1

/**
 * @brief
 *
 */
class IWindow {
  public:
	virtual void show(void) = 0;

	virtual void hide(void) = 0;

	virtual void close(void) = 0;

	virtual void focus(void) = 0;

	virtual void restore(void) = 0;

	virtual void maximize(void) = 0;

	virtual void minimize(void) = 0;

	virtual void setTitle(const char *title) = 0;

	virtual const char *getTitle(void) const = 0;

	virtual void getPosition(int *x, int *y) const = 0;

	virtual void setPosition(int x, int y) = 0;

	virtual void setSize(int width, int height) = 0;

	virtual void getSize(int *width, int *height) const = 0;

	virtual int width(void) const noexcept;
	virtual int height(void) const noexcept;

	virtual float getGamma(void) const = 0;

	virtual void setGamma(float gamma) = 0;

	virtual void resizable(bool resizable) = 0;

	virtual void setFullScreen(bool fullscreen) = 0;

	virtual bool isFullScreen(void) const = 0;

	virtual void setBordered(bool borded) = 0;

	virtual void setMinimumSize(int width, int height) = 0;
	virtual void getMinimumSize(int *width, int *height) = 0;
	virtual void setMaximumSize(int width, int height) = 0;
	virtual void getMaximumSize(int *width, int *height) = 0;

	void setUserData(void *userData) noexcept { this->userData = userData; }

	virtual void *getUserData(void) const noexcept { return this->userData; }

	virtual intptr_t getNativePtr(void) const = 0; /*  Get native window reference object. */

  protected:
	void *userData = nullptr;
};
#endif