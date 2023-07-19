#ifndef SHADOW_H
#define SHADOW_H

#include <glad/glad.h> 
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include "object.h"
#include "light.h"
using std::shared_ptr;
using std::vector;

class Shadow
{
	virtual void DrawShadowMap(vector<Object> objects) = 0;
	virtual void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT) =0;
};

class DirShadow:public Shadow
{
public:
	DirShadow(DirLight _light, glm::vec3 _position, float near=0.1f,float far=30.0f);
	void DrawShadowMap(vector<Object> objects)  override;
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH=800, unsigned int SCR_HEIGHT=600) override;
private:
	void GetFramebuffer();
	unsigned int depthMapFBO, depthMap;
	glm::vec3 position;
	float near_plane, far_plane;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Shader shadowmapShader = Shader("res/shader/shadowMap_Dir_Vertex.shader", "res/shader/shadowMap_Dir_Fragment.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_Dir_Vertex.shader", "res/shader/shadow_Dir_Fragment.shader");
	DirLight light;
	glm::mat4 lightSpaceMatrix;
};

DirShadow::DirShadow(DirLight _light, glm::vec3 _position, float near, float far)
	:position(_position),near_plane(near),far_plane(far)
{
	GetFramebuffer();
	light = DirLight(_light);

	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(position, position + light.Direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;
}

void DirShadow::GetFramebuffer()
{
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0,1.0,1.0,1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirShadow::DrawShadowMap(vector<Object> objects) 
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);

	shadowmapShader.use();

	shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	
	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
}

