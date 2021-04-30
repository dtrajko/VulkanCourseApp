#include "CameraController.h"

#include "WindowLVE.h"

#include <cstdio>


CameraController::CameraController(std::shared_ptr<Camera> camera, float aspectRatio, float moveSpeed, float turnSpeed)
{
	m_Camera = camera;
	m_Camera->SetAspectRatio(aspectRatio);
	m_MoveSpeed = moveSpeed;
	m_TurnSpeed = turnSpeed;
}

void CameraController::KeyControl(bool* keys, float deltaTime, GLFWwindow* windowHandle)
{
	// Don't move the camera when using Ctrl+S or Ctrl+D in Editor
	if (Input::IsKeyPressed(Key::LeftControl, windowHandle)) return;

	// Move the camera only the viewport accepts events, i.e. in focus or hovered (mouse over)
	// if (!ImGuiWrapper::CanViewportReceiveEvents()) return;

	m_SpeedBoostEnabled = false;

	float velocity = m_MoveSpeed * deltaTime;

	if (Input::IsKeyPressed(Key::LeftShift, windowHandle))
	{
		velocity *= m_SpeedBoost;
		m_SpeedBoostEnabled = true;
		// printf("Pressed key LEFT SHIFT\n");
	}
	if (Input::IsKeyPressed(Key::W, windowHandle) || Input::IsKeyPressed(Key::Up, windowHandle))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
		printf("Pressed key W%s\n", m_SpeedBoostEnabled ? " + LEFT SHIFT" : "");
		printf("Camera position [%.2f %.2f %.2f]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
	}
	if (Input::IsKeyPressed(Key::S, windowHandle) || Input::IsKeyPressed(Key::Down, windowHandle))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetFront() * velocity);
		printf("Pressed key S%s\n", m_SpeedBoostEnabled ? " + LEFT SHIFT" : "");
		printf("Camera position [%.2f %.2f %.2f]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
	}
	if (Input::IsKeyPressed(Key::A, windowHandle) || Input::IsKeyPressed(Key::Left, windowHandle))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetRight() * velocity);
		printf("Pressed key A%s\n", m_SpeedBoostEnabled ? " + LEFT SHIFT" : "");
		printf("Camera position [%.2f %.2f %.2f]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
	}
	if (Input::IsKeyPressed(Key::D, windowHandle) || Input::IsKeyPressed(Key::Right, windowHandle))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetRight() * velocity);
		printf("Pressed key D%s\n", m_SpeedBoostEnabled ? " + LEFT SHIFT" : "");
		printf("Camera position [%.2f %.2f %.2f]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
	}
	if (Input::IsKeyPressed(Key::Q, windowHandle))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() - m_Camera->GetUp() * velocity);
		printf("Pressed key Q%s\n", m_SpeedBoostEnabled ? " + LEFT SHIFT" : "");
		printf("Camera position [%.2f %.2f %.2f]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
	}
	if (Input::IsKeyPressed(Key::E, windowHandle) || Input::IsKeyPressed(Key::Space, windowHandle))
	{
		m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetUp() * velocity);
		printf("Pressed key E%s\n", m_SpeedBoostEnabled ? " + LEFT SHIFT" : "");
		printf("Camera position [%.2f %.2f %.2f]\n", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
	}

	if (Input::IsKeyPressed(Key::L, windowHandle))
	{
		// printf("CameraController GLFW_KEY_L {0}, keys[GLFW_KEY_L] {1}", GLFW_KEY_L, keys[GLFW_KEY_L]);
		// printf("CameraController::KeyControl Position [ {0}, {1}, {2} ]", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
		// printf("CameraController::KeyControl Front    [ {0}, {1}, {2} ]", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
	}
}

void CameraController::MouseControl(bool* buttons, float xChange, float yChange, GLFWwindow* windowHandle)
{
	if (Input::IsMouseButtonPressed(Mouse::ButtonRight, windowHandle))
	{
		m_Camera->SetYaw(m_Camera->GetYaw() + xChange * m_TurnSpeed);
		m_Camera->SetPitch(m_Camera->GetPitch() - yChange * m_TurnSpeed);
	}
}

void CameraController::MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset, GLFWwindow* windowHandle)
{
	if (abs(yOffset) < 0.1f || abs(yOffset) > 10.0f)
		return;

	float velocity = m_MoveSpeed * yOffset;

	if (Input::IsKeyPressed(Key::LeftShift, windowHandle))
	{
		velocity *= m_SpeedBoost;
	}

	m_Camera->SetPosition(m_Camera->GetPosition() + m_Camera->GetFront() * velocity);
}

void CameraController::Update(float deltaTime, GLFWwindow* windowHandle)
{
	KeyControl(WindowLVE::getKeys(), deltaTime, windowHandle);

	MouseControl(
		WindowLVE::getMouseButtons(),
		WindowLVE::getChangeX(),
		WindowLVE::getChangeY(), windowHandle);

	MouseScrollControl(
		WindowLVE::getKeys(), deltaTime,
		WindowLVE::getMouseScrollOffsetX(),
		WindowLVE::getMouseScrollOffsetY(), windowHandle);

	CalculateFront();
}

//	void CameraController::OnEvent(Event& e)
//	{
//	}

void CameraController::InvertPitch()
{
	float pitch = m_Camera->GetPitch();
	m_Camera->SetPitch(-pitch);
}

void CameraController::OnResize(uint32_t width, uint32_t height)
{
	// TODO (void Hazel::OrthographicCameraController::OnResize(float width, float height))
	m_Camera->SetAspectRatio((float)width / (float)height);
}

void CameraController::CalculateFront()
{
	float pitch = m_Camera->GetPitch();
	float yaw = m_Camera->GetYaw();
	glm::vec3 front = glm::vec3(0.0f);
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = -sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
	m_Camera->SetFront(front);
}

CameraController::~CameraController()
{
}
