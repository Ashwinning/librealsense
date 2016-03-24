// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>

//Include the STD stuff we need for IO
#include <iostream>
#include <fstream>

// Also include GLFW to allow for graphical display
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

double yaw, pitch, lastX, lastY; int ml;
static void on_mouse_button(GLFWwindow * win, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) ml = action == GLFW_PRESS;
}
static double clamp(double val, double lo, double hi) { return val < lo ? lo : val > hi ? hi : val; }
static void on_cursor_pos(GLFWwindow * win, double x, double y)
{
	if (ml)
	{
		yaw = clamp(yaw - (x - lastX), -120, 120);
		pitch = clamp(pitch + (y - lastY), -80, 80);
	}
	lastX = x;
	lastY = y;
}

int main() try
{
	// Open a GLFW window to display our output
	glfwInit();
	GLFWwindow * win = glfwCreateWindow(1280, 960, "librealsense tutorial #3", nullptr, nullptr);
	glfwSetCursorPosCallback(win, on_cursor_pos);
	glfwSetMouseButtonCallback(win, on_mouse_button);
	glfwMakeContextCurrent(win);
	while (!glfwWindowShouldClose(win))
	{
		// Wait for new frame data
		glfwPollEvents();
		
		/*
		// Retrieve our images
		const uint16_t * depth_image = (const uint16_t *)dev->get_frame_data(rs::stream::depth);
		const uint8_t * color_image = (const uint8_t *)dev->get_frame_data(rs::stream::color);

		// Retrieve camera parameters for mapping between depth and color
		rs::intrinsics depth_intrin = dev->get_stream_intrinsics(rs::stream::depth);
		rs::extrinsics depth_to_color = dev->get_extrinsics(rs::stream::depth, rs::stream::color);
		rs::intrinsics color_intrin = dev->get_stream_intrinsics(rs::stream::color);
		float scale = dev->get_depth_scale();
		*/

		// Retrieve image
		//Create pointers for depth and color
		char * depthFile;
		char * colorFile;
		//Read File contents for depth and color.
		ReadFileAndReturnBytes("D:\SXSW Hat Scans\capture-20160314-103304\depth\20160314-103305.dep", depthFile);
		ReadFileAndReturnBytes("D:\SXSW Hat Scans\capture-20160314-103304\color\20160314-103305.color", colorFile);

		const uint16_t * depth_image = (const uint16_t *)depthFile;
		const uint8_t * color_image = (const uint8_t *)colorFile;

		// Set up a perspective transform in a space that we can rotate by clicking and dragging the mouse
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60, (float)1280 / 960, 0.01f, 20.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);
		glTranslatef(0, 0, +0.5f);
		glRotated(pitch, 1, 0, 0);
		glRotated(yaw, 0, 1, 0);
		glTranslatef(0, 0, -0.5f);

		// We will render our depth data as a set of points in 3D space
		glPointSize(2);
		glEnable(GL_DEPTH_TEST);
		glBegin(GL_POINTS);

		for (int dy = 0; dy<depth_intrin.height; ++dy)
		{
			for (int dx = 0; dx<depth_intrin.width; ++dx)
			{
				// Retrieve the 16-bit depth value and map it into a depth in meters
				uint16_t depth_value = depth_image[dy * depth_intrin.width + dx];
				float depth_in_meters = depth_value * scale;

				// Skip over pixels with a depth value of zero, which is used to indicate no data
				if (depth_value == 0) continue;

				// Map from pixel coordinates in the depth image to pixel coordinates in the color image
				rs::float2 depth_pixel = { (float)dx, (float)dy };
				rs::float3 depth_point = depth_intrin.deproject(depth_pixel, depth_in_meters);
				rs::float3 color_point = depth_to_color.transform(depth_point);
				rs::float2 color_pixel = color_intrin.project(color_point);

				// Use the color from the nearest color pixel, or pure white if this point falls outside the color image
				const int cx = (int)std::round(color_pixel.x), cy = (int)std::round(color_pixel.y);
				if (cx < 0 || cy < 0 || cx >= color_intrin.width || cy >= color_intrin.height)
				{
					glColor3ub(255, 255, 255);
				}
				else
				{
					glColor3ubv(color_image + (cy * color_intrin.width + cx) * 3);
				}

				// Emit a vertex at the 3D location of this depth pixel
				glVertex3f(depth_point.x, depth_point.y, depth_point.z);
			}
		}
		glEnd();

		glfwSwapBuffers(win);
	}

	return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
	// Method calls against librealsense objects may throw exceptions of type rs::error
	printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
	printf("    %s\n", e.what());
	return EXIT_FAILURE;
}

/*
	Accepts a File Path and a pointer.
	Writes the contents of the file to that location in memory.

	Usage :
	char * myFile; //Create pointer to store your file's location in memory
	ReadFileAndReturnBytes("C:\file.bin", myFile); //Read file at the path to your location in memory
	//Do something with your data.
	delete[] myFile; //delete file from memory.
*/
void ReadFileAndReturnBytes(std::string filePath, char * memblock)
{
	std::streampos size;
	//the file is open with the ios::ate flag, which means that the get pointer will be positioned at the end of the file. 
	//This way, when we call to member tellg(), we will directly obtain the size of the file.
	std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		size = file.tellg();
		//request the allocation of a memory block large enough to hold the entire file
		memblock = new char[size];
		//set the get position at the beginning of the file (we opened the file with this pointer at the end)
		file.seekg(0, std::ios::beg);
		//read the entire file
		file.read(memblock, size);
		//close the file
		file.close();
	}
	else
	{
		std::cout << "Unable to open file";
	} 
	
}