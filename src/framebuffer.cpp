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
#include <mikroplot/framebuffer.h>
#include <mikroplot/texture.h>
#include <assert.h>
#include <stdexcept>

namespace mikroplot {

namespace {
    const GLenum COLOR_ATTACHMENT_LOOKUP[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6
    };
}


FrameBuffer::FrameBuffer() {
	glGenFramebuffers(1, &m_fboId);
    glGenRenderbuffers(1, &m_rboId);
}


FrameBuffer::~FrameBuffer() {
	glDeleteFramebuffers(1, &m_fboId);
    glDeleteRenderbuffers(1, &m_rboId);
}


void FrameBuffer::addColorTexture( int index, std::shared_ptr<Texture> tex ) {
	assert( index >= 0 && index < sizeof(COLOR_ATTACHMENT_LOOKUP)/sizeof(COLOR_ATTACHMENT_LOOKUP[0]) );	
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
    //glBindRenderbuffer(GL_RENDERBUFFER, m_rboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, COLOR_ATTACHMENT_LOOKUP[index], GL_TEXTURE_2D, tex->getTextureId(), 0);
    if( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER) ) {
		throw std::runtime_error("Texture could not add texture to framebuffer!");
    }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if( m_drawBuffers.size() <= std::size_t(index) ) {
		m_drawBuffers.resize(index+1);
	}
    if( m_textures.size() <= std::size_t(index) ) {
		m_textures.resize(index+1);
	}
	m_textures[index] = tex;
	m_drawBuffers[index] = COLOR_ATTACHMENT_LOOKUP[index];
}


void FrameBuffer::setDepthTexture(std::shared_ptr<Texture> tex) {
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->getTextureId(), 0);
    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		throw std::runtime_error("Texture could not add texture to framebuffer!");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
    if( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER) ) {
		throw std::runtime_error("Texture could not add to framebuffer!");
	}
    glDrawBuffers(m_drawBuffers.size(), &m_drawBuffers[0]);
}


void FrameBuffer::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}


std::shared_ptr<Texture> FrameBuffer::getTexture(int index) const {
	assert( index >= 0 && index < int(m_textures.size()) );
    return m_textures[index];
}

} // End - mikroplot



