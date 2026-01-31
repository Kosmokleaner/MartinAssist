#pragma once

class WindowPersist
{
public:
	
	bool fullscreen = false;
	bool maximized = false;
	// left, top, right, bottom
	int rect[4] = { 100, 100, 100 + 1024, 100 + 768 };

//#ifdef _WIN32
//	void LoadState(HWND hWnd);
//	void SaveState(HWND hWnd);
//#else
	// this changes the window
	// @param window 0 is silently ignored
	void ApplyState(struct GLFWwindow* window) const;
	// this changes the object internals
	// @param window 0 is silently ignored
	void SaveState(struct GLFWwindow* window);

//#endif
};