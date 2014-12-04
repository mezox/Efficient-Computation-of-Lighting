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
Buffer class definition.
*/


/****************************************************************************/
/* Copyright (c) 2011, Ola Olsson
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

#include <GL/glew.h>
#include <iostream>

template <typename T>
class GlBufferObject
{
	public:
		GlBufferObject() : m_id(0), m_elements(0)
		{
		}

		~GlBufferObject()
		{
			if (m_id)
				glDeleteBuffers(1, &m_id);
		}

		void init(size_t elements, const T *hostData = 0, unsigned int dataUpdateKind = GL_DYNAMIC_COPY)
		{
			m_dataUpdateKind = dataUpdateKind;
			//ASSERT(!m_id);
			
			glGenBuffers(1, &m_id);
			
			m_elements = elements;
			if (elements)
			{
				copyFromHost(hostData, elements);
			}
		}

		void resize(size_t elements)
		{
			copyFromHost(0, elements);
		}

		size_t size() const
		{
			return m_elements;
		}

		operator GLuint() const
		{
			//ASSERT(m_id > 0);
			return m_id;
		}

		void copyFromHost(const T *hostData, size_t elements)
		{
			//ASSERT(elements > 0);
			m_elements = elements;
			glBindBuffer(GL_ARRAY_BUFFER, m_id);
			

			// buffer data
			glBufferData(GL_ARRAY_BUFFER, m_elements * sizeof(T), hostData, m_dataUpdateKind);
			

			// make sure buffer is not bound
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			
		}

		void bind(GLenum target = GL_ARRAY_BUFFER, unsigned int offset = 0) const
		{
			//ASSERT(offset == 0);
			glBindBuffer(target, m_id);
			
		}

		void bindSlot(GLenum target, unsigned int slot) const
		{
			glBindBufferBase(target, slot, m_id);
			
		}

		void bindSlotRange(GLenum target, unsigned int slot, unsigned int offset, unsigned int count = 1) const
		{
			size_t tmp = sizeof(T)* offset;
			glBindBufferRange(target, slot, m_id, tmp, sizeof(T)* count);
			
		}

		void unbind(GLenum target = GL_ARRAY_BUFFER) const
		{
			glBindBuffer(target, 0);
		}

	private:
		size_t m_elements;
		unsigned int m_id;
		unsigned int m_dataUpdateKind;
};
