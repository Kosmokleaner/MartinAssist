#include "WindowPersist.h"
#include <ImGui/GLFW/glfw3.h> // Will drag system OpenGL headers

// see https://www.glfw.org/docs/3.3/window_guide.html

void WindowPersist::LoadState(GLFWwindow* window) const
{
	if(!window)
		return;

	if(fullscreen)
		glfwMaximizeWindow(window);
	else
	{
		glfwSetWindowPos(window, rect[0], rect[1]);
		int width = rect[2] - rect[0];
		int height = rect[3] - rect[1];
		glfwSetWindowSize(window, width, height);
	}
}

void WindowPersist::SaveState(GLFWwindow* window)
{
	if (!window)
		return;

	fullscreen = glfwGetWindowMonitor(window) != nullptr;
	glfwGetWindowPos(window, &rect[0], &rect[1]);
	int width = 0, height = 0;
	glfwGetWindowSize(window, &width, &height);
	rect[2] = rect[0] + width;
	rect[3] = rect[1] + height;
}
