#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

const float YAW = 90.0f;
const float PITCH = 0.0f;
const float FOV = 45.0f;
const float MOVESPEED = 15.0f;
const float SENSITIVITY = 0.05f;
const float FOVSPEED = 3.0f;

class Camera {
public:
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f))
		: Pitch(PITCH), Yaw(YAW), Fov(FOV), MoveSpeed(MOVESPEED), Sensitivity(SENSITIVITY),FovSpeed(FOVSPEED)
	{
		Position = position;
		Front = front;
		WorldUp = worldUp;
		
		CoordUpdate();
	}

	void PositionMove(Camera_Movement direction, double deltaTime);
	void ViewMove(double xoffset, double yoffset, bool pitchConstrain = true, float pitchUp = 89.0f, float pitchDown = -89.0f);
	void FovMove(double offset);
	glm::mat4 ViewMatrix() const;

public:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float Fov;
	float Yaw;
	float Pitch;

	float MoveSpeed;
	float FovSpeed;
	float Sensitivity;

private:
	void CoordUpdate();
};

void Camera::PositionMove(Camera_Movement direction, double deltaTime)
{
	float moveLength = MoveSpeed * (float)deltaTime;
	if (direction==FORWARD)
		Position+= moveLength * Front;
	if (direction == BACKWARD)
		Position -= moveLength * Front;
	if (direction == LEFT)
		Position -= moveLength * Right;
	if (direction == RIGHT)
		Position += moveLength * Right;
	if (direction == UP)
		Position += moveLength * WorldUp;
	if (direction == DOWN)
		Position -= moveLength * WorldUp;
}

void Camera::ViewMove(double xoffset, double yoffset, bool pitchConstrain, float pitchUp, float pitchDown)
{
	xoffset *= Sensitivity;
	yoffset *= Sensitivity;

	Yaw += (float)xoffset;
	Pitch += (float)yoffset;

	if (pitchConstrain)
	{
		if (Pitch > pitchUp)
			Pitch = pitchUp;
		else if (Pitch < pitchDown)
			Pitch = pitchDown;
	}

	glm::vec3 front;
	front.x = cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
	front.y = sin(glm::radians(Pitch));
	front.z = cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
	Front = front;
	CoordUpdate();
}

void Camera::CoordUpdate()
{
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::FovMove(double offset)
{
	if (Fov >= 1.0f && Fov <= 90.0f)
		Fov -= (float)offset * FovSpeed;
	if (Fov < 1.0f)
		Fov = 1.0f;
	if (Fov > 90.0f)
		Fov = 90.0f;
}

glm::mat4 Camera::ViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, WorldUp);
}

#endif
