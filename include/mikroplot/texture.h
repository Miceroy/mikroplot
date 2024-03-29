//// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= ////
//// MikRoPlot - C++ Plotting made easy.
////
//// MIT License
////
//// Copyright (c) 2022 Mikko Romppainen.
////
//// Permission is hereby granted, free of charge, to any person obtaining
//// a copy of this software and associated documentation files (the
//// "Software"), to deal in the Software without restriction, including
//// without limitation the rights to use, copy, modify, merge, publish,
//// distribute, sublicense, and/or sell copies of the Software, and to
//// permit persons to whom the Software is furnished to do so, subject to
//// the following conditions:
////
//// The above copyright notice and this permission notice shall be included
//// in all copies or substantial portions of the Software.
////
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= ////
#pragma once
//#include <glad/gl.h>		// Include glad
#include <stdint.h>

namespace mikroplot {

	class Texture {
	public:
		Texture() : m_textureId(-1), m_width(0), m_height(0) {}
		Texture(int width, int height, int nrChannels, const uint8_t* data);
		Texture(int width, int height, int nrChannels, const float* data);
		Texture(int width, int height, bool isDepthTexture);
		~Texture();

		uint32_t getTextureId() const;
		auto getWidth() const {return m_width;}
		auto getHeight() const {return m_height;}

	private:
		uint32_t				m_textureId;	// Texture id
		int m_width;
		int m_height;
	};

}
