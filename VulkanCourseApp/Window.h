#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>


class Window
{
public:
	Window() = default;
	Window(uint32_t width, uint32_t height, std::string title);
	~Window();

	Window(const Window&) = delete;

	bool shouldClose() { return glfwWindowShouldClose(m_WindowHandle); }
	VkExtent2D getExtent() { return { m_Width, m_Height }; }
	bool wasWindowResized() { return m_FramebufferResized; }
	void resetWindowResizedFlag() { m_FramebufferResized = false; }

	void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	GLFWwindow* getHandle() { return m_WindowHandle; }

private:
	static void framebufferResizeCallback(GLFWwindow* windowHandle, int width, int height);
	void initWindow();

private:
	uint32_t m_Width;
	uint32_t m_Height;
	bool m_FramebufferResized = false;
	std::string m_Title;
	GLFWwindow* m_WindowHandle;

};
