#ifndef LIGHT_H
#define LIGHT_H

#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "object.h"

enum LightType
{
	Dir=0,Dot,Spot
};

struct LightInfo
{
	glm::vec3 Direction;
	glm::vec3 Position;
	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;

	glm::vec3 Intensity;

	float Constant ;
	float Linear;
	float Quadratic;

	float CutOff;
	float OuterCutOff;
};

class Light
{
public:
	virtual void GetLightInfo(LightInfo& info) const=0;
	virtual void GetSphere(unsigned int& VAO, unsigned int& count, glm::mat4& model) const {};
	virtual LightType GetType()const = 0;
};

class DotLight:public Light
{
public:
	DotLight() = default;
	DotLight(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
		:Position(position), Ambient(ambient), Diffuse(diffuse), Specular(specular) {
		SphereVolume = Sphere(nullptr, nullptr, 0);
		SphereVolume.SetModel(Position, GetSphereRadius());
	}

	DotLight(glm::vec3 position, glm::vec3 intensity)
		:Position(position), Intensity(intensity) {};

	void GetLightInfo(LightInfo& info) const override
	{
		info.Position = Position;
		info.Ambient = Ambient;
		info.Diffuse = Diffuse;
		info.Specular = Specular;

		info.Constant = Constant;
		info.Linear = Linear;
		info.Quadratic = Quadratic;

		info.Intensity = Intensity;
	}

	void GetSphere(unsigned int& VAO, unsigned int& count, glm::mat4& model) const override
	{
		VAO = SphereVolume.SphereVolumeVAO;
		count = SphereVolume.Count;
		model = SphereVolume.model;
	}

	LightType GetType()const override
	{
		return Dot;
	}

	glm::vec3 Position;
	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;
	glm::vec3 Intensity;
private:
	float Constant = 1.0f;
	float Linear = 0.022f;
	float Quadratic = 0.019f;
	
	Sphere SphereVolume = Sphere(nullptr, nullptr, 0);
	float GetSphereRadius() {
		float lightMax = std::fmaxf(std::fmaxf(Diffuse.r, Diffuse.g), Diffuse.b);
		float radius =
			(-Linear + std::sqrtf(Linear * Linear - 4 * Quadratic * (Constant - (256.0 / 5.0) * lightMax)))
			/ (2 * Quadratic);
		return radius;
	}
};

class DirLight :public Light
{
public:
	DirLight() = default;
	DirLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
		:Direction(direction), Ambient(ambient), Diffuse(diffuse), Specular(specular) {
	};

	glm::vec3 Direction;
	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;

	virtual void GetLightInfo(LightInfo& info) const override {
		info.Direction = Direction;
		info.Ambient = Ambient;
		info.Diffuse = Diffuse;
		info.Specular = Specular;
	}
	LightType GetType()const override
	{
		return Dir;
	}
};

class SpotLight :public Light
{
public:
	SpotLight() = default;
	SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
		:Position(position), Direction(direction), Ambient(ambient), Diffuse(diffuse), Specular(specular) {
	};

	glm::vec3 Position;
	glm::vec3 Direction;
	glm::vec3 Ambient;
	glm::vec3 Diffuse;
	glm::vec3 Specular;

	float Constant = 1.0f;
	float Linear = 0.022f;
	float Quadratic = 0.019f;

	float CutOff = glm::cos(glm::radians(12.5f));
	float OuterCutOff = glm::cos(glm::radians(15.0f));

	void GetLightInfo(LightInfo& info) const override
	{
		info.Position = Position;
		info.Direction = Direction;
		info.Ambient = Ambient;
		info.Diffuse = Diffuse;
		info.Specular = Specular;

		info.Constant = Constant;
		info.Linear = Linear;
		info.Quadratic = Quadratic;

		info.CutOff = CutOff;
		info.OuterCutOff = OuterCutOff;
	}
	LightType GetType()const override
	{
		return Spot;
	}
};

#endif
