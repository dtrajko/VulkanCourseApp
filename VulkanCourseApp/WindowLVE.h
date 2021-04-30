#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>


class WindowLVE
{
public:
	WindowLVE() = default;
	WindowLVE(uint32_t width, uint32_t height, std::string title);
	~WindowLVE();

	WindowLVE(const WindowLVE&) = delete;

	bool shouldClose() { return glfwWindowShouldClose(m_WindowHandle); }
	VkExtent2D getExtent() { return { m_Width, m_Height }; }
	bool wasWindowResized() { return m_FramebufferResized; }
	void resetWindowResizedFlag() { m_FramebufferResized = false; }

	void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	GLFWwindow* getHandle() { return m_WindowHandle; }

	// from MoravaEngine/src/Platform/Windows/WindowsWindow.cpp
	static bool* getKeys() { return s_Keys; };
	static bool* getMouseButtons() { return s_Buttons; };

	static float getChangeX();
	static float getChangeY();

	static float getMouseScrollOffsetX();
	static float getMouseScrollOffsetY();

private:
	static void framebufferResizeCallback(GLFWwindow* windowHandle, int width, int height);
	void initWindow();

private:
	uint32_t m_Width;
	uint32_t m_Height;
	bool m_FramebufferResized = false;
	std::string m_Title;
	GLFWwindow* m_WindowHandle;

	static bool s_Keys[1024];
	static bool s_Buttons[32];

	static bool s_Keys_prev[1024];
	static bool s_Buttons_prev[32];

	static float s_ChangeX;
	static float s_ChangeY;
	static float s_CursorIgnoreLimit;

	static float s_MouseScrollOffsetX;
	static float s_MouseScrollOffsetY;

};
