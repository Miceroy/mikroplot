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

namespace mikroplot {
    namespace shaders {
        static std::string projectionVSSource(){
            return
                std::string("#version 330 core\n") +
                std::string("layout (location = 0) in vec2 inPosition;\n") +
                std::string("layout (location = 1) in vec2 inTexCoord;\n") +
                std::string("uniform mat4 P;") +
                std::string("out vec2 texCoord;") +
                std::string("void main()\n") +
                std::string("{\n") +
                std::string("   texCoord = inTexCoord;\n") +
                std::string("   gl_Position = P*vec4(vec3(inPosition,0.0),1.0);\n") +
                std::string("}");
        }

        static std::string modelProjectionVSSource() {
            return
                std::string("#version 330 core\n") +
                std::string("layout (location = 0) in vec2 inPosition;\n") +
                std::string("layout (location = 1) in vec2 inTexCoord;\n") +
                std::string("uniform mat4 P;") +
                std::string("uniform mat4 M;") +
                std::string("out vec2 texCoord;") +
                std::string("void main()\n") +
                std::string("{\n") +
                std::string("   texCoord = inTexCoord;\n") +
                std::string("   gl_Position = P*M*vec4(vec3(inPosition,0.0),1.0);\n") +
                std::string("}");
        }

        static std::string shadeVSSource(){
            return std::string(
                std::string("#version 330 core\n") +
                std::string("layout (location = 0) in vec2 inPosition;\n") +
                std::string("layout (location = 1) in vec2 inTexCoord;\n") +
                std::string("out float x;\n") +
                std::string("out float y;\n") +
                std::string("out float z;\n") +
                std::string("out float w;\n") +
                std::string("uniform mat4 M;\n") +
                std::string("uniform vec2 leftBottom;\n") +
                std::string("uniform vec2 rightTop;\n") +
                std::string("void main()\n") +
                std::string("{\n") +
                std::string("   vec4 p = M*vec4(vec3(inPosition,0.0),1.0);\n") +
                std::string("   x = mix(leftBottom.x, rightTop.x, inTexCoord.x);\n") +
                std::string("   y = mix(leftBottom.y, rightTop.y, inTexCoord.y);\n") +
                std::string("   z = p.z;\n") +
                std::string("   w = p.w;\n") +
                std::string("   gl_Position = p;\n") +
                std::string("}"));
        };

        static std::string shadeFSSource(const std::string& inputUniforms, const std::string& globals, const std::string& fragmentShaderMain) {
            return std::string(
                std::string("#version 330 core\n") +
                std::string("out vec4 FragColor;\n") +
                std::string("in float x;\n") +
                std::string("in float y;\n") +
                std::string("in float z;\n") +
                std::string("in float w;\n") +
                std::string("uniform vec2 max;\n") +
                std::string("uniform vec2 min;\n") +
                std::string("uniform vec2 size;\n") +
                inputUniforms + "\n" +
                globals + "\n" +
                std::string("\nvoid main(){\n") +
                std::string("vec4 color;\n") +
                fragmentShaderMain + "\n" +
                std::string("gl_FragData[0] = color;\n") +
                std::string("\n}\n"));
        };

        static std::string textureFSSource(const std::string& inputUniforms, const std::string& globals, const std::string& shader){
            return
                std::string("#version 330 core\n") +
                std::string("in vec2 texCoord;\n") +
                std::string("out vec4 FragColor;\n")
                + inputUniforms + "\n" +
                std::string("uniform sampler2D texture0;\n")
                + globals + "\n" +
                std::string("void main(){\n") +
                std::string("vec4 color = texture2D(texture0, texCoord);\n") +
                shader +
                std::string("gl_FragData[0] = color;\n}\n");
        }

        static std::string constants(const std::vector<Constant>& inputConstants) {
            std::string res;
            for(auto& c : inputConstants){
                auto& name = c.first;
                auto& val = c.second;
                if(val.size() == 1){
                    res += "uniform float "+name+";\n";
                } else if(val.size() == 2){
                    res += "uniform vec2 "+name+";\n";
                } else if(val.size() == 3){
                    res += "uniform vec3 "+name+";\n";
                } else if(val.size() == 4){
                    res += "uniform vec4 "+name+";\n";
                } else {
                    assert(0); // Invalid value length. Must be between 1 to 4
                }
            }
            return res;
        }
    }

    namespace mesh{
        static std::unique_ptr<Mesh> create(const std::vector<vec2>& positions,const std::vector<vec2>& textureCoords) {
            std::unique_ptr<Mesh> res = std::make_unique<Mesh>();
            // Create VAOs and VBOs for sprite
            glGenVertexArrays(1, &res->vao);
            checkGLError();
            glGenBuffers(sizeof(res->vbos)/sizeof(res->vbos[0]), res->vbos);
            checkGLError();
            // And set data
            res->setVBOData(0,positions);
            res->setVBOData(1,textureCoords);
            return res;
        }

        static void render(const Mesh& mesh, GLenum mode, GLsizei count) {
            // Bind
            glBindVertexArray(mesh.vao);
            checkGLError();
            int numVertexArrys = sizeof(mesh.vbos)/sizeof(mesh.vbos[0]);
            for(size_t i=0; i<numVertexArrys; ++i){
                glEnableVertexAttribArray(i);
                checkGLError();
            }
            // Draw
            glDrawArrays(mode, 0, count);
            checkGLError();
            // Unbind
            for(size_t i=0; i<numVertexArrys; ++i){
                glDisableVertexAttribArray(i);
                checkGLError();
            }
            glBindVertexArray(0);
            checkGLError();
        };

        void Mesh::setVBOData(int index, const std::vector<vec2>& data) {
            glBindVertexArray(vao);
            checkGLError();

            glBindBuffer(GL_ARRAY_BUFFER, vbos[index]);
            checkGLError();
            glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(data[0]), &data[0], GL_STATIC_DRAW);
            checkGLError();
            glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            checkGLError();
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            checkGLError();

            glBindVertexArray(0);
            checkGLError();
        }

        void Mesh::release() {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(sizeof(vbos)/sizeof(vbos[0]), vbos);
        }
    }

    namespace quad {
        std::unique_ptr<mesh::Mesh> create() {
            static const std::vector<vec2> POSITIONS({
                vec2( 0.5f, -0.5f),
                vec2( 0.5f,  0.5f),
                vec2(-0.5f,  0.5f),
                vec2( 0.5f, -0.5f),
                vec2(-0.5f,  0.5f),
                vec2(-0.5f, -0.5f)
            });
            static const std::vector<vec2> TEXTURE_COORDS({
                vec2(1,1),
                vec2(1,0),
                vec2(0,0),
                vec2(1,1),
                vec2(0,0),
                vec2(0,1)
            });
            return mesh::create(POSITIONS,TEXTURE_COORDS);
        }

        void render(const mesh::Mesh& mesh) {
            mesh::render(mesh, GL_TRIANGLES, 6);
        };


        void setPositions(mesh::Mesh& mesh, const std::vector<vec2>& positions) {
            mesh.setVBOData(0,positions);
        }

    }

}
