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
#include <glad/gl.h>		// Include glad
#include <string>           // Inlude std::string
#include <vector>           // Inlude std::string

namespace mikroplot {

    class Shader {
    public:
        Shader(const std::string& vertexShaderString, const std::string& fragmentShaderString);
        ~Shader();

        template<typename F>
        void use(F f){
            bind();
            f();
            unbind();
        }
        void setUniformv(const std::string& name, const std::vector<float>& v);
        void setUniform(const std::string& name, float v);
        void setUniform(const std::string& name, float x, float y);
        void setUniform(const std::string& name, float x, float y, float z);
        void setUniform(const std::string& name, float x, float y, float z, float w);
        void setUniformm(const std::string& name, const std::vector<float> & m, bool transposed=false);
        void setUniform(const std::string& name, int value);

    private:

        void bind();
        void unbind();
        GLint m_shaderProgram;	// Handle to the shader program
    };

}
