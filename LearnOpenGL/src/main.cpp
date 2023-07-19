#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <iostream>

#include "shader.h"
#include "image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"
#include <map> 
#include "object.h"
#include "font.h"
#include "fps.h"
#include "GI3D/RSM.h" 
#include "GI3D/VXGI.h"
using std::map;

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

extern const unsigned int SCR_WIDTH = 800;
extern const unsigned int SCR_HEIGHT = 600;

//camera
Camera camera(glm::vec3(0.0f, 0.0f, -20.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

float lastTime = 0.0f, deltaTime = 0.0f;
double lastX, lastY;
bool firstMouse = true;

//FPS
FPS_COUNTER ourFPS;

int main() {

	//GLFW初始化
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_SAMPLES, 4);//多重采样缓冲

	//GLFW窗口
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VXGI", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	//GLAD初始化
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//callback
	//glViewport(0, 0, 400, 300);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//VAO VBO EBO
	float vertices[] = {
		//     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -   ----法线----
			 -0.5f, -0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,// 左下
			 0.5f, -0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,// 右下
			 0.5f,  0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,// 右上
			-0.5f,  0.5f, 0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f, // 左上

			//     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -   ----法线----
				-0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,// 左下
				0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,// 右下
				 0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,// 右上
				-0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,// 左上

				//     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -   ----法线----
					-0.5f, 0.5f,-0.5f,    0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,// 左下
					-0.5f, 0.5f, 0.5f,    1.0f, 1.0f, 0.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f, // 右下
					 0.5f, 0.5f, 0.5f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,// 右上
					0.5f, 0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,  0.0f, 1.0f, 0.0f, // 左上

					//     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -   ----法线----
						-0.5f, -0.5f,-0.5f,    0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,// 左下
						-0.5f, -0.5f, 0.5f,    1.0f, 1.0f, 0.0f,   1.0f,0.0f,   0.0f, -1.0f, 0.0f, // 右下
						 0.5f, -0.5f, 0.5f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,// 右上
						 0.5f, -0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,  0.0f, -1.0f, 0.0f, // 左上

						 //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -   ----法线----
							 0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,// 左下
							 0.5f, 0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,    1.0f, 0.0f, 0.0f,// 右下
							  0.5f, 0.5f,  0.5f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,// 右上
							  0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f,// 左上

							  //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -   ----法线----
								  -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,// 左下
								  -0.5f, 0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,    -1.0f, 0.0f, 0.0f,// 右下
								   -0.5f, 0.5f,  0.5f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,// 右上
								   -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f // 左上
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

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	unsigned int EBO;
	glGenBuffers(1, &EBO);
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

	//program
	Shader frameShader("res/shader/hdr.vert", "res/shader/hdr.frag");

	//帧缓冲
	unsigned int FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	unsigned int texColorBuffer;
	glGenTextures(1, &texColorBuffer);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

	unsigned int RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	float frameVertices[] =
	{
		-1.0, -1.0,  0.0, 0.0,
		 1.0, -1.0,  1.0, 0.0,
		 1.0,  1.0,  1.0, 1.0,
		 1.0,  1.0,  1.0, 1.0,
		-1.0,  1.0,  0.0, 1.0,
		-1.0, -1.0,  0.0, 0.0
	};

	unsigned int frameVBO;
	glGenBuffers(1, &frameVBO);
	unsigned int frameVAO;
	glGenVertexArrays(1, &frameVAO);

	glBindVertexArray(frameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(frameVertices), frameVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//rsm
	glm::vec3 ourDirPosition = glm::vec3(10.0f, 10.0f, 10.0f);
	glm::vec3 ourDirDirection = glm::vec3(-10.0f, -7.0f, -10.0f);
	glm::vec3 ourDirAmbient = glm::vec3(0.05f, 0.05f, 0.05f);
	glm::vec3 ourDirDiffuse = glm::vec3(10.0f, 10.0f, 10.0f);
	glm::vec3 ourDirSpecular = glm::vec3(0.4f, 0.4f, 0.4f);

	DirLight ourDirLight(ourDirDirection, ourDirAmbient, ourDirDiffuse, ourDirSpecular);
	DirRSM ourDriRSM(ourDirLight, ourDirPosition);
	ourDriRSM.SetTwoPass();

	const char* ourDirCubeTex = "res/texture/gold.png";
	const char* ourDirSquareTex = "res/texture/brick.jpg";
	Cube ourDirCube(ourDirCubeTex, ourDirCubeTex,true,0.6);
	ourDirCube.SetModel(glm::vec3(2.5, 1.7, 7.0), 3.0);
	Cube ourDirFloor(ourDirSquareTex, ourDirSquareTex,true,0.6);
	ourDirFloor.SetModel(glm::vec3(5.0, 0.0, 5.0), glm::vec3(10.0,0.1,10.0));
	Cube ourDirWall1(ourDirSquareTex, ourDirSquareTex,true,0.6);
	ourDirWall1.SetModel(glm::vec3(5.0, 3.0, 0.0), glm::vec3(10.0, 6.0, 0.1));
	Cube ourDirWall2(ourDirSquareTex, ourDirSquareTex,true,0.6);
	ourDirWall2.SetModel(glm::vec3(0.0, 3.0, 5.0), glm::vec3(0.1, 6.0, 10.0));
	Sphere ourDirSphere(ourDirCubeTex, ourDirCubeTex,0.8);
	ourDirSphere.SetModel(glm::vec3(7.0, 3.2, 4.0), 3.0);

	vector<Object>ourDirObjects;
	ourDirObjects.push_back(ourDirCube);
	ourDirObjects.push_back(ourDirFloor);
	ourDirObjects.push_back(ourDirWall1);
	ourDirObjects.push_back(ourDirWall2);
	ourDirObjects.push_back(ourDirSphere);

	//Font
	Font ourFont;

	//Voxel
	Cube ourVoxCube(ourDirCubeTex, ourDirCubeTex);

	glm::vec3 min(-12.0);
	glm::vec3 max(12.0);
	DirVXGI ourDirVXGI(128, &ourDriRSM, min, max);

	//GLFW渲染循环
	while (!glfwWindowShouldClose(window))
	{
		//输入
		processInput(window);

		//MSAA
		//glEnable(GL_MULTISAMPLE);

		//framebuffer
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

		//voxel
		ourDirVXGI.Voxelization(ourDirObjects, camera.Position);

		//view projection
		glm::mat4 view = camera.ViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

		//draw
		//ourDirVXGI.DrawVoxel(FBO, ourVoxCube,0, view, projection);
		//ourDirVXGI.DrawVoxel(FBO, ourDirObjects, 0, view, projection);
		ourDirVXGI.DrawObject(FBO, ourDirObjects, camera.Position, view, projection);


		//time
		float currentTime = (float)glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		ourFPS.Update();

		//Font: 放在最后，以便混合
		std::stringstream ss;
		ss << "FPS:" << ourFPS.GetFps();
		ourFont.RenderText(ss.str().substr(0, 10), 10.0f, 550.0f, 0.8f, glm::vec3(0.0, 0.5, 0.0));

		//后处理
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		frameShader.use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,  texColorBuffer);
		frameShader.setInt("texColorBuffer", 0);

		frameShader.setFloat("exposure", 1.0f);
		glBindVertexArray(frameVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//事件检查、缓冲交换
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	//释放资源
	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.PositionMove(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.PositionMove(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.PositionMove(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.PositionMove(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.PositionMove(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
		camera.PositionMove(DOWN, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	camera.ViewMove(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.FovMove(yoffset);
}
