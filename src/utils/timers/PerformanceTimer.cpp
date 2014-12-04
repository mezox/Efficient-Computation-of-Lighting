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
Performance timer class used to measure CPU time of some parts of code. 
*/

/****************************************************************************/
/* Copyright (c) 2011, Ola Olsson, Markus Billeter
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/****************************************************************************/
#include "utils\timers\PerformanceTimer.h"

#ifdef _WIN32
  #include "utils\timers\Win32ApiWrapper.h"
#elif defined(__linux__)
  #include <time.h>
  #define NSEC_PER_SEC 1000000000llu
#else // 
  #error No implementation for the current platform available
#endif // _WIN32


unsigned int PerformanceTimer::getTickCount()
{
	#if defined(_WIN32)
	  LARGE_INTEGER i;
	  QueryPerformanceCounter(&i);
	  return i.QuadPart;
	#elif defined(__linux__)
	  timespec ts;
	  clock_gettime( CLOCK_MONOTONIC, &ts );
	  return ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
	#endif
}


unsigned int PerformanceTimer::initTicksPerSecond()
{
	#if defined(_WIN32)
	  LARGE_INTEGER hpFrequency;
	  QueryPerformanceFrequency(&hpFrequency);
	  return  hpFrequency.QuadPart;
	#elif defined(__linux__)
	  return 1000000000llu;
	#endif
}
