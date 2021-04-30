#pragma once


#include "glm/glm.hpp"


class Camera
{

public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);
	Camera(glm::vec3 position, float yaw, float pitch, float fovDegrees, float aspectRatio, float moveSpeed, float turnSpeed);
	~Camera();

	virtual void OnUpdate(float deltaTime);
	// virtual void OnEvent(Event& e) override;

	virtual void SetViewportSize(float width, float height);
	virtual glm::mat4& GetViewMatrix();

	inline void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }
	inline float& GetAspectRatio() { return m_AspectRatio; }

	inline void SetPosition(glm::vec3 position) { m_Position = position; };
	inline const glm::vec3& GetPosition() const { return m_Position; }

	inline void SetFront(glm::vec3 front) { m_Front = front; };
	inline glm::vec3 GetFront() const { return m_Front; }

	virtual void SetPitch(float pitch);
	inline float GetPitch() const { return m_Pitch; }

	inline void SetYaw(float yaw) { m_Yaw = yaw; };
	inline float GetYaw() const { return m_Yaw; }

	inline glm::vec3 GetUp() const { return m_Up; }
	inline glm::vec3 GetRight() const { return m_Right; }

private:
	void UpdateProjection();
	void UpdateView();

private:
	glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 m_ViewMatrix;

	glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	float m_Pitch = 0.0f;
	float m_Yaw = 0.0f;
	float m_PerspectiveFOV = glm::radians(45.0f);
	float m_AspectRatio = 1.778f;
	glm::vec3 m_Front;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;
	float m_ViewportWidth = 1280.0f;
	float m_ViewportHeight = 720.0f;
	float m_PerspectiveNear = 0.01f;
	float m_PerspectiveFar = 1000.0f;

};
