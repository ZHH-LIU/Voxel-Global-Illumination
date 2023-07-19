#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <iostream>

#include "image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<string>
#include<vector>

using std::string;
using std::vector;

struct pbrMaps {
	unsigned int albedoMap, normalMap, metalnessMap, aoMap, roughnessMap;
};

unsigned int LoadTexture(const char* path, bool isSRGB = true)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 1)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		else if (nrChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		else if (nrChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture." << std::endl;
	}
	stbi_image_free(data);
	return texture;
}

class Object {
public:
	glm::mat4 model = glm::mat4(1.0f);
	unsigned int VAO, VBO, EBO;
	unsigned int texture_diffuse, texture_specular;
	pbrMaps PBR;
	float Shininess=32;
	float Roughness = 0.5;
	unsigned int Count;
	virtual void GetVertexArray(unsigned int n, bool out) {}
	virtual void GetTextures(const char* diffuse, const char* specular) {}
	void SetModel(const glm::vec3 &_position, const float &_scale=1.0f, const float &_angle=0.0f, const glm::vec3 &_axis=glm::vec3(0.0,1.0,0.0))
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, _position);
		model = glm::scale(model, glm::vec3(_scale));
		model = glm::rotate(model, glm::radians(_angle), _axis);
	}
	void SetModel(const glm::vec3& _position, const glm::vec3 & _scale = glm::vec3(1.0f), const float& _angle = 0.0f, const glm::vec3& _axis = glm::vec3(0.0, 1.0, 0.0))
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, _position);
		model = glm::scale(model, _scale);
		model = glm::rotate(model, glm::radians(_angle), _axis);
	}
	void SetPBR(const char* albedo, const char* normal, const char* metalness, const char* ao, const char* roughness);
};

void Object::SetPBR(const char* albedo, const char* normal, const char* metalness, const char* ao, const char* roughness)
{
	//albedo
	if (albedo)
		PBR.albedoMap = LoadTexture(albedo);
	//normal
	if (normal)
		PBR.normalMap = LoadTexture(normal,false);
	//metalness
	if (metalness)
		PBR.metalnessMap = LoadTexture(metalness, false);

	//ao
	if (ao)
		PBR.aoMap = LoadTexture(ao);
	//roughness
	if (roughness)
		PBR.roughnessMap = LoadTexture(roughness, false);
}

class Cube :public Object
{
public:
	Cube(const char* diffuse, const char* specular, bool out=true, float roughtness=0.5, float shininess=36.0f);
	void GetVertexArray(unsigned int n, bool out) override;
	void GetTextures(const char* diffuse, const char* specular) override;
};

Cube::Cube(const char* diffuse, const char* specular, bool out, float roughness, float shininess)
{
	GetVertexArray(0,out);
	GetTextures(diffuse, specular);
	Count = 36;
	Shininess = shininess;
	Roughness = roughness;
}

