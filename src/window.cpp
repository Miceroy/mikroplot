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
#include <mikroplot/window.h>
#include <glad/gl.h>		// Include glad
#include <GLFW/glfw3.h>		// Include glfw
#include <string>
#include <vector>
#include <stdexcept>
#include <stb_image_write.h>
#include <assert.h>
#include <algorithm>
#include <mikroplot/shader.h>
#include <mikroplot/framebuffer.h>
#include <mikroplot/texture.h>
#include <mikroplot/GLUtils.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// If ytou want to take screenshots, you must speciy following:
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;

namespace mikroplot {
    static const std::string SSQ_VERT =
        std::string("#version 330 core\n") +
        std::string("layout (location = 0) in vec2 inPosition;\n") +
        std::string("layout (location = 1) in vec2 inTexCoord;\n") +
        std::string("uniform mat4 M;") +
        std::string("out vec2 texCoord;") +
        std::string("void main()\n") +
        std::string("{\n") +
        std::string("   texCoord = inTexCoord;\n") +
        std::string("   gl_Position = M*vec4(vec3(inPosition,0.0),1.0);\n") +
        std::string("}");

    static const std::string SSQ_FRAG =
        std::string("#version 330 core\n") +
        std::string("in vec2 texCoord;\n") +
        std::string("out vec4 FragColor;\n") +
        std::string("uniform sampler2D texture0;\n") +
        std::string("void main(){\n") +
        std::string("gl_FragData[0] = texture2D(texture0, texCoord);\n") +
        std::string("\n}\n");

    static const std::string SPRITE_VERT =
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

    std::string SPRITE_FRAG(const std::string& shader,const std::string& inputUniforms, const std::string& globals){
        return
        std::string("#version 330 core\n") +
        std::string("in vec2 texCoord;\n") +
        std::string("out vec4 FragColor;\n") + inputUniforms +
        std::string("\nuniform sampler2D texture0;\n") + globals +
        std::string("\nvoid main(){\n") +
        std::string("vec4 color = texture2D(texture0, texCoord);\n") +
        shader +
        std::string("gl_FragData[0] = color;\n}\n");
    }

    static ma_engine g_engine;

    // Class for static initialization of glfw
    class StaticInit {
    public:
        StaticInit() {
            // Set c++-lambda as error call back function for glfw.
            glfwSetErrorCallback([](int error, const char* description) {
                fprintf(stderr, "Error %dm_spriteVao: %s\n", error, description);
            });
            // Try to initialize glfw
            if (!glfwInit()) {
                throw std::runtime_error("Failed to initialize OpenGL!");
                return;
            }

            auto result = ma_engine_init(NULL, &g_engine);
            if (result != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialize audio engine!");
                return;
            }
        }
        ~StaticInit() {
            ma_engine_uninit(&g_engine);

            // Terminate glfw
            glfwTerminate();
        }
    };

    static StaticInit init;

