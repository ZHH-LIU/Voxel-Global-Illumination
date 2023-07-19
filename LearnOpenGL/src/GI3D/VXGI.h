#ifndef VXGI_H
#define VXGI_H

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "../shader.h"
#include "../object.h"
#include "RSM.h"

using std::vector;

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

class DirVXGI
{
public:
	DirVXGI(unsigned int step, DirRSM* rsm, glm::vec3 _min, glm::vec3 _max)
		:Step(step), ourRSM(rsm)
	{
		min = _min;
		max = _max;
		GetImage3D();
	};
	DirRSM* ourRSM;
	unsigned int Tex;
	unsigned int Step;
	void Voxelization(vector<Object>objects, glm::vec3 viewPos);
	void GetImage3D();
	void DrawVoxel(unsigned int FBO, Object& object, int mip, const glm::mat4& view, const glm::mat4& projection);
	void DrawVoxel(unsigned int FBO, const vector<Object>& objects, int mip, const glm::mat4& view, const glm::mat4& projection);
	void DrawObject(const unsigned int FBO, const vector<Object>& objects, const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection);
	Shader vexShader= Shader("res/shader/image3D.vert", "res/shader/image3D.geom", "res/shader/image3D.frag");
	Shader drawShader = Shader("res/shader/cube.vert", "res/shader/cube.frag"); 
	Shader coneShader = Shader("res/shader/coneTracing.vert", "res/shader/coneTracing.frag");
	glm::vec3 min, max;
	glm::vec3 getVoxelPosition(unsigned int n, int step, int mip);
};

inline glm::vec3 DirVXGI::getVoxelPosition(unsigned int n, int step, int mip)
{
	step = step/glm::pow(2,mip);
	int step2 = step * step;
	int pz = (n - (n % step2)) / step2;
	n %= step2;
	int py = (n - (n % step)) / step;
	n %= step;
	int px = n;

	glm::vec3 pos = glm::vec3(px, py, pz);
	pos -= glm::vec3(step / 2);
	pos /= step;
	pos *= max - min;
	pos += (max + min) / 2.0f;
	return pos;
}

void DirVXGI::GetImage3D()
{
	glGenTextures(1, &Tex);
	glBindTexture(GL_TEXTURE_3D, Tex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexStorage3D(GL_TEXTURE_3D, 5, GL_RGBA, Step, Step, Step);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, Step, Step, Step, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindImageTexture(0, Tex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
	
	glBindTexture(GL_TEXTURE_3D, 0);
}

void DirVXGI::Voxelization(vector<Object>objects, glm::vec3 viewPos)
{
	ourRSM->DrawRSM(objects);

	glm::vec3 range = max - min;
	glm::vec3 center = (max + min) / 2.0f;

	glm::mat4 proj = glm::ortho(-range.x / 2.0f, range.x / 2.0f, -range.y / 2.0f, range.y / 2.0f, 0.2f, 0.2f + range.z);
	glm::mat4 projectionX = proj * glm::lookAt(glm::vec3(max.x+0.2, center.y, center.z), center, glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 projectionY = proj * glm::lookAt(glm::vec3(center.x, max.y+0.2, center.z), center, glm::vec3(0.0, 0.0, -1.0));
	glm::mat4 projectionZ = proj * glm::lookAt(glm::vec3(center.x, center.y, max.z+0.2), center, glm::vec3(0.0, 1.0, 0.0));

	glViewport(0, 0, Step, Step);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	vexShader.use();

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_3D, Tex);
	//vexShader.setInt("tex", 0);
	vexShader.setInt("Step", Step);
	vexShader.setVec3("minPos", min);
	vexShader.setVec3("maxPos", max);

	vexShader.setMat4("projectionX", glm::value_ptr(projectionX));
	vexShader.setMat4("projectionY", glm::value_ptr(projectionY));
	vexShader.setMat4("projectionZ", glm::value_ptr(projectionZ));

	vexShader.setMat4("lightSpaceMatrix", glm::value_ptr(ourRSM->lightSpaceMatrix));

	vexShader.setFloat("material.shininess", 32.0f);
	vexShader.setVec3("viewPos", viewPos);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ourRSM->RSM_PositionDepth);
	vexShader.setInt("gPositionDepth", 1);

	LightInfo info;
	ourRSM->light.GetLightInfo(info);
	vexShader.setVec3("dirlight.ambient", info.Ambient);
	vexShader.setVec3("dirlight.diffuse", info.Diffuse);
	vexShader.setVec3("dirlight.specular", info.Specular);
	vexShader.setVec3("dirlight.direction", info.Direction);

	int numObject = objects.size();
	for (int i = 0; i != numObject; i++)
	{
		vexShader.setMat4("model", glm::value_ptr(objects[i].model));
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		vexShader.setInt("material.diffuse", 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		vexShader.setInt("material.specular", 3);

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

	glBindTexture(GL_TEXTURE_3D, Tex);
	glGenerateMipmap(GL_TEXTURE_3D);
}

void DirVXGI::DrawVoxel(unsigned int FBO, const vector<Object>& objects, int mip, const glm::mat4& view, const glm::mat4& projection)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glEnable(GL_DEPTH_TEST);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);

	glDisable(GL_STENCIL_TEST);

	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	int step = Step / glm::pow(2, mip);
	int length = step * step * step;
	float* Data = new float[length * 4];
	glGetTextureImage(Tex, mip, GL_RGBA, GL_FLOAT, length * 4 * sizeof(float), Data);
	std::vector<glm::vec3> Voxel_Positions;
	std::vector<glm::vec3> Voxel_Colors;
	if (Data != nullptr)
	{
		for (unsigned int i = 0; i < length * 4; i += 4)
		{
			if (Data[i + 3] == 0) continue;
			glm::vec3 c = glm::vec3(Data[i], Data[i + 1], Data[i + 2]);
			glm::vec3 p = getVoxelPosition(i / 4, Step, mip);
			Voxel_Colors.push_back(c);
			Voxel_Positions.push_back(p);
			//std::cout << Data[i + 3] << std::endl;
		}
	}
	else std::cout << "¶ÁÈ¡Ê§°Ü" << std::endl;
	delete[] Data;
	drawShader.use();

	drawShader.setMat4("view", glm::value_ptr(view));
	drawShader.setMat4("projection", glm::value_ptr(projection));

	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, Tex);
	drawShader.setInt("tex", 0);
	//drawShader.setInt("tex", 0);
	drawShader.setInt("Step", Step);
	drawShader.setVec3("minPos", min);
	drawShader.setVec3("maxPos", max);
	drawShader.setInt("mip", mip);

	std::cout << Voxel_Positions.size()<<std::endl;
	for (int i = 0; i != objects.size(); i++)
	{
		drawShader.setMat4("model", glm::value_ptr(objects[i].model));

		//drawShader.setVec3("color", Voxel_Colors[i]);
		

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}
}

