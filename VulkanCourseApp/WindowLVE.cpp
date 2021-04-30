#include "WindowLVE.h"

#include <stdexcept>


bool WindowLVE::s_Keys[1024];
bool WindowLVE::s_Buttons[32];

bool WindowLVE::s_Keys_prev[1024];
bool WindowLVE::s_Buttons_prev[32];

float WindowLVE::s_ChangeX;
float WindowLVE::s_ChangeY;
float WindowLVE::s_CursorIgnoreLimit;

float WindowLVE::s_MouseScrollOffsetX;
float WindowLVE::s_MouseScrollOffsetY;


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

float WindowLVE::getChangeX()
{
	float theChange = 0.0f;
	if (std::abs(s_ChangeX) > s_CursorIgnoreLimit) {
		theChange = s_ChangeX;
	}
	return theChange;
}

float WindowLVE::getChangeY()
{
	float theChange = 0.0f;
	if (std::abs(s_ChangeY) > s_CursorIgnoreLimit) {
		theChange = s_ChangeY;
	}
	return theChange;
}

float WindowLVE::getMouseScrollOffsetX()
{
	float theOffset = s_MouseScrollOffsetX;
	s_MouseScrollOffsetX = 0.0f;
	return theOffset;
}

float WindowLVE::getMouseScrollOffsetY()
{
	float theOffset = s_MouseScrollOffsetY;
	s_MouseScrollOffsetY = 0.0f;
	return theOffset;
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

	// from MoravaEngine/src/Platform/Windows/WindowsWindow.cpp
	for (size_t i = 0; i < 1024; i++) {
		s_Keys[i] = false;
		s_Keys_prev[i] = false;
	}

	for (size_t i = 0; i < 32; i++) {
		s_Buttons[i] = false;
		s_Buttons_prev[i] = false;
	}
}

void WindowLVE::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, m_WindowHandle, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a window surface.");
	}
}
