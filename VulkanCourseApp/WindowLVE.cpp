#include "WindowLVE.h"

#include <stdexcept>


WindowLVE::WindowLVE(uint32_t width, uint32_t height, std::string title)
	: m_Width(width), m_Height(height), m_Title(title)
{
	initWindow();
}

WindowLVE::~WindowLVE()
{
	// Destroy GLFW window and stop GLFW
	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();
}

void WindowLVE::framebufferResizeCallback(GLFWwindow* windowHandle, int width, int height)
{
	auto window = reinterpret_cast<WindowLVE*>(glfwGetWindowUserPointer(windowHandle));
	window->m_FramebufferResized = true;
	window->m_Width = width;
	window->m_Height = height;
}

void WindowLVE::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_WindowHandle = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_WindowHandle, this);
	// glfwSetFramebufferSizeCallback(m_WindowHandle, framebufferResizeCallback);
}

void WindowLVE::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, m_WindowHandle, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a window surface.");
	}
}