void Cube::GetVertexArray(unsigned int n, bool out)
{
	float vertices_out[] = {
		// ---- 位置 ----      - 纹理坐标 -   ----法线----
		-0.5f, -0.5f, 0.5f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,// 左下
		0.5f, -0.5f, 0.5f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,// 右下
		0.5f,  0.5f, 0.5f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,// 右上
		-0.5f,  0.5f, 0.5f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f, // 左上

		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,// 左下
		0.5f, -0.5f, -0.5f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,// 右下
		0.5f,  0.5f, -0.5f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,// 右上
		-0.5f,  0.5f, -0.5f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,// 左上

		-0.5f, 0.5f,-0.5f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,// 左下
		-0.5f, 0.5f, 0.5f,    1.0f, 0.0f,   0.0f, 1.0f, 0.0f, // 右下
		0.5f, 0.5f, 0.5f,    1.0f, 1.0f,   0.0f, 1.0f, 0.0f,// 右上
		0.5f, 0.5f, -0.5f,    0.0f, 1.0f,  0.0f, 1.0f, 0.0f, // 左上

		-0.5f, -0.5f,-0.5f,    0.0f, 0.0f,   0.0f, -1.0f, 0.0f,// 左下
		-0.5f, -0.5f, 0.5f,    1.0f,0.0f,   0.0f, -1.0f, 0.0f, // 右下
		 0.5f, -0.5f, 0.5f,    1.0f, 1.0f,   0.0f, -1.0f, 0.0f,// 右上
		 0.5f, -0.5f, -0.5f,    0.0f, 1.0f,  0.0f, -1.0f, 0.0f, // 左上

		0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,// 左下
		0.5f, 0.5f, -0.5f,   1.0f, 0.0f,    1.0f, 0.0f, 0.0f,// 右下
		0.5f, 0.5f,  0.5f,    1.0f, 1.0f,   1.0f, 0.0f, 0.0f,// 右上
		0.5f, -0.5f,  0.5f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f,// 左上

		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,// 左下
		-0.5f, 0.5f, -0.5f,   1.0f, 0.0f,    -1.0f, 0.0f, 0.0f,// 右下
		-0.5f, 0.5f,  0.5f,    1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,// 右上
		-0.5f, -0.5f,  0.5f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f // 左上
	};
	float vertices_in[] = {
		// ---- 位置 ----      - 纹理坐标 -   ----法线----
		-0.5f, -0.5f, 0.5f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,// 左下
		0.5f, -0.5f, 0.5f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,// 右下
		0.5f,  0.5f, 0.5f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,// 右上
		-0.5f,  0.5f, 0.5f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f, // 左上

		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,// 左下
		0.5f, -0.5f, -0.5f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,// 右下
		0.5f,  0.5f, -0.5f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,// 右上
		-0.5f,  0.5f, -0.5f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,// 左上

		-0.5f, 0.5f,-0.5f,    0.0f, 0.0f,   0.0f, -1.0f, 0.0f,// 左下
		-0.5f, 0.5f, 0.5f,    1.0f, 0.0f,   0.0f, -1.0f, 0.0f, // 右下
		0.5f, 0.5f, 0.5f,    1.0f, 1.0f,   0.0f, -1.0f, 0.0f,// 右上
		0.5f, 0.5f, -0.5f,    0.0f, 1.0f,  0.0f, -1.0f, 0.0f, // 左上

		-0.5f, -0.5f,-0.5f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,// 左下
		-0.5f, -0.5f, 0.5f,    1.0f,0.0f,   0.0f, 1.0f, 0.0f, // 右下
		 0.5f, -0.5f, 0.5f,    1.0f, 1.0f,   0.0f, 1.0f, 0.0f,// 右上
		 0.5f, -0.5f, -0.5f,    0.0f, 1.0f,  0.0f, 1.0f, 0.0f, // 左上

		0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,// 左下
		0.5f, 0.5f, -0.5f,   1.0f, 0.0f,    -1.0f, 0.0f, 0.0f,// 右下
		0.5f, 0.5f,  0.5f,    1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,// 右上
		0.5f, -0.5f,  0.5f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,// 左上

		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,// 左下
		-0.5f, 0.5f, -0.5f,   1.0f, 0.0f,    1.0f, 0.0f, 0.0f,// 右下
		-0.5f, 0.5f,  0.5f,    1.0f, 1.0f,   1.0f, 0.0f, 0.0f,// 右上
		-0.5f, -0.5f,  0.5f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f // 左上
	};

	unsigned int indices[] = {
		0, 1, 2, // 第一个三角形
		2, 3, 0,  // 第二个三角形
		6, 5, 4,
		4, 7, 6,
		8, 9, 10,
		10, 11, 8,
		14 ,13, 12,
		12, 15, 14,
		16, 17, 18,
		18, 19, 16,
		22, 21, 20,
		20, 23, 22
	};

	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	if (out)
	{
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_out), vertices_out, GL_STATIC_DRAW);
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_in), vertices_in, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void Cube::GetTextures(const char* diffuse, const char* specular)
{
	if (diffuse != nullptr)
	{
		texture_diffuse = LoadTexture(diffuse);
	}

	if (specular != nullptr)
	{
		texture_specular = LoadTexture(specular);
	}
}