void DirShadow::DrawObjects(vector<Object>objects,unsigned int FBO,glm::vec3 viewPos,glm::mat4 view,glm::mat4 projection,unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	shadowObjectShader.use();

	shadowObjectShader.setVec3("dirlight.ambient", light.Ambient);
	shadowObjectShader.setVec3("dirlight.diffuse", light.Diffuse);
	shadowObjectShader.setVec3("dirlight.specular", light.Specular);
	shadowObjectShader.setVec3("dirlight.direction", light.Direction);

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	shadowObjectShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("shadowMap", 2);

		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

class DotShadow :public Shadow
{
public:
	DotShadow() = default;
	DotShadow(DotLight _light, float _near = 0.1f, float _far = 200.0f);

	void DrawShadowMap(vector<Object> objects)  override;
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;

private:
	void GetFramebuffer();
	unsigned int depthCubeMapFBO, depthCubeMap;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	float near, far;
	vector<glm::mat4> shadowTransforms;
	DotLight light;

	Shader shadowmapShader = Shader("res/shader/shadowMap_Dot_Vertex.shader", "res/shader/shadowMap_Dot_Geometry.shader", "res/shader/shadowMap_Dot_Fragment.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_Dot_Vertex.shader", "res/shader/shadow_Dot_Fragment.shader");
};

DotShadow::DotShadow(DotLight _light, float _near, float _far)
	: near(_near),far(_far)
{
	GetFramebuffer();

	light = DotLight(_light);

	float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
}

void DotShadow::GetFramebuffer()
{
	glGenFramebuffers(1, &depthCubeMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthCubeMapFBO);

	glGenTextures(1, &depthCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
	for (unsigned int i = 0; i != 6; i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DotShadow::DrawShadowMap(vector<Object> objects)
{
	glBindFramebuffer(GL_FRAMEBUFFER, depthCubeMapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	shadowmapShader.use();

	shadowmapShader.setFloat("far_plane", far);
	shadowmapShader.setVec3("lightPos", light.Position);

	for (unsigned int i = 0; i != 6; i++)
	{
		shadowmapShader.setMat4(("shadowMatrixs[" + std::to_string(i) + "]").c_str(), glm::value_ptr(shadowTransforms[i]));
	}

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glBindVertexArray(objects[i].VAO);

		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

void DotShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	shadowObjectShader.use();

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("pointlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("pointlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("pointlight.specular", info.Specular);
	shadowObjectShader.setVec3("pointlight.position", info.Position);
	shadowObjectShader.setFloat("pointlight.constant", info.Constant);
	shadowObjectShader.setFloat("pointlight.linear", info.Linear);
	shadowObjectShader.setFloat("pointlight.quadratic", info.Quadratic);

	shadowObjectShader.setFloat("far_plane", far);

	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("shadowMap", 2);

		glBindVertexArray(objects[i].VAO);
		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

class SpotShadow :public Shadow
{
public:
	SpotShadow() = default;
	SpotShadow(SpotLight _light, float _near = 0.1f, float _far = 200.0f);

	void DrawShadowMap(vector<Object> objects)  override;
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
	void SetLight(glm::vec3 _position, glm::vec3 _direction)
	{
		light.Position = _position;
		light.Direction = _direction;
	}
private:
	void GetFramebuffer();
	unsigned int depthCubeMapFBO, depthCubeMap;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	float near, far;
	vector<glm::mat4> shadowTransforms;
	SpotLight light;

	Shader shadowmapShader = Shader("res/shader/shadowMap_Dot_Vertex.shader", "res/shader/shadowMap_Dot_Geometry.shader", "res/shader/shadowMap_Dot_Fragment.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_Spot_Vertex.shader", "res/shader/shadow_Spot_Fragment.shader");
};

SpotShadow::SpotShadow(SpotLight _light, float _near, float _far)
	: near(_near), far(_far)
{
	GetFramebuffer();

	light = SpotLight(_light);

	float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(light.Position, light.Position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
}

void SpotShadow::GetFramebuffer()
{
	glGenFramebuffers(1, &depthCubeMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthCubeMapFBO);

	glGenTextures(1, &depthCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
	for (unsigned int i = 0; i != 6; i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SpotShadow::DrawShadowMap(vector<Object> objects)
{
	glBindFramebuffer(GL_FRAMEBUFFER, depthCubeMapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	shadowmapShader.use();

	shadowmapShader.setFloat("far_plane", far);
	shadowmapShader.setVec3("lightPos", light.Position);

	for (unsigned int i = 0; i != 6; i++)
	{
		shadowmapShader.setMat4(("shadowMatrixs[" + std::to_string(i) + "]").c_str(), glm::value_ptr(shadowTransforms[i]));
	}

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glBindVertexArray(objects[i].VAO);

		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

void SpotShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	shadowObjectShader.use();

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("spotlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("spotlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("spotlight.specular", info.Specular);
	shadowObjectShader.setVec3("spotlight.position", info.Position);
	shadowObjectShader.setFloat("spotlight.constant", info.Constant);
	shadowObjectShader.setFloat("spotlight.linear", info.Linear);
	shadowObjectShader.setFloat("spotlight.quadratic", info.Quadratic);
	shadowObjectShader.setVec3("spotlight.direction", info.Direction);
	shadowObjectShader.setFloat("spotlight.cutOff", info.CutOff);
	shadowObjectShader.setFloat("spotlight.outerCutOff", info.OuterCutOff);

	shadowObjectShader.setFloat("far_plane", far);

	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("shadowMap", 2);

		glBindVertexArray(objects[i].VAO);
		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}


class PCSSShadow :public Shadow
{
public:
	PCSSShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, float near = 0.1f, float far = 100.0f);
	void DrawShadowMap(vector<Object> objects)  override;
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
	unsigned int depthMapFBO, depthMap;
private:
	void GetFramebuffer();
	
	glm::vec3 direction;
	float near_plane, far_plane;
	float lightWidth, orthoWidth=100.0f;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Shader shadowmapShader = Shader("res/shader/shadowMap_Dir_Vertex.shader", "res/shader/shadowMap_Dir_Fragment.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_PCSS_Vertex.shader", "res/shader/shadow_PCSS_Fragment.shader");
	DotLight light;
	glm::mat4 lightSpaceMatrix;
};

PCSSShadow::PCSSShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, float near, float far)
	:direction(_direction), lightWidth(_lightWidth), near_plane(near), far_plane(far)
{
	GetFramebuffer();
	light = DotLight(_light);
	
	glm::mat4 lightProjection = glm::ortho(-orthoWidth/2.0f, orthoWidth / 2.0f, -orthoWidth / 2.0f, orthoWidth / 2.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(light.Position, light.Position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;
}

void PCSSShadow::GetFramebuffer()
{
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0,1.0,1.0,1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PCSSShadow::DrawShadowMap(vector<Object> objects)
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);

	shadowmapShader.use();

	shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
}

void PCSSShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	shadowObjectShader.use();

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("pointlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("pointlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("pointlight.specular", info.Specular);
	shadowObjectShader.setVec3("pointlight.position", info.Position);
	shadowObjectShader.setFloat("pointlight.constant", info.Constant);
	shadowObjectShader.setFloat("pointlight.linear", info.Linear);
	shadowObjectShader.setFloat("pointlight.quadratic", info.Quadratic);

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	shadowObjectShader.setFloat("far_plane", far_plane);
	shadowObjectShader.setFloat("near_plane", near_plane);
	shadowObjectShader.setFloat("lightWidth", lightWidth);
	shadowObjectShader.setFloat("orthoWidth", orthoWidth);

	shadowObjectShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("shadowMap", 2);

		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

class VSSMShadow :public Shadow
{
public:
	VSSMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, float near = 0.1f, float far = 100.0f);
	void DrawMaps(vector<Object> objects, unsigned int r=2);
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
	unsigned int depthMapFBO, depthMap, depth2MapFBO, depth2Map, SATMapFBO[2], SATMap[2], SAT;
private:
	void DrawShadowMap(vector<Object> objects)  override;
	void DrawShadow2Map();
	void DrawSATs(unsigned int r);
	unsigned int GetSAT();

	void GetFramebuffer();
	void GetVertexArray();
	glm::vec3 direction;
	float near_plane, far_plane;
	float lightWidth, orthoWidth = 100.0f;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Shader shadowmapShader = Shader("res/shader/shadowMap_Dir_Vertex.shader", "res/shader/shadowMap_Dir_Fragment.shader");
	Shader shadowmap2Shader = Shader("res/shader/shadowMap_Square_Vertex.shader", "res/shader/shadowMap_Square_Fragment.shader");
	Shader satHorizontalShader = Shader("res/shader/satVertex.shader", "res/shader/satFragment_Horizontal.shader");
	Shader satVerticalShader = Shader("res/shader/satVertex.shader", "res/shader/satFragment_Vertical.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_VSSM_Vertex.shader", "res/shader/shadow_VSSM_Fragment.shader");
	DotLight light;
	glm::mat4 lightSpaceMatrix;

	unsigned int frameVAO, frameVBO;
};

VSSMShadow::VSSMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, float near, float far)
	:direction(_direction), lightWidth(_lightWidth), near_plane(near), far_plane(far)
{
	GetFramebuffer();
	GetVertexArray();
	light = DotLight(_light);

	glm::mat4 lightProjection = glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoWidth / 2.0f, orthoWidth / 2.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(light.Position, light.Position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;
}

void VSSMShadow::GetFramebuffer()
{
	//shadow map
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 0.0,0.0,0.0,0.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//shadow square map
	glGenFramebuffers(1, &depth2MapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depth2MapFBO);

	glGenTextures(1, &depth2Map);
	glBindTexture(GL_TEXTURE_2D, depth2Map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depth2Map, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//sat map
	glGenFramebuffers(2, SATMapFBO);
	glGenTextures(2, SATMap);

	for (unsigned int i = 0; i != 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, SATMapFBO[i]);
		glBindTexture(GL_TEXTURE_2D, SATMap[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		GLfloat borderColor[] = { 0.0,0.0,0.0,0.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SATMap[i], 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VSSMShadow::DrawShadowMap(vector<Object> objects)
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glFrontFace(GL_CCW);

	shadowmapShader.use();

	shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	//glCullFace(GL_BACK);
	//glDisable(GL_CULL_FACE);
}

void VSSMShadow::DrawShadow2Map()
{
	glBindFramebuffer(GL_FRAMEBUFFER, depth2MapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	shadowmap2Shader.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	shadowmap2Shader.setInt("depthMap", 0);

	glBindVertexArray(frameVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VSSMShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	
	shadowObjectShader.use();

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("pointlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("pointlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("pointlight.specular", info.Specular);
	shadowObjectShader.setVec3("pointlight.position", info.Position);
	shadowObjectShader.setFloat("pointlight.constant", info.Constant);
	shadowObjectShader.setFloat("pointlight.linear", info.Linear);
	shadowObjectShader.setFloat("pointlight.quadratic", info.Quadratic);

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	shadowObjectShader.setFloat("far_plane", far_plane);
	shadowObjectShader.setFloat("near_plane", near_plane);
	shadowObjectShader.setFloat("lightWidth", lightWidth);
	shadowObjectShader.setFloat("orthoWidth", orthoWidth);

	shadowObjectShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, GetSAT());

		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("satMap", 2);

		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

double logbase(double a, double base)
{
	return log(a) / log(base);
}

void VSSMShadow::GetVertexArray()
{
	float frameVertices[] =
	{
		-1.0, -1.0,  0.0, 0.0,
		 1.0, -1.0,  1.0, 0.0,
		 1.0,  1.0,  1.0, 1.0,
		 1.0,  1.0,  1.0, 1.0,
		-1.0,  1.0,  0.0, 1.0,
		-1.0, -1.0,  0.0, 0.0
	};

	glGenBuffers(1, &frameVBO);
	glGenVertexArrays(1, &frameVAO);

	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), frameVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void VSSMShadow::DrawSATs(unsigned int r)
{
	unsigned int n = (unsigned int)logbase(SHADOW_WIDTH, r);
	unsigned int m = (unsigned int)logbase(SHADOW_HEIGHT, r);

	bool flapFlag = false;
	
	for (unsigned int i = 0; i != n; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, SATMapFBO[flapFlag]);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		satHorizontalShader.use();

		satHorizontalShader.setInt("r", (int)r);
		satHorizontalShader.setInt("index", (int)i);

		//satHorizontalShader.setBool("first", i == 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, i == 0 ? depth2Map : SATMap[!flapFlag]);
		satHorizontalShader.setInt("satMap", 0);

		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		flapFlag = !flapFlag;
	}
	
	for (unsigned int i = 0; i != m; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, SATMapFBO[flapFlag]);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		satVerticalShader.use();

		satVerticalShader.setInt("r", (int)r);
		satVerticalShader.setInt("index", (int)i);

		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, i == 0 ? depthMap : SATMap[!flapFlag]);
		glBindTexture(GL_TEXTURE_2D, SATMap[!flapFlag]);
		satVerticalShader.setInt("satMap", 0);

		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		flapFlag = !flapFlag;
	}
	
	SAT = SATMap[!flapFlag];
}

inline unsigned int VSSMShadow::GetSAT()
{
	return SAT;
}

void VSSMShadow::DrawMaps(vector<Object> objects, unsigned int r)
{
	DrawShadowMap(objects);
	DrawShadow2Map();
	DrawSATs(r);
}

class ESMShadow :public Shadow
{
public:
	ESMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, float _c = 80.0f, int _kernelSize = 5, float near = 0.1f, float far = 100.0f);
	void DrawShadowMap(vector<Object> objects)  override;
	void DrawExpMap();
	void DrawGaussMaps();
	unsigned int GetGaussMap();
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
	unsigned int depthMapFBO, depthMap, expMapFBO, expMap, gaussFBOs[2], gaussMaps[2];
private:
	void GetFramebuffer();
	unsigned int frameVAO, frameVBO;
	void GetVertexArray();
	glm::vec3 direction;
	float near_plane, far_plane;
	float lightWidth, orthoWidth = 50.0f;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Shader shadowmapShader = Shader("res/shader/shadowMap_Dir_Vertex.shader", "res/shader/shadowMap_Dir_Fragment.shader");
	Shader shadowexpShader = Shader("res/shader/shadowMap_Exp_Vertex.shader", "res/shader/shadowMap_Exp_Fragment.shader");
	Shader shadowgaussShader = Shader("res/shader/shadowMap_Gauss_Vertex.shader", "res/shader/shadowMap_Gauss_Fragment.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_ESM_Vertex.shader", "res/shader/shadow_ESM_Fragment.shader");
	DotLight light;
	glm::mat4 lightSpaceMatrix;
	float c;
	int kernelSize;
};

ESMShadow::ESMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, float _c, int _kernelSize, float near, float far)
	:direction(_direction), lightWidth(_lightWidth), near_plane(near), far_plane(far), c(_c), kernelSize(_kernelSize)
{
	GetFramebuffer();
	light = DotLight(_light);

	glm::mat4 lightProjection = glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoWidth / 2.0f, orthoWidth / 2.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(light.Position, light.Position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;

	GetVertexArray();
}

void ESMShadow::GetFramebuffer()
{
	//shadow map
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0,1.0,1.0,1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	//exp map
	glGenFramebuffers(1, &expMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, expMapFBO);

	glGenTextures(1, &expMap);
	glBindTexture(GL_TEXTURE_2D, expMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, expMap, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//gauss
	glGenFramebuffers(2, gaussFBOs);
	glGenTextures(2, gaussMaps);

	for (unsigned int i = 0; i != 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gaussFBOs[i]);

		glBindTexture(GL_TEXTURE_2D, gaussMaps[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gaussMaps[i], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ESMShadow::DrawShadowMap(vector<Object> objects)
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glFrontFace(GL_CCW);

	shadowmapShader.use();

	shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	//glCullFace(GL_BACK);
	//glDisable(GL_CULL_FACE);
}

void ESMShadow::DrawExpMap()
{
	glBindFramebuffer(GL_FRAMEBUFFER, expMapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	shadowexpShader.use();

	shadowexpShader.setFloat("c", c);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	shadowexpShader.setInt("depthMap", 0);

	glBindVertexArray(frameVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ESMShadow::GetVertexArray()
{
	float frameVertices[] =
	{
		-1.0, -1.0,  0.0, 0.0,
		 1.0, -1.0,  1.0, 0.0,
		 1.0,  1.0,  1.0, 1.0,
		 1.0,  1.0,  1.0, 1.0,
		-1.0,  1.0,  0.0, 1.0,
		-1.0, -1.0,  0.0, 0.0
	};

	glGenBuffers(1, &frameVBO);
	glGenVertexArrays(1, &frameVAO);

	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), frameVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void ESMShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	glEnable(GL_DEPTH_TEST);

	shadowObjectShader.use();

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("pointlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("pointlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("pointlight.specular", info.Specular);
	shadowObjectShader.setVec3("pointlight.position", info.Position);
	shadowObjectShader.setFloat("pointlight.constant", info.Constant);
	shadowObjectShader.setFloat("pointlight.linear", info.Linear);
	shadowObjectShader.setFloat("pointlight.quadratic", info.Quadratic);

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	shadowObjectShader.setFloat("c", c);

	shadowObjectShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, GetGaussMap());
		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("expMap", 2);

		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

void ESMShadow::DrawGaussMaps()
{
	bool horizontal = true;
	bool first = true;

	unsigned int times = 2;
	for (unsigned int i = 0; i != times; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gaussFBOs[horizontal]);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

		shadowgaussShader.use();

		glActiveTexture(GL_TEXTURE0);
		unsigned int texture = first ? expMap : gaussMaps[!horizontal];
		glBindTexture(GL_TEXTURE_2D, texture);
		shadowgaussShader.setInt("texColorBuffer", 0);

		shadowgaussShader.setInt("kernelSize", kernelSize);

		shadowgaussShader.setBool("horizontal", horizontal);

		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		if (first)
			first = false;
		horizontal = !horizontal;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int ESMShadow::GetGaussMap()
{
	return gaussMaps[0];
}

class MSMShadow :public Shadow
{
public:
	MSMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, int _kernelSize = 5, float near = 0.1f, float far = 100.0f);
	void DrawShadowMap(vector<Object> objects)  override;
	void DrawMomentMaps();
	unsigned int GetGaussMap();
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
	unsigned int depthMapFBO, depthMap, msmMapFBO, msmMap, gaussFBOs[2], gaussMaps[2];
private:
	void GetFramebuffer();
	unsigned int frameVAO, frameVBO;
	void GetVertexArray();
	glm::vec3 direction;
	float near_plane, far_plane;
	float lightWidth, orthoWidth = 50.0f;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Shader shadowmapShader = Shader("res/shader/shadowMap_Dir_Vertex.shader", "res/shader/shadowMap_Dir_Fragment.shader");
	Shader shadowmomentShader = Shader("res/shader/shadowMap_MSMGauss_Vertex.shader", "res/shader/shadowMap_MSMGauss_Fragment.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_MSM_Vertex.shader", "res/shader/shadow_MSM_Fragment.shader");
	DotLight light;
	glm::mat4 lightSpaceMatrix;
	int kernelSize;
};

MSMShadow::MSMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, int _kernelSize, float near, float far)
	:direction(_direction), lightWidth(_lightWidth), near_plane(near), far_plane(far), kernelSize(_kernelSize)
{
	GetFramebuffer();
	light = DotLight(_light);

	glm::mat4 lightProjection = glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoWidth / 2.0f, orthoWidth / 2.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(light.Position, light.Position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;

	GetVertexArray();
}

void MSMShadow::GetFramebuffer()
{
	//shadow map
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0,1.0,1.0,1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	//moment map & gauss
	glGenFramebuffers(2, gaussFBOs);
	glGenTextures(2, gaussMaps);

	for (unsigned int i = 0; i != 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gaussFBOs[i]);

		glBindTexture(GL_TEXTURE_2D, gaussMaps[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gaussMaps[i], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MSMShadow::DrawShadowMap(vector<Object> objects)
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glFrontFace(GL_CCW);

	shadowmapShader.use();

	shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	//glCullFace(GL_BACK);
	//glDisable(GL_CULL_FACE);
}

void MSMShadow::GetVertexArray()
{
	float frameVertices[] =
	{
		-1.0, -1.0,  0.0, 0.0,
		 1.0, -1.0,  1.0, 0.0,
		 1.0,  1.0,  1.0, 1.0,
		 1.0,  1.0,  1.0, 1.0,
		-1.0,  1.0,  0.0, 1.0,
		-1.0, -1.0,  0.0, 0.0
	};

	glGenBuffers(1, &frameVBO);
	glGenVertexArrays(1, &frameVAO);

	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), frameVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void MSMShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	glEnable(GL_DEPTH_TEST);

	shadowObjectShader.use();

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("pointlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("pointlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("pointlight.specular", info.Specular);
	shadowObjectShader.setVec3("pointlight.position", info.Position);
	shadowObjectShader.setFloat("pointlight.constant", info.Constant);
	shadowObjectShader.setFloat("pointlight.linear", info.Linear);
	shadowObjectShader.setFloat("pointlight.quadratic", info.Quadratic);

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	shadowObjectShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, GetGaussMap());
		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("momentMap", 2);

		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

void MSMShadow::DrawMomentMaps()
{
	bool horizontal = true;
	bool first = true;

	unsigned int times =2;
	for (unsigned int i = 0; i != times; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gaussFBOs[horizontal]);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glDisable(GL_BLEND);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		shadowmomentShader.use();

		glActiveTexture(GL_TEXTURE0);
		unsigned int texture = first ? depthMap : gaussMaps[!horizontal];
		glBindTexture(GL_TEXTURE_2D, texture);
		shadowmomentShader.setInt("depthMap", 0);

		shadowmomentShader.setInt("kernelSize", kernelSize);
		shadowmomentShader.setBool("horizontal", horizontal);
		shadowmomentShader.setBool("last", i==times-1);

		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		if (first)
			first = false;
		horizontal = !horizontal;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int MSMShadow::GetGaussMap()
{
	return gaussMaps[0];
}

class VSMShadow :public Shadow
{
public:
	VSMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, int _kernelSize = 2, float near = 0.1f, float far = 100.0f);
	void DrawMaps(vector<Object> objects, unsigned int r = 2);
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
	unsigned int depthMapFBO, depthMap, depth2MapFBO, depth2Map, SATMapFBO[2], SATMap[2], SAT;
private:
	void DrawShadowMap(vector<Object> objects)  override;
	void DrawShadow2Map();
	void DrawSATs(unsigned int r);
	unsigned int GetSAT();

	void GetFramebuffer();
	void GetVertexArray();
	glm::vec3 direction;
	float near_plane, far_plane;
	float lightWidth, orthoWidth = 100.0f;
	unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	Shader shadowmapShader = Shader("res/shader/shadowMap_Dir_Vertex.shader", "res/shader/shadowMap_Dir_Fragment.shader");
	Shader shadowmap2Shader = Shader("res/shader/shadowMap_Square_Vertex.shader", "res/shader/shadowMap_Square_Fragment.shader");
	Shader satHorizontalShader = Shader("res/shader/satVertex.shader", "res/shader/satFragment_Horizontal.shader");
	Shader satVerticalShader = Shader("res/shader/satVertex.shader", "res/shader/satFragment_Vertical.shader");
	Shader shadowObjectShader = Shader("res/shader/shadow_VSM_Vertex.shader", "res/shader/shadow_VSM_Fragment.shader");
	DotLight light;
	glm::mat4 lightSpaceMatrix;

	unsigned int frameVAO, frameVBO;
	int kernelSize;
};

VSMShadow::VSMShadow(DotLight _light, glm::vec3 _direction, float _lightWidth, int _kernelSize, float near, float far)
	:direction(_direction), lightWidth(_lightWidth), kernelSize(_kernelSize), near_plane(near), far_plane(far)
{
	GetFramebuffer();
	GetVertexArray();
	light = DotLight(_light);

	glm::mat4 lightProjection = glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f, -orthoWidth / 2.0f, orthoWidth / 2.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(light.Position, light.Position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;
}

void VSMShadow::GetFramebuffer()
{
	//shadow map
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 0.0,0.0,0.0,0.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//shadow square map
	glGenFramebuffers(1, &depth2MapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depth2MapFBO);

	glGenTextures(1, &depth2Map);
	glBindTexture(GL_TEXTURE_2D, depth2Map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depth2Map, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//sat map
	glGenFramebuffers(2, SATMapFBO);
	glGenTextures(2, SATMap);

	for (unsigned int i = 0; i != 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, SATMapFBO[i]);
		glBindTexture(GL_TEXTURE_2D, SATMap[i]);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		GLfloat borderColor[] = { 0.0,0.0,0.0,0.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SATMap[i], 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VSMShadow::DrawShadowMap(vector<Object> objects)
{
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glFrontFace(GL_CCW);

	shadowmapShader.use();

	shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));
		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	//glCullFace(GL_BACK);
	//glDisable(GL_CULL_FACE);
}

void VSMShadow::DrawShadow2Map()
{
	glBindFramebuffer(GL_FRAMEBUFFER, depth2MapFBO);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	shadowmap2Shader.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	shadowmap2Shader.setInt("depthMap", 0);

	glBindVertexArray(frameVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VSMShadow::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	shadowObjectShader.use();

	LightInfo info;
	light.GetLightInfo(info);
	shadowObjectShader.setVec3("pointlight.ambient", info.Ambient);
	shadowObjectShader.setVec3("pointlight.diffuse", info.Diffuse);
	shadowObjectShader.setVec3("pointlight.specular", info.Specular);
	shadowObjectShader.setVec3("pointlight.position", info.Position);
	shadowObjectShader.setFloat("pointlight.constant", info.Constant);
	shadowObjectShader.setFloat("pointlight.linear", info.Linear);
	shadowObjectShader.setFloat("pointlight.quadratic", info.Quadratic);

	shadowObjectShader.setFloat("material.shininess", 32.0f);
	shadowObjectShader.setVec3("viewPos", viewPos);

	shadowObjectShader.setFloat("far_plane", far_plane);
	shadowObjectShader.setFloat("near_plane", near_plane);
	shadowObjectShader.setFloat("lightWidth", lightWidth);
	shadowObjectShader.setFloat("orthoWidth", orthoWidth);

	shadowObjectShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	shadowObjectShader.setMat4("view", glm::value_ptr(view));
	shadowObjectShader.setMat4("projection", glm::value_ptr(projection));

	shadowObjectShader.setInt("kernelSize", kernelSize);

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, GetSAT());

		shadowObjectShader.setInt("material.diffuse", 0);
		shadowObjectShader.setInt("material.specular", 1);
		shadowObjectShader.setInt("satMap", 2);

		shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

void VSMShadow::GetVertexArray()
{
	float frameVertices[] =
	{
		-1.0, -1.0,  0.0, 0.0,
		 1.0, -1.0,  1.0, 0.0,
		 1.0,  1.0,  1.0, 1.0,
		 1.0,  1.0,  1.0, 1.0,
		-1.0,  1.0,  0.0, 1.0,
		-1.0, -1.0,  0.0, 0.0
	};

	glGenBuffers(1, &frameVBO);
	glGenVertexArrays(1, &frameVAO);

	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), frameVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void VSMShadow::DrawSATs(unsigned int r)
{
	unsigned int n = (unsigned int)logbase(SHADOW_WIDTH, r);
	unsigned int m = (unsigned int)logbase(SHADOW_HEIGHT, r);

	bool flapFlag = false;

	for (unsigned int i = 0; i != n; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, SATMapFBO[flapFlag]);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		satHorizontalShader.use();

		satHorizontalShader.setInt("r", (int)r);
		satHorizontalShader.setInt("index", (int)i);

		//satHorizontalShader.setBool("first", i == 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, i == 0 ? depth2Map : SATMap[!flapFlag]);
		satHorizontalShader.setInt("satMap", 0);

		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		flapFlag = !flapFlag;
	}

	for (unsigned int i = 0; i != m; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, SATMapFBO[flapFlag]);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		satVerticalShader.use();

		satVerticalShader.setInt("r", (int)r);
		satVerticalShader.setInt("index", (int)i);

		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, i == 0 ? depthMap : SATMap[!flapFlag]);
		glBindTexture(GL_TEXTURE_2D, SATMap[!flapFlag]);
		satVerticalShader.setInt("satMap", 0);

		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		flapFlag = !flapFlag;
	}

	SAT = SATMap[!flapFlag];
}

inline unsigned int VSMShadow::GetSAT()
{
	return SAT;
}

void VSMShadow::DrawMaps(vector<Object> objects, unsigned int r)
{
	DrawShadowMap(objects);
	DrawShadow2Map();
	DrawSATs(r);
}

#endif
