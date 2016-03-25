// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

///////////////////////////////////////////////////////
// librealsense tutorial #3 - Point cloud generation //
///////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>

// Also include GLFW to allow for graphical display
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

//Includes for writing files
#include <fstream>
#include <iostream>

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
	// Turn on logging. We can separately enable logging to console or to file, and use different severity filters for each.
	rs::log_to_console(rs::log_severity::warn);
	//rs::log_to_file(rs::log_severity::debug, "librealsense.log");

	// Create a context object. This object owns the handles to all connected realsense devices.
	rs::context ctx;
	printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
	if (ctx.get_device_count() == 0) return EXIT_FAILURE;

	// This tutorial will access only a single device, but it is trivial to extend to multiple devices
	rs::device * dev = ctx.get_device(0);
	printf("\nUsing device 0, an %s\n", dev->get_name());
	printf("    Serial number: %s\n", dev->get_serial());
	printf("    Firmware version: %s\n", dev->get_firmware_version());

	// Configure depth and color to run with the device's preferred settings
	dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
	dev->enable_stream(rs::stream::color, rs::preset::best_quality);
	dev->start();

	bool trinsicsWritten = false;
	// Open a GLFW window to display our output
	glfwInit();
	GLFWwindow * win = glfwCreateWindow(1280, 960, "WriteTrinsicsToFile", nullptr, nullptr);
	glfwSetCursorPosCallback(win, on_cursor_pos);
	glfwSetMouseButtonCallback(win, on_mouse_button);
	glfwMakeContextCurrent(win);
	while (!trinsicsWritten)
	{
		// Wait for new frame data
		glfwPollEvents();
		dev->wait_for_frames();

		// Retrieve our images
		const uint16_t * depth_image = (const uint16_t *)dev->get_frame_data(rs::stream::depth);
		const uint8_t * color_image = (const uint8_t *)dev->get_frame_data(rs::stream::color);

		// Retrieve camera parameters for mapping between depth and color
		rs::intrinsics depth_intrin = dev->get_stream_intrinsics(rs::stream::depth);
		rs::extrinsics depth_to_color = dev->get_extrinsics(rs::stream::depth, rs::stream::color);
		rs::intrinsics color_intrin = dev->get_stream_intrinsics(rs::stream::color);
		float scale = dev->get_depth_scale();

		//write parameters to files
		
		//write depth intrinsics
		std::ofstream di_stream("depth_intrinsics.bin", std::ios::binary);
		di_stream.write((char *)&depth_intrin, sizeof(depth_intrin));
		//write depth to color extrinsics
		std::ofstream dtc_stream("depth_to_color.bin", std::ios::binary);
		dtc_stream.write((char *)&depth_to_color, sizeof(depth_to_color));
		//write color intrinsics
		std::ofstream ci_stream("color_intrinsics.bin", std::ios::binary);
		ci_stream.write((char *)&color_intrin, sizeof(color_intrin));
		//write scale
		//(oh Ashwin, why are you writing a float to a file?)
		//(because screw you internet, that's why!)
		std::ofstream s_stream("scale.bin", std::ios::binary);
		s_stream.write((char *)&scale, sizeof(color_intrin));

		//dunzos
		trinsicsWritten = true;

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


