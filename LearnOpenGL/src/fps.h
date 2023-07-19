//////////////////////////////////////////////////////////////////////////////////////////
//	FPS_COUNTER.h
//	Frames per sceond counter class
//	Downloaded from: www.paulsprojects.net
//	Created:	20th July 2002
//
//	Copyright (c) 2006, Paul Baker
//	Distributed under the New BSD Licence. (See accompanying file License.txt or copy at
//	http://www.paulsprojects.net/NewBSDLicense.txt)
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

class FPS_COUNTER
{
public:
	FPS_COUNTER() : fps(0.0f), lastTime(0.0f), frames(0), time(0.0f)
	{}
	~FPS_COUNTER() {}

	void Update(void);
	float GetFps(void) { return fps; }

protected:
	float fps;

	float lastTime;
	int frames;
	float time;
};

void FPS_COUNTER::Update(void)
{
	//keep track of time passed and frame count
	time = glfwGetTime();
	++frames;

	//If a second has passed
	if (time - lastTime > 1.0f)
	{
		fps = frames / (time - lastTime);	//update the number of frames per second
		lastTime = time;				//set time for the start of the next count
		frames = 0;					//reset fps for this second
	}
}

#endif	//FPS_COUNTER_H