#include "Input.h"

#include "WindowLVE.h"


bool Input::IsKeyPressed(KeyCode key, GLFWwindow* windowHandle)
{
	auto state = glfwGetKey(windowHandle, static_cast<int32_t>(key));
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(MouseCode button, GLFWwindow* windowHandle)
{
	auto state = glfwGetMouseButton(windowHandle, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

bool Input::IsMouseButtonReleased(MouseCode button, GLFWwindow* windowHandle)
{
	auto state = glfwGetMouseButton(windowHandle, static_cast<int32_t>(button));
	return state == GLFW_RELEASE;
}

std::pair<float, float> Input::GetMousePosition(GLFWwindow* windowHandle)
{
	double xpos, ypos;
	glfwGetCursorPos(windowHandle, &xpos, &ypos);
	return { (float)xpos, (float)ypos };
}

float Input::GetMouseX(GLFWwindow* windowHandle)
{
	std::pair<float, float> coords = GetMousePosition(windowHandle);
	return coords.first;
}

float Input::GetMouseY(GLFWwindow* windowHandle)
{
	std::pair<float, float> coords = GetMousePosition(windowHandle);
	return coords.second;
}
