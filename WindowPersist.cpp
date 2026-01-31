#include "WindowPersist.h"
#include <ImGui/GLFW/glfw3.h> // Will drag system OpenGL headers

// see https://www.glfw.org/docs/3.3/window_guide.html

void WindowPersist::ApplyState(GLFWwindow* window) const
{
	if(!window)
		return;

	if(fullscreen || maximized)
		glfwMaximizeWindow(window);
	else
	{
//		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
//		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		// without this call the glfwSetWindowPos() call removes the window title bar
//		glfwShowWindow(window);
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

	maximized = fullscreen = glfwGetWindowMonitor(window) != nullptr;
	glfwGetWindowPos(window, &rect[0], &rect[1]);
	int width = 0, height = 0;
	glfwGetWindowSize(window, &width, &height);
	rect[2] = rect[0] + width;
	rect[3] = rect[1] + height;
}