void DirVXGI::DrawVoxel(unsigned int FBO, Object& object, int mip, const glm::mat4& view, const glm::mat4& projection)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glEnable(GL_DEPTH_TEST);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);

	glDisable(GL_STENCIL_TEST);

	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	int step = Step / glm::pow(2, mip);
	int length = step * step * step;
	float* Data = new float[length * 4];
	glGetTextureImage(Tex, mip, GL_RGBA, GL_FLOAT, length * 4 * sizeof(float), Data);
	std::vector<glm::vec3> Voxel_Positions;
	std::vector<glm::vec3> Voxel_Colors;
	if (Data != nullptr)
	{
		for (unsigned int i = 0; i < length * 4; i += 4)
		{
			if (Data[i + 3] == 0) continue;
			glm::vec3 c = glm::vec3(Data[i], Data[i + 1], Data[i + 2]);
			glm::vec3 p = getVoxelPosition(i / 4, Step, mip);
			Voxel_Colors.push_back(c);
			Voxel_Positions.push_back(p);
		}
	}
	else std::cout << "¶ÁÈ¡Ê§°Ü" << std::endl;
	delete[] Data;
	drawShader.use();

	drawShader.setMat4("view", glm::value_ptr(view));
	drawShader.setMat4("projection", glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, Tex);
	drawShader.setInt("tex", 0);
	//drawShader.setInt("tex", 0);
	drawShader.setInt("Step", Step);
	drawShader.setVec3("minPos", min);
	drawShader.setVec3("maxPos", max);
	drawShader.setInt("mip", mip);

	glm::vec3 range = max - min;
	float _step = range.x / Step;
	//std::cout << Voxel_Positions.size()<<std::endl;
	for (int i = 0; i != Voxel_Colors.size(); i++)
	{
		object.SetModel(Voxel_Positions[i]- 0.5f * glm::vec3(_step * 0.5 * glm::pow(2, mip)), _step * 0.5 * glm::pow(2, mip));
		drawShader.setMat4("model", glm::value_ptr(object.model));

		drawShader.setVec3("color", Voxel_Colors[i]);


		glBindVertexArray(object.VAO);
		glDrawElements(GL_TRIANGLES, object.Count, GL_UNSIGNED_INT, 0);
	}
}

void DirVXGI::DrawObject(const unsigned int FBO, const vector<Object>& objects, const glm::vec3& viewPos, const glm::mat4& view, const glm::mat4& projection)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glDisable(GL_STENCIL_TEST);

	glDisable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	coneShader.use();

	coneShader.setMat4("view", glm::value_ptr(view));
	coneShader.setMat4("projection", glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, Tex);
	coneShader.setInt("tex", 0);
	coneShader.setInt("Step", Step);
	coneShader.setVec3("minPos", min);
	coneShader.setVec3("maxPos", max);

	coneShader.setVec3("viewPos", viewPos);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ourRSM->RSM_PositionDepth);
	coneShader.setInt("gPositionDepth", 1);

	LightInfo info;
	ourRSM->light.GetLightInfo(info);
	coneShader.setVec3("dirlight.ambient", info.Ambient);
	coneShader.setVec3("dirlight.diffuse", info.Diffuse);
	coneShader.setVec3("dirlight.specular", info.Specular);
	coneShader.setVec3("dirlight.direction", info.Direction);

	coneShader.setMat4("lightSpaceMatrix", glm::value_ptr(ourRSM->lightSpaceMatrix));

	int numObjects = objects.size();
	for (int i = 0; i != numObjects; i++)
	{
		coneShader.setMat4("model", glm::value_ptr(objects[i].model));
		coneShader.setFloat("material.roughness", objects[i].Roughness);
		coneShader.setFloat("material.shininess", objects[i].Shininess);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_diffuse);
		coneShader.setInt("material.diffuse", 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture_specular);
		coneShader.setInt("material.specular", 3);

		glBindVertexArray(objects[i].VAO);
		glDrawElements(GL_TRIANGLES, objects[i].Count, GL_UNSIGNED_INT, 0);
	}

}

#endif
