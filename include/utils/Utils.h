/*
Project:		Efficient computation of Lighting
Type:			Bachelor's thesis
Author:			Tomáš Kubovčík, xkubov02@stud.fit.vutbr.cz
Supervisor:		Ing. Tomáš Milet
School info:	Brno Univeristy of Technology (VUT)
Faculty of Information Technology (FIT)
Department of Computer Graphics and Multimedia (UPGM)

Project information
---------------------
The goal of this project is to efficiently compute lighting in scenes
with hundrends to thousands light sources. To handle this there have been
implemented lighting techniques as deferred shading, tiled deferred shading
and tiled forward shading. Application requires GPU supporting OpenGL 3.3+
but may be compatible with older versions. Application logic was implemented
using C/C++ with some external helper libraries to handle basic operations.

File information
-----------------
This file implements some utilities used in application such as random number
generator, fps counter, etc...
*/

#include <GLFW/glfw3.h>
#include <sstream>
#include <string>
#include <cstdlib>         //rand()

/// <summary>
/// Random float number calculator.
/// </summary>
/// <param name="min">lower limit.</param>
/// <param name="max">upper limit.</param>
/// <returns>generated random number from range <min, max></returns>
float randf(float min, float max)
{
	float r = (float)rand() / (float)RAND_MAX;

	return (min + r*(max - min));
}

/// <summary>
/// Templated version of my_equal so it could work with both char and wchar_t
/// </summary>
template<typename charT>
struct my_equal {
	my_equal(const std::locale& loc) : loc_(loc) {}
	bool operator()(charT ch1, charT ch2) {
		return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
	}
private:
	const std::locale& loc_;
};

/// <summary>
/// Find substring (case insensitive)
/// </summary>
/// <param name="str1">Source string.</param>
/// <param name="str2">substring.</param>
/// <param name="loc">default location.</param>
/// <returns>location of substring's first character in source string</returns>
template<typename T>
int findSubstringCI(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
	T::const_iterator it = std::search(str1.begin(), str1.end(),
		str2.begin(), str2.end(), my_equal<T::value_type>(loc));
	if (it != str1.end()) return it - str1.begin();
	else return -1; // not found
}

/// <summary>
/// Calculates the FPS. [source: http://r3dux.org/2012/07/a-simple-glfw-fps-counter/]
/// </summary>
/// <param name="theTimeInterval">The time interval for calculating fps.</param>
/// <param name="theWindowTitle">application window title.</param>
/// <returns>fps</returns>
double calcFPS(GLFWwindow * win, double theTimeInterval = 1.0, std::string theWindowTitle = "NONE")
{
	// Static values which only get initialised the first time the function runs
	static double t0Value = glfwGetTime(); // Set the initial time to now
	static int    fpsFrameCount = 0;             // Set the initial FPS frame count to 0
	static double fps = 0.0;           // Set the initial FPS value to 0.0

	// Get the current time in seconds since the program started (non-static, so executed every time)
	double currentTime = glfwGetTime();

	// Ensure the time interval between FPS checks is sane (low cap = 0.1s, high-cap = 10.0s)
	// Negative numbers are invalid, 10 fps checks per second at most, 1 every 10 secs at least.
	if (theTimeInterval < 0.1)
	{
		theTimeInterval = 0.1;
	}
	if (theTimeInterval > 10.0)
	{
		theTimeInterval = 10.0;
	}

	// Calculate and display the FPS every specified time interval
	if ((currentTime - t0Value) > theTimeInterval)
	{
		// Calculate the FPS as the number of frames divided by the interval in seconds
		fps = (double)fpsFrameCount / (currentTime - t0Value);

		// If the user specified a window title to append the FPS value to...
		if (theWindowTitle != "NONE")
		{
			// Convert the fps value into a string using an output stringstream
			std::ostringstream stream;
			stream << fps;
			std::string fpsString = stream.str();

			// Append the FPS value to the window title details
			theWindowTitle += " | FPS: " + fpsString;

			// Convert the new window title to a c_str and set it
			const char* pszConstString = theWindowTitle.c_str();
			glfwSetWindowTitle(win, pszConstString);
		}

		// Reset the FPS frame counter and set the initial time to be now
		fpsFrameCount = 0;
		t0Value = glfwGetTime();
	}
	else // FPS calculation time interval hasn't elapsed yet? Simply increment the FPS frame counter
	{
		fpsFrameCount++;
	}

	// Return the current FPS - doesn't have to be used if you don't want it!
	return fps;
}