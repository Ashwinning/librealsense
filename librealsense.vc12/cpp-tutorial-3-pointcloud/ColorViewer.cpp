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



/*
Accepts a File Path and a pointer.
Writes the contents of the file to that location in memory.

Usage :
char * myFile; //Create pointer to store your file's location in memory
ReadFileAndReturnBytes("C:\file.bin", myFile); //Read file at the path to your location in memory
//Do something with your data.
delete[] myFile; //delete file from memory.
*/
char* ReadFile(std::string filePath)
{
	std::streampos size;
	char * memblock;
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
		return memblock;
	}
	else
	{
		std::cout << "Unable to open file at " + filePath + "\n";
	}
}

std::streampos GetFileSize(std::string filePath)
{
	std::cout << "Getting file size" << std::endl;
	std::ifstream is;
	is.open(filePath.c_str(), std::ios::binary);
	is.seekg(0, std::ios::end);
	std::cout << "File size : " + is.tellg() << std::endl;
	return is.tellg();
}


char* GetFileSegment(char * colorFile, std::streampos fileSize, int index, int totalSegments)
{
	char * segment;
	//int chunk = fileSize / totalSegments;
	//std::cout << "Chunk size : " + (int)chunk << std::endl;
	std::memcpy(segment, colorFile, fileSize);
	return segment;
}


int main(int argc, char* argv[]) try
{
	bool operationCompleted = false;

	std::cout << " Starting. \n ";

	//Create pointer for color
	char * colorFile;

	//Accept color file location
	std::string colorLocation;

	//Handle "Open With" scenario (default program)
	//if we had more than 1 argument as a command line parameter
	if (argc > 1)
	{
		//set color location to the 2nd argument (1st is it's own filename)
		colorLocation = argv[1];
	}
	else
	{
		//ask for the location
		std::cout << "Please enter a path to a depth file. \n";
		std::cout << "Use forward slashes '/', or double back slashes '\\'. \n";
		std::cout << "Enter file path : ";
		std::getline(std::cin, colorLocation);
	}

	std::cout << "Reading color from : " + colorLocation + "\n";

	std::streampos size = GetFileSize(colorLocation);

	//Read File contents for color.
	colorFile = ReadFile(colorLocation);
	std::cout << "File read. \n";
	

	char * frame = GetFileSegment(colorFile, size, 0, 30);
	std::cout << "Got file segment \n";
	const uint8_t * color_image = (const uint8_t *)frame;
	//const uint8_t * color_image = (const uint8_t *)colorFile;

	// Open a GLFW window to display our output
	glfwInit();
	GLFWwindow * win = glfwCreateWindow(640, 480, "ColorViewer", nullptr, nullptr);
	glfwMakeContextCurrent(win);
	while (!glfwWindowShouldClose(win))
	{
		// Wait for new frame data
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);
		glPixelZoom(1, -1);

		// Display color image as RGB triples
		glRasterPos2f(0, 1);
		glDrawPixels(640, 480, GL_RGB, GL_UNSIGNED_BYTE,color_image);

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