#ifndef RSM_H
#define RSM_H

#include <glad/glad.h> 
#include "../shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include "../object.h"
#include "../light.h"
#include<random>

using std::shared_ptr;
using std::vector;

class RSM
{
	virtual void DrawRSM(vector<Object> objects) = 0;
	virtual void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT) = 0;
};

class DirRSM :public RSM
{
public:
	DirRSM(DirLight _light, glm::vec3 _position, float near = 0.1f, float far = 30.0f);
	void SetTwoPass() {
		onePass = false;
	}
	void DrawRSM(vector<Object> objects)  override;
	void DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH = 800, unsigned int SCR_HEIGHT = 600) override;
private:
	void GetFramebuffer();
	void GetSamples();
	
	bool onePass = true;
	glm::vec3 position;
	float near_plane, far_plane;
	unsigned int SHADOW_WIDTH = 512, SHADOW_HEIGHT = 512, FIRST_WIDTH = 400, FIRST_HEIGHT = 300;
	Shader shadowmapShader = Shader("res/shader/rsm_Dir.vert", "res/shader/rsm_Dir.frag");
	Shader shadowObjectShader = Shader("res/shader/rsmObject_Dir.vert", "res/shader/rsmObject_Dir.frag");
	Shader shadowObjectPass2Shader = Shader("res/shader/rsmObject_Dir.vert", "res/shader/rsmObjectPass2_Dir.frag");
	
	
	vector<glm::vec2> Samples;
public:
	glm::mat4 lightSpaceMatrix;
	unsigned int RSMFBO, depthMap, RSM_PositionDepth, RSM_Normal, RSM_Flux, Pass1FBO, Pass1Map, Pass1Pos, Pass1Norm;
	DirLight light;
};

