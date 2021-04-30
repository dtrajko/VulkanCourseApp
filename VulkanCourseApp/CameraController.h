#pragma once

#include "Camera.h"

#include "Input.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class CameraController
{
public:
	CameraController() = default;
	CameraController(std::shared_ptr<Camera> camera, float aspectRatio, float moveSpeed, float turnSpeed);
	virtual ~CameraController();

	virtual void Update(float deltaTime, GLFWwindow* windowHandle);
	// void OnEvent(Event& e); // TODO: integrate Events system from Hazel/MoravaEngine

	virtual void KeyControl(bool* keys, float deltaTime, GLFWwindow* windowHandle);
	virtual void MouseControl(bool* buttons, float xChange, float yChange, GLFWwindow* windowHandle);
	virtual void MouseScrollControl(bool* keys, float deltaTime, float xOffset, float yOffset, GLFWwindow* windowHandle);
	inline std::shared_ptr<Camera> GetCamera() { return m_Camera; };
	void InvertPitch();
	inline void SetUnlockRotation(bool unlockRotation) { m_UnlockRotation = unlockRotation; };

	void OnResize(uint32_t width, uint32_t height);

	inline void SetZoomLevel(float zoomLevel) { m_ZoomLevel = zoomLevel; }
	inline float GetZoomLevel() const { return m_ZoomLevel; }
	inline float GetAspectRatio() const { return m_Camera->GetAspectRatio(); }

protected:
	void CalculateFront();

protected:
	std::shared_ptr<Camera> m_Camera;

	float m_ZoomLevel = 1.0f;

	float m_MoveSpeed;
	float m_TurnSpeed;
	float m_SpeedBoost = 4.0f;
	float m_SpeedBoostEnabled = false;

	float m_MouseDeltaX = 0.0f;
	float m_MouseDeltaY = 0.0f;

	bool m_UnlockRotation; // Left SHIFT for mouse rotation

	// Hazel Camera::MouseRotate
	glm::vec2 m_InitialMousePosition;
	float m_YawSign = 0.0f;
	float m_RotationSpeed = 0.01f;

};