class Sphere :public Object
{
public:
	Sphere(const char* diffuse, const char* specular, float roughness=0.5, float shininess=36.0f);
	Sphere() = default;
	void GetVertexArray(unsigned int n, bool out) override;
	void GetTextures(const char* diffuse, const char* specular) override;
	unsigned int SphereVolumeVAO;
private:
	unsigned int X_SEGMENTS = 200;
	unsigned int Y_SEGMENTS = 100;
};

Sphere::Sphere(const char* diffuse, const char* specular, float roughness, float shininess)
{
	GetVertexArray(1,true);
	GetTextures(diffuse, specular);
	Shininess = shininess;
	Roughness = roughness;
}

void Sphere::GetVertexArray(unsigned int n, bool out)
{
	vector<float> vertices;
	vector<unsigned int> indices;

	float PI = 3.1415926f;

	for (unsigned int y = 0; y <= Y_SEGMENTS; y++)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; x++)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
			float yPos = cos(ySegment * PI);
			float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

			vertices.push_back(xPos);
			vertices.push_back(yPos);
			vertices.push_back(zPos);

			vertices.push_back(xSegment);
			vertices.push_back(ySegment);

			vertices.push_back(xPos);
			vertices.push_back(yPos);
			vertices.push_back(zPos);
		}
	}

	for (unsigned int i = 0; i < Y_SEGMENTS; i++)
	{
		for (unsigned int j = 0; j < X_SEGMENTS; j++)
		{
			indices.push_back(i * (X_SEGMENTS + 1) + j);
			indices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
			indices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
			
			indices.push_back(i * (X_SEGMENTS + 1) + j);
			indices.push_back(i * (X_SEGMENTS + 1) + j + 1);
			indices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
		}
	}

	Count = (unsigned int)indices.size();

	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), &vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glGenVertexArrays(1, &SphereVolumeVAO);
	glBindVertexArray(SphereVolumeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void Sphere::GetTextures(const char* diffuse, const char* specular)
{
	if (diffuse != nullptr)
	{
		texture_diffuse = LoadTexture(diffuse);
	}

	if (specular != nullptr)
	{
		texture_specular = LoadTexture(specular);
	}
}

class Square :public Object
{
public:
	Square(const char* diffuse, const char* specular, float roughness=0.5, unsigned int n = 3, float shininess = 36.0f);
	void GetVertexArray(unsigned int n, bool out) override;
	void GetTextures(const char* diffuse, const char* specular) override;
};

Square::Square(const char* diffuse, const char* specular, float roughness,unsigned int n, float shininess)
{
	GetVertexArray(n,true);
	GetTextures(diffuse, specular);
	Count = 6;
	Shininess = shininess;
	Roughness = roughness;
}

void Square::GetVertexArray(unsigned int n, bool out)
{
	float vertices[] = {
		// ---- 位置 ----      - 纹理坐标 -   ----法线----
		-0.5f, 0.0f,-0.5f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,// 左下
		-0.5f, 0.0f, 0.5f,    1.0f, 0.0f,   0.0f, 1.0f, 0.0f, // 右下
		0.5f, 0.0f, 0.5f,    1.0f, 1.0f,   0.0f, 1.0f, 0.0f,// 右上
		0.5f, 0.0f, -0.5f,    0.0f, 1.0f,  0.0f, 1.0f, 0.0f // 左上
	};
	unsigned int indices[] = {
		0, 1, 2, // 第一个三角形
		2, 3, 0  // 第二个三角形
	};

	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	if (n == 1)
	{
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}
	else if (n == 2)
	{
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}
	else if (n == 3)
	{
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	glBindVertexArray(0);
}

void Square::GetTextures(const char* diffuse, const char* specular)
{
	if (diffuse != nullptr)
	{
		texture_diffuse = LoadTexture(diffuse);
	}

	if (specular != nullptr)
	{
		texture_specular = LoadTexture(specular);
	}
}

#endif