DirRSM::DirRSM(DirLight _light, glm::vec3 _position, float near, float far)
	:position(_position), near_plane(near), far_plane(far)
{
	GetFramebuffer();
	GetSamples();

	light = DirLight(_light);

	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(position, position + light.Direction, glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjection * lightView;
}

void DirRSM::GetSamples()
{
	std::uniform_real_distribution<float> randomFloat(0.0f, 1.0f);
	std::default_random_engine generator;
	const float rmax = 0.3;
	const float PI = 3.14159265359;
	for (unsigned int i = 0; i != 16; i++)
	{
		for (unsigned int j = 0; j != 16; j++)
		{
			float x1 = ((float)i + randomFloat(generator)) / 16.0f;
			float x2 = ((float)j + randomFloat(generator)) / 16.0f;
			glm::vec2 sample = glm::vec2(x1, x2);

			Samples.push_back(sample);
		}
	}
}

void DirRSM::GetFramebuffer()
{
	glGenFramebuffers(1, &RSMFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, RSMFBO);

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

	glGenTextures(1, &RSM_PositionDepth);
	glBindTexture(GL_TEXTURE_2D, RSM_PositionDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, RSM_PositionDepth, 0);

	glGenTextures(1, &RSM_Normal);
	glBindTexture(GL_TEXTURE_2D, RSM_Normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, RSM_Normal, 0);

	glGenTextures(1, &RSM_Flux);
	glBindTexture(GL_TEXTURE_2D, RSM_Flux);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, RSM_Flux, 0);

	unsigned int Attcahments[3] = { GL_COLOR_ATTACHMENT0 ,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, Attcahments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	
	glGenFramebuffers(1, &Pass1FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, Pass1FBO);

	glGenTextures(1, &Pass1Map);
	glBindTexture(GL_TEXTURE_2D, Pass1Map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, FIRST_WIDTH, FIRST_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Pass1Map, 0);

	glGenTextures(1, &Pass1Pos);
	glBindTexture(GL_TEXTURE_2D, Pass1Pos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, FIRST_WIDTH, FIRST_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Pass1Pos, 0);

	glGenTextures(1, &Pass1Norm);
	glBindTexture(GL_TEXTURE_2D, Pass1Norm);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, FIRST_WIDTH, FIRST_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, Pass1Norm, 0);

	unsigned int Pass1Attcahments[3] = { GL_COLOR_ATTACHMENT0 ,GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, Pass1Attcahments);

	unsigned int RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, FIRST_WIDTH, FIRST_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirRSM::DrawRSM(vector<Object> objects)
{
	glBindFramebuffer(GL_FRAMEBUFFER, RSMFBO);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	for (unsigned int i = 0; i != objects.size(); i++)
	{
		shadowmapShader.use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		shadowmapShader.setInt("texture_diffuse", 0);

		shadowmapShader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
		shadowmapShader.setMat4("model", glm::value_ptr(objects[i].model));

		shadowmapShader.setVec3("light", light.Diffuse);

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirRSM::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
	if (onePass)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glEnable(GL_DEPTH_TEST);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glFrontFace(GL_CCW);

		glDisable(GL_STENCIL_TEST);

		glDisable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		shadowObjectShader.setBool("onePass", onePass);

		for (unsigned int i = 0; i != 256; i++)
		{
			shadowObjectShader.setVec2(("samples[" + std::to_string(i) + "]").c_str(), Samples[i]);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, RSM_PositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, RSM_Normal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, RSM_Flux);
		shadowObjectShader.setInt("gPositionDepth", 0);
		shadowObjectShader.setInt("gNormal", 1);
		shadowObjectShader.setInt("gFlux", 2);

		for (unsigned int i = 0; i != objects.size(); i++)
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
			shadowObjectShader.setInt("material.diffuse", 3);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
			shadowObjectShader.setInt("material.specular", 4);

			shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

			glBindVertexArray(objects[i].VAO);
			glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
		}
	}
	else
	{
		//First Pass
		glBindFramebuffer(GL_FRAMEBUFFER, Pass1FBO);
		glViewport(0, 0, 400, 300);
		glEnable(GL_DEPTH_TEST);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glFrontFace(GL_CCW);

		glDisable(GL_STENCIL_TEST);

		glDisable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		shadowObjectShader.setBool("onePass", onePass);

		for (unsigned int i = 0; i != 256; i++)
		{
			shadowObjectShader.setVec2(("samples[" + std::to_string(i) + "]").c_str(), Samples[i]);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, RSM_PositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, RSM_Normal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, RSM_Flux);
		shadowObjectShader.setInt("gPositionDepth", 0);
		shadowObjectShader.setInt("gNormal", 1);
		shadowObjectShader.setInt("gFlux", 2);

		for (unsigned int i = 0; i != objects.size(); i++)
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
			shadowObjectShader.setInt("material.diffuse", 3);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
			shadowObjectShader.setInt("material.specular", 4);

			shadowObjectShader.setMat4("model", glm::value_ptr(objects[i].model));

			glBindVertexArray(objects[i].VAO);
			glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
		}

		//Second Pass
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glEnable(GL_DEPTH_TEST);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glFrontFace(GL_CCW);

		glDisable(GL_STENCIL_TEST);

		glDisable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shadowObjectPass2Shader.use();

		shadowObjectPass2Shader.setVec3("dirlight.ambient", light.Ambient);
		shadowObjectPass2Shader.setVec3("dirlight.diffuse", light.Diffuse);
		shadowObjectPass2Shader.setVec3("dirlight.specular", light.Specular);
		shadowObjectPass2Shader.setVec3("dirlight.direction", light.Direction);

		shadowObjectPass2Shader.setFloat("material.shininess", 32.0f);
		shadowObjectPass2Shader.setVec3("viewPos", viewPos);

		shadowObjectPass2Shader.setMat4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
		shadowObjectPass2Shader.setMat4("view", glm::value_ptr(view));
		shadowObjectPass2Shader.setMat4("projection", glm::value_ptr(projection));


		for (unsigned int i = 0; i != 256; i++)
		{
			shadowObjectPass2Shader.setVec2(("samples[" + std::to_string(i) + "]").c_str(), Samples[i]);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, RSM_PositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, RSM_Normal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, RSM_Flux);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, Pass1Map);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, Pass1Pos);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, Pass1Norm);
		shadowObjectPass2Shader.setInt("gPositionDepth", 0);
		shadowObjectPass2Shader.setInt("gNormal", 1);
		shadowObjectPass2Shader.setInt("gFlux", 2);
		shadowObjectPass2Shader.setInt("Pass1Map", 3);
		shadowObjectPass2Shader.setInt("Pass1Pos", 4);
		shadowObjectPass2Shader.setInt("Pass1Norm", 5);

		for (unsigned int i = 0; i != objects.size(); i++)
		{
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
			shadowObjectPass2Shader.setInt("material.diffuse", 6);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
			shadowObjectPass2Shader.setInt("material.specular", 7);

			shadowObjectPass2Shader.setMat4("model", glm::value_ptr(objects[i].model));

			glBindVertexArray(objects[i].VAO);
			glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
		}
	}
}

class DotRSM :public RSM
{
public:
	DotRSM() = default;
	DotRSM(DotLight _light, float _near = 0.1f, float _far = 200.0f);

	void DrawRSM(vector<Object> objects)  override;
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

DotRSM::DotRSM(DotLight _light, float _near, float _far)
	: near(_near), far(_far)
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

void DotRSM::GetFramebuffer()
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

void DotRSM::DrawRSM(vector<Object> objects)
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

void DotRSM::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
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

class SpotRSM :public RSM
{
public:
	SpotRSM() = default;
	SpotRSM(SpotLight _light, float _near = 0.1f, float _far = 200.0f);

	void DrawRSM(vector<Object> objects)  override;
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

SpotRSM::SpotRSM(SpotLight _light, float _near, float _far)
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

void SpotRSM::GetFramebuffer()
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

void SpotRSM::DrawRSM(vector<Object> objects)
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

void SpotRSM::DrawObjects(vector<Object>objects, unsigned int FBO, glm::vec3 viewPos, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
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

#endif