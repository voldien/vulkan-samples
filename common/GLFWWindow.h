
#ifndef _VKCOMMON_GLFW_WINDOW_H_
#define _VKCOMMON_GLFW_WINDOW_H_ 1
#include <GLFW/glfw3.h>
#include <string>
#include"IWindow.h"

/**
 * @brief
 *
 */
class GLFWWindow : public IWindow {
  public:
	virtual void show(void) noexcept;

	virtual void hide(void) noexcept;

	virtual void close(void) noexcept;

	virtual void focus(void);

	virtual void restore(void);

	virtual void maximize(void);

	virtual void minimize(void);

	virtual void setTitle(const std::string &title);

	virtual std::string getTitle(void);

	virtual int x(void) const noexcept;
	virtual int y(void) const noexcept;

	virtual int width(void) const noexcept;
	virtual int height(void) const noexcept;

	virtual void getPosition(int *x, int *y) const;

	virtual void setPosition(int x, int y) noexcept;

	virtual void setSize(int width, int height) noexcept;

	virtual void getSize(int *width, int *height) const;

	virtual void resizable(bool resizable) noexcept;

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen(void) const;

	virtual void setBordered(bool borded);

	virtual void setMinimumSize(int width, int height);
	virtual void getMinimumSize(int *width, int *height);
	virtual void setMaximumSize(int width, int height);
	virtual void getMaximumSize(int *width, int *height);

  protected:
	GLFWwindow *window;
};

#endif
