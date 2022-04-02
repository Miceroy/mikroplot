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
#include <mikroplot/graphics.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// If ytou want to take screenshots, you must speciy following:
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;

namespace mikroplot {

    // Class for static initialization of glfw and miniaudio
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

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

            auto result = ma_engine_init(NULL, &audioEngine);
            if (result != MA_SUCCESS) {
                throw std::runtime_error("Failed to initialize audio engine!");
                return;
            }
        }
        ~StaticInit() {
            ma_engine_uninit(&audioEngine);

            // Terminate glfw
            glfwTerminate();
        }

        ma_engine audioEngine;
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
    {
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

        glfwSetWindowUserPointer(m_window,this);
        // Specify the key callback as c++-lambda to glfw
        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            // Close window if escape is pressed by the user.
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            Window* pThis = (Window*)glfwGetWindowUserPointer(window);

            if(action == GLFW_PRESS){
                pThis->m_curKeys[key] = true;
            }
            if(action == GLFW_RELEASE){
                pThis->m_curKeys[key] = false;
            }
        });

        m_ssqShader = std::make_unique<Shader>(shaders::projectionVSSource(), shaders::textureFSSource("","",""));

        glEnable(GL_BLEND);
        checkGLError();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        checkGLError();

		glEnable(GL_POINT_SMOOTH);
        checkGLError();
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        checkGLError();

        // Create sprite and screen size quad meshes
        m_sprite = quad::create();
        m_ssq = quad::create();

        // Query the size of the framebuffer (window content) from glfw.
        int screenWidth, screenHeight;
        glfwGetFramebufferSize(m_window, &screenWidth, &screenHeight);
        glViewport(0, 0, screenWidth, screenHeight);
        setScreen(0,screenWidth,0,screenHeight);

        // Create FBOs
        m_shadeFbo = std::make_unique<FrameBuffer>();
        m_shadeFbo->addColorTexture(0, std::make_shared<Texture>(screenWidth, screenHeight, false));
    }

    Window::~Window() {
        m_shadeFbo = 0;

        m_sprite.release();
        m_ssq.release();

        // Destroy window
        glfwDestroyWindow(m_window);
    }


    bool keyState(const std::map<int, bool>& keyMap, int keyCode) {
        auto it = keyMap.find(keyCode);
        if(it == keyMap.end()){
            return false;
        }
        return it->second;
    }

    int Window::getKeyState(int keyCode) {
        if(keyState(m_curKeys,keyCode) ){
            return true;
        }
        return false;
    }

    int Window::getKeyPressed(int keyCode){
        return keyState(m_curKeys,keyCode) && !keyState(m_prevKeys,keyCode);
    }

    int Window::getKeyReleased(int keyCode){
        return !keyState(m_curKeys,keyCode) && keyState(m_prevKeys,keyCode);
    };

    int Window::update() {
        if (shouldClose()) {
            return -1;
        }
        // Set current context
        glfwMakeContextCurrent(m_window);
        drawScreenSizeQuad(m_shadeFbo->getTexture(0).get());
        glfwSwapBuffers(m_window);
        glFinish();

        if(m_screenshotFileName.length()>0){
            takeScreenshot(m_screenshotFileName);
            m_screenshotFileName = "";
        }

        auto rgb = m_palette[m_clearColor];
        glClearColor(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,1.0);
        //glClearColor(1.0f,1.0f,1.0f,1.0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        m_shadeFbo->use([](){
            //glClearColor(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,0.0);
            glClearColor(0.0f,0.0f,0.0f,0.0);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        });
        m_prevKeys = m_curKeys;
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
        if(m_left==left && m_right==right && m_bottom==bottom && m_top==top){
            return;
        }
        float m_near = -1.0f;
        float m_far = 1.0f;
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(m_left, m_right, m_bottom, m_top, m_near, m_far);

        float sx = right-left;
        float sy = top-bottom;

        const vec2 ssqTopLeft        = vec2(-sx*0.5f, -sy*0.5f);
        const vec2 ssqTopRight       = vec2(-sx*0.5f,  sy*0.5f);
        const vec2 ssqBottomLeft     = vec2( sx*0.5f, -sy*0.5f);
        const vec2 ssqBottomRight    = vec2( sx*0.5f,  sy*0.5f);
        const std::vector<vec2> screenSizeQuad({
            ssqBottomLeft,
            ssqBottomRight,
            ssqTopRight,
            ssqBottomLeft,
            ssqTopRight,
            ssqTopLeft
        });

        quad::setPositions(*m_ssq, screenSizeQuad);

        m_projection = {
            2.0f/(m_right-m_left),  0.0f,                       0.0f,                   0.0f,
            0.0f,                   2.0f/(m_top-m_bottom),      0.0f,                   0.0f,
            0.0f,                   0.0f,                      -2.0f/(m_far-m_near),    0.0f,
            0.0f,                   0.0f,                       0.0f,                   1.0f
        };

        m_ssqShader->use([&](){
            m_ssqShader->setUniform("texture0", 0);
            m_ssqShader->setUniformm("P", m_projection);
        });
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

        std::vector<float> matModel;
        for(size_t y=0; y<transform.size(); ++y){
            for(size_t x=0; x<transform[y].size(); ++x){
                matModel.push_back(transform[y][x]);
            }
        }
        drawSprite(matModel, &texture, inputConstants, surfaceShader, globals);
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
        //glFinish();
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
        //glFinish();
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
        //glFinish();
	}

    void Window::shade(const std::string& fragmentShaderMain, const std::string& globals){
        shade(std::vector<Constant>(), fragmentShaderMain, globals);
    }

    void Window::shade(const std::vector<Constant>& inputConstants, const std::string& fragmentShaderMain, const std::string& globals) {
        Shader shadeShader(shaders::shadeVSSource(), shaders::shadeFSSource(shaders::constants(inputConstants), globals, fragmentShaderMain));
        m_shadeFbo->use([&](){
            shadeShader.use([&](){
                shadeShader.setUniformm("M", m_projection);

                auto maxX = max(m_right, m_left);
                auto minX = min(m_right, m_left);
                auto maxY = max(m_top, m_bottom);
                auto minY = min(m_top, m_bottom);
                shadeShader.setUniformv("leftBottom", {m_left, m_bottom});
                shadeShader.setUniformv("rightTop", {m_right, m_top});
                shadeShader.setUniform("min", minX, minY);
                shadeShader.setUniform("max", maxX, maxY);
                shadeShader.setUniform("size", maxX-minX, maxY-minY);
                for(auto& c : inputConstants){
                    shadeShader.setUniformv(c.first, c.second);
                }

                // Render screen size quad
                quad::render(*m_ssq);
            });
        });
        //glFinish();
    }


    void Window::playSound(const std::string& fileName){
        auto result = ma_engine_play_sound(&init.audioEngine, fileName.c_str(), NULL);
        if (result != MA_SUCCESS) {
            throw std::runtime_error("Failed to play sound!");
            return;
        }

    }

    void Window::drawScreenSizeQuad(Texture* texture) {
        m_ssqShader->use([&]() {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture->getTextureId());
            // Render screen size quad
            quad::render(*m_ssq);
        });
    }

    void Window::drawSprite(const std::vector<float>& M, Texture* texture, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals) {
        Shader spriteShader(shaders::modelProjectionVSSource(), shaders::textureFSSource(shaders::constants(inputConstants), globals, surfaceShader));
        spriteShader.use([&]() {
            spriteShader.setUniformm("P", m_projection);
            spriteShader.setUniformm("M", M);
            spriteShader.setUniform("texture0", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture->getTextureId());
            for(auto& c : inputConstants){
                spriteShader.setUniformv(c.first,c.second);
            }
            // Draw sprite
            quad::render(*m_sprite);
        });
    }

}
