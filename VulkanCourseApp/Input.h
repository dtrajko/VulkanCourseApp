#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <utility>


class Input
{
public:
	static bool IsKeyPressed(KeyCode key, GLFWwindow* windowHandle);               // KeyCode key
	static bool IsMouseButtonPressed(MouseCode button, GLFWwindow* windowHandle);  // MouseCode button
	static bool IsMouseButtonReleased(MouseCode button, GLFWwindow* windowHandle); // MouseCode button
	static std::pair<float, float> GetMousePosition(GLFWwindow* windowHandle);
	static float GetMouseX(GLFWwindow* windowHandle);
	static float GetMouseY(GLFWwindow* windowHandle);

};