    Window::Window(int sizeX, int sizeY, const std::string& title, const std::vector<RGBA>& palette, int clearColor)
        : m_clearColor(clearColor)
        , m_width(sizeX)
        , m_height(sizeY)
        , m_palette(palette)
        , m_window(0)
        , m_left(0)
        , m_right(0)
        , m_bottom(0)
        , m_top(0)
        , m_shadeFbo()
        , m_ssqVao(0)
        , m_spriteVao(0)
    {
        for(size_t i=0; i<2; ++i){
            m_ssqVbos[i] =0;
        }
        for(size_t i=0; i<2; ++i){
            m_spriteVbos[i] =0;
        }

        // Create window and check that creation was succesful.

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(m_width+2, m_height+2, title.c_str(), 0, 0);
        if (!m_window) {
            throw std::runtime_error("Failed to create window!");
            return;
        }

        // Set current context
        glfwMakeContextCurrent(m_window);
        // Load GL functions using glad
        gladLoadGL(glfwGetProcAddress);

        // Specify the key callback as c++-lambda to glfw
        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            // Close window if escape is pressed by the user.
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        });

        glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);

        m_ssqShader = std::make_unique<Shader>(SSQ_VERT, SSQ_FRAG);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

        // Create VAOs and VBOs
        glGenVertexArrays(1, &m_ssqVao);
        checkGLError();
        glGenBuffers(2, m_ssqVbos);
        checkGLError();

        glGenVertexArrays(1, &m_spriteVao);
        checkGLError();
        glGenBuffers(2, m_spriteVbos);
        checkGLError();

        // Query the size of the framebuffer (window content) from glfw.
        int screenWidth, screenHeight;
        glfwGetFramebufferSize(m_window, &screenWidth, &screenHeight);
        // Setup the opengl wiewport (i.e specify area to draw)
        glViewport(0, 0, screenWidth, screenHeight);
        setScreen(0,screenWidth,0,screenHeight);

        // Create FBOs
        m_shadeFbo = std::make_unique<FrameBuffer>();
        m_shadeFbo->addColorTexture(0, std::make_shared<Texture>(screenWidth, screenHeight, false));
    }

    Window::~Window() {
        m_shadeFbo = 0;

        glDeleteVertexArrays(1, &m_ssqVao);
        glDeleteBuffers(2, m_ssqVbos);

        glDeleteVertexArrays(1, &m_spriteVao);
        glDeleteBuffers(2, m_spriteVbos);

        // Destroy window
        glfwDestroyWindow(m_window);
    }


    int Window::getKeyState(int keyCode){
        int state = glfwGetKey(m_window, keyCode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    int Window::getKeyPressed(int keyCode){
        int state = glfwGetKey(m_window, keyCode);
        return state == GLFW_PRESS;
    }

    int Window::getKeyReleased(int keyCode){
        int state = glfwGetKey(m_window, keyCode);
        return state == GLFW_RELEASE;
    };

    int Window::update() {
        if (shouldClose()) {
            return -1;
        }
        // Set current context
        glfwMakeContextCurrent(m_window);
        drawScreenSizeQuad(m_shadeFbo->getTexture(0).get());
        glfwSwapBuffers(m_window);

        if(m_screenshotFileName.length()>0){
            takeScreenshot(m_screenshotFileName);
            m_screenshotFileName = "";
        }

        auto rgb = m_palette[m_clearColor];
        glClearColor(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,1.0);
        //glClearColor(1.0f,1.0f,1.0f,1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glFinish();
        m_shadeFbo->use([](){
            //glClearColor(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,0.0);
            glClearColor(0.0f,0.0f,0.0f,0.0);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glFinish();
        });
        // Poll other window events.
        glfwPollEvents();
        if (glfwWindowShouldClose(m_window)) {
            return -1;
        }
        return 0;
    }

    void Window::takeScreenshot(const std::string filename) {
        // Set current context
        glfwMakeContextCurrent(m_window);
        int width, height;
        int channels = 4;
        glfwGetFramebufferSize(m_window, &width, &height);
        std::vector<uint8_t> lastFrame;
        lastFrame.resize(channels*width*height);
        glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE, &lastFrame[0]);
        for(size_t i=0; i<lastFrame.size(); ++i){
            if((i%4) == 3){
                lastFrame[i] = 0xff;
            }
        }
        stbi_flip_vertically_on_write(true);
        stbi_write_png(filename.c_str(), width, height, channels, &lastFrame[0], width*channels);
        stbi_flip_vertically_on_write(false);
    }

    void Window::screenshot(const std::string filename) {        
        m_screenshotFileName = filename;
    }

    bool Window::shouldClose() {
        glfwPollEvents();
        return m_window == 0 || glfwWindowShouldClose(m_window);
    }


    void Window::setTitle(const std::string& title){
        glfwSetWindowTitle(m_window, title.c_str());
    }

    void Window::setScreen(vec2 p, float s){
        float left   = p.x-(0.5f*s);
        float right  = p.x+(0.5f*s);
        float top    = p.y-(0.5f*s);
        float bottom = p.y+(0.5f*s);
        setScreen(left, right, top, bottom);
    }

    void Window::setScreen(float left, float right, float bottom, float top) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        float m_near = -1.0f;
        float m_far = 1.0f;
        glOrtho(left, right, bottom, top, m_near, m_far);
        float sx = right-left;
        float sy = top-bottom;

        const vec2 ssqTopLeft        = vec2(-sx*0.5f, -sy*0.5f);
        const vec2 ssqTopRight       = vec2(-sx*0.5f,  sy*0.5f);
        const vec2 ssqBottomLeft     = vec2( sx*0.5f, -sy*0.5f);
        const vec2 ssqBottomRight    = vec2( sx*0.5f,  sy*0.5f);
        const vec2 screenSizeQuad[] = {
            ssqBottomLeft,
            ssqBottomRight,
            ssqTopRight,
            ssqBottomLeft,
            ssqTopRight,
            ssqTopLeft
        };

        const vec2 spriteTopLeft        = vec2(-0.5f, -0.5f);
        const vec2 spriteTopRight       = vec2(-0.5f,  0.5f);
        const vec2 spriteBottomLeft     = vec2( 0.5f, -0.5f);
        const vec2 spriteBottomRight    = vec2( 0.5f,  0.5f);
        const vec2 spriteQuad[] = {
            spriteBottomLeft,
            spriteBottomRight,
            spriteTopRight,
            spriteBottomLeft,
            spriteTopRight,
            spriteTopLeft
        };

        static const vec2 textureCoords[] = {
            vec2(1,1),
            vec2(1,0),
            vec2(0,0),
            vec2(1,1),
            vec2(0,0),
            vec2(0,1)
        };

        // Set Screen size quad data
        glFinish();
        glBindVertexArray(m_ssqVao);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, m_ssqVbos[0]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, sizeof(screenSizeQuad), screenSizeQuad, GL_STATIC_DRAW);
        checkGLError();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, m_ssqVbos[1]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);
        checkGLError();
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        checkGLError();
        glBindVertexArray(0);
        checkGLError();
        glFinish();

        // Set Sprite data
        glFinish();
        glBindVertexArray(m_spriteVao);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, m_spriteVbos[0]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, sizeof(screenSizeQuad), spriteQuad, GL_STATIC_DRAW);
        checkGLError();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, m_spriteVbos[1]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);
        checkGLError();
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        checkGLError();
        glBindVertexArray(0);
        checkGLError();
        glFinish();

        m_projection = {
            2.0f/(m_right-m_left),  0.0f,                       0.0f,                   0.0f,
            0.0f,                   2.0f/(m_top-m_bottom),      0.0f,                   0.0f,
            0.0f,                   0.0f,                      -2.0f/(m_far-m_near),    0.0f,
            0.0f,                   0.0f,                       0.0f,                   1.0f
        };

        m_ssqShader->bind();
        m_ssqShader->setUniform("texture0", 0);
        m_ssqShader->setUniformm("M", m_projection);
        m_ssqShader->unbind();

        glFinish();
    }

    void Window::drawAxis(int thickColor, int thinColor, int thick, int thin) {
        std::vector<vec2> lines;
        int startX = (int)m_left;
        int maxX = (int)m_right;
        int startY = (int)m_bottom;
        int maxY = (int)m_top;
        // Thin lines
        for(int x = startX; x<maxX; ++x) {
            lines.push_back(vec2(x,startY-1));
            lines.push_back(vec2(x,maxY+1));
        }
        for(int y = startY; y<maxY; ++y) {
            lines.push_back(vec2(startX-1,y));
            lines.push_back(vec2(maxX+1,y));
        }
        drawLines(lines, thinColor, thin, false);

        // Thick lines
        lines.clear();
        lines.push_back(vec2(startX-1,0));
        lines.push_back(vec2(maxX+1, 0));
        lines.push_back(vec2(0,startY-1));
        lines.push_back(vec2(0,maxY+1));
        drawLines(lines, thickColor, thick, false);
    }

    void Window::drawPixels(const Grid& pixels) {
        std::vector<uint8_t> mapData;
        int mapWidth = 0;
        int mapHeight = 0;
        for(auto& row : pixels) {
            ++mapHeight;
            for(auto index : row) {
                assert(index >= 0 && index <m_palette.size());
                auto& color = m_palette[index];
                mapData.push_back(color.r);
                mapData.push_back(color.g);
                mapData.push_back(color.b);
                mapData.push_back(color.a);
                ++mapWidth;
            }
        }
        assert(mapWidth != 0);
        assert(mapHeight != 0);
        mapWidth /= mapHeight;

        Texture texture(mapWidth,mapHeight,4,&mapData[0]);
        drawScreenSizeQuad(&texture);
    }


    void Window::drawHeatMap(const HeatMap& pixels, const float valueMin, float valueMax) {
        std::vector<uint8_t> mapData;
        int mapWidth = 0;
        int mapHeight = 0;
        for(auto& row : pixels) {
            ++mapHeight;
            for(auto heat : row) {
                /*assert(index >= 0 && index <m_palette.size());*/
                auto color = heatToRGB(heat, valueMin, valueMax);
                mapData.push_back(color.r);
                mapData.push_back(color.g);
                mapData.push_back(color.b);
                mapData.push_back(color.a);
                ++mapWidth;
            }
        }
        assert(mapWidth != 0);
        assert(mapHeight != 0);
        mapWidth /= mapHeight;

        Texture texture(mapWidth,mapHeight,4,&mapData[0]);
        drawScreenSizeQuad(&texture);
    }

    void Window::drawLines(const std::vector<vec2>& lines, int color, size_t lineWidth, bool drawStrips) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glLineWidth(lineWidth);
        auto rgb = m_palette[color];
        glColor4f(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,rgb.a/255.0f);

        glBegin(drawStrips ? GL_LINE_STRIP : GL_LINES);
        for(size_t i=0; i<lines.size(); ++i){
           glVertex2f(lines[i].x, lines[i].y);
        }
        glEnd();
    }

    void Window::drawSprite(const std::vector< std::vector<float> >& transform, const Grid& pixels, const std::string& surfaceShader, const std::string& globals){
        drawSprite(transform, pixels, {}, surfaceShader, globals);
    }

    void Window::drawSprite(const std::vector< std::vector<float> >& transform, const Grid& pixels, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals){
        std::vector<uint8_t> mapData;
        int mapWidth = 0;
        int mapHeight = 0;
        for(auto& row : pixels) {
            ++mapHeight;
            for(auto index : row) {
                assert(index >= 0 && index <m_palette.size());
                auto& color = m_palette[index];
                mapData.push_back(color.r);
                mapData.push_back(color.g);
                mapData.push_back(color.b);
                mapData.push_back(color.a);
                ++mapWidth;
            }
        }
        if(pixels.size()==0){
            mapWidth = ++mapHeight = 1;
            mapData.push_back(0xff);
            mapData.push_back(0xff);
            mapData.push_back(0xff);
            mapData.push_back(0xff);
        }
        assert(mapWidth != 0);
        assert(mapHeight != 0);
        mapWidth /= mapHeight;

        Texture texture(mapWidth,mapHeight,4,&mapData[0]);

        std::vector<float> model;
        for(size_t y=0; y<transform.size(); ++y){
            for(size_t x=0; x<transform[y].size(); ++x){
                model.push_back(transform[y][x]);
            }
        }
        drawSprite(model, &texture, inputConstants, surfaceShader, globals);
    }

    void Window::drawFunction(const std::function<float (float)> &f, int color, size_t lineWidth){
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glLineWidth(lineWidth);
        auto rgb = m_palette[color];
        glColor4f(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,rgb.a/255.0f);

        glBegin(GL_LINE_STRIP);
        float dX = (m_right-m_left)/float(width);
        for(size_t i=0; i<width; i+=4) {
            float x = m_left + (i*dX);
            glVertex2f(x, f(x));
        }
        glEnd();
    }

    void Window::drawPoints(const std::vector<vec2>& points, int color, size_t pointSize) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glPointSize(pointSize);
        auto rgb = m_palette[color];
        glColor4f(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,rgb.a/255.0f);

        glBegin(GL_POINTS);
        for(size_t i=0; i<points.size(); ++i){
           glVertex2f(points[i].x, points[i].y);
        }
        glEnd();
    }


    void  Window::drawCircle(const vec2& pos, float r, int color, size_t lineWidth, size_t numSegments) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glLineWidth(lineWidth);
        auto rgb = m_palette[color];
        glColor4f(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,rgb.a/255.0f);

		glBegin(GL_LINE_LOOP);
		for (int i=0; i<numSegments; i++)
		{
			float theta = 2.0f * 3.1415926f * float(i) / float(numSegments);
			float x = r * cosf(theta);
			float y = r * sinf(theta);
			glVertex2f(pos.x + x, pos.y + y);
		}
        glEnd();
	}

    void Window::shade(const std::string& fragmentShaderMain, const std::string& globals){
        shade(std::vector<Constant>(), fragmentShaderMain, globals);
    }

    void Window::shade(const std::vector<Constant>& inputConstants, const std::string& fragmentShaderMain, const std::string& globals) {
        static const std::string vertexShaderSource =
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
            std::string("}");

        std::string inputUniforms;
        for(auto& c : inputConstants){
            auto& name = c.first;
            auto& val = c.second;
            if(val.size() == 1){
                inputUniforms += "uniform float "+name+";\n";
            } else if(val.size() == 2){
                inputUniforms += "uniform vec2 "+name+";\n";
            } else if(val.size() == 3){
                inputUniforms += "uniform vec3 "+name+";\n";
            } else if(val.size() == 4){
                inputUniforms += "uniform vec4 "+name+";\n";
            } else {
                assert(0); // Invalid value length. Must be between 1 to 4
            }
        }

        const std::string fragmentShaderSource =
            std::string("#version 330 core\n") +
            std::string("out vec4 FragColor;\n") +
            std::string("in float x;\n") +
            std::string("in float y;\n") +
            std::string("in float z;\n") +
            std::string("in float w;\n") +
            std::string("uniform vec2 max;\n") +
            std::string("uniform vec2 min;\n") +
            std::string("uniform vec2 size;\n") +
            inputUniforms +
            std::string("\n"+globals+"\n") +
            std::string("\nvoid main(){\n") +
            std::string("vec4 color;\n") +
            fragmentShaderMain +
            std::string("gl_FragData[0] = color;\n") +
            std::string("\n}\n");

        // Create and compile vertex shader
        Shader shader(vertexShaderSource, fragmentShaderSource);
        shader.bind();
        shader.setUniformm("M", m_projection);

        auto maxX = max(m_right,m_left);
        auto minX = min(m_right,m_left);
        auto maxY = max(m_top,m_bottom);
        auto minY = min(m_top,m_bottom);
        shader.setUniformv("leftBottom", {m_left, m_bottom});
        shader.setUniformv("rightTop", {m_right, m_top});
        shader.setUniform("min", minX, minY);
        shader.setUniform("max", maxX, maxY);
        shader.setUniform("size", maxX-minX, maxY-minY);
        for(auto& c : inputConstants){
            shader.setUniformv(c.first,c.second);
        }

        // Render
        m_shadeFbo->use([&]() {
            // Draw ssq only with positions
            glBindVertexArray(m_ssqVao);
            checkGLError();
            glEnableVertexAttribArray(0);
            checkGLError();
            glEnableVertexAttribArray(1);
            checkGLError();

            glDrawArrays(GL_TRIANGLES, 0, 6);
            checkGLError();

            // Unbind
            glDisableVertexAttribArray(1);
            checkGLError();
            glDisableVertexAttribArray(0);
            checkGLError();
            glBindVertexArray(0);
            checkGLError();
            glFinish();
            checkGLError();
        });

        shader.unbind();
    }


    void Window::playSound(const std::string& fileName){
        auto result = ma_engine_play_sound(&g_engine, fileName.c_str(), NULL);
        if (result != MA_SUCCESS) {
            throw std::runtime_error("Failed to play sound!");
            return;
        }

    }

    void Window::drawScreenSizeQuad(Texture* texture) {
        m_ssqShader->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

        // Draw 6 vertices as triangles
        glBindVertexArray(m_ssqVao);
        checkGLError();
        glEnableVertexAttribArray(0);
        checkGLError();
        glEnableVertexAttribArray(1);
        checkGLError();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        checkGLError();
        m_ssqShader->unbind();

        // Unbind
        glDisableVertexAttribArray(0);
        checkGLError();
        glDisableVertexAttribArray(1);
        checkGLError();
        glBindVertexArray(0);
        checkGLError();
        glFinish();
    }

    void Window::drawSprite(const std::vector<float>& M, Texture* texture, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals) {
        std::string inputUniforms;
        for(auto& c : inputConstants){
            auto& name = c.first;
            auto& val = c.second;
            if(val.size() == 1){
                inputUniforms += "uniform float "+name+";\n";
            } else if(val.size() == 2){
                inputUniforms += "uniform vec2 "+name+";\n";
            } else if(val.size() == 3){
                inputUniforms += "uniform vec3 "+name+";\n";
            } else if(val.size() == 4){
                inputUniforms += "uniform vec4 "+name+";\n";
            } else {
                assert(0); // Invalid value length. Must be between 1 to 4
            }
        }

        Shader spriteShader(SPRITE_VERT, SPRITE_FRAG(surfaceShader,inputUniforms,globals));
        spriteShader.bind();
        spriteShader.setUniformm("P", m_projection);
        spriteShader.setUniformm("M", M);
        spriteShader.setUniform("texture0", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getTextureId());
        for(auto& c : inputConstants){
            spriteShader.setUniformv(c.first,c.second);
        }

        // Draw 6 vertices as triangles
        glBindVertexArray(m_spriteVao);
        checkGLError();
        glEnableVertexAttribArray(0);
        checkGLError();
        glEnableVertexAttribArray(1);
        checkGLError();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        checkGLError();
        spriteShader.unbind();

        // Unbind
        glDisableVertexAttribArray(0);
        checkGLError();
        glDisableVertexAttribArray(1);
        checkGLError();
        glBindVertexArray(0);
        checkGLError();
        glFinish();
    }

}
