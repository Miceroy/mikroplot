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

// If you want to take screenshots, you must speciy following:
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// If you want to load textures, you must speciy following:
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>



using namespace std;

namespace mikroplot {
	namespace mesh {
	void Mesh::setVBOData(int index, const std::vector<float>& data, size_t numComponents) {
		glBindVertexArray(vao);
		checkGLError();

		glBindBuffer(GL_ARRAY_BUFFER, vbos[index]);
		checkGLError();
		glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(data[0]), &data[0], GL_STATIC_DRAW);
		checkGLError();
		glVertexAttribPointer(index, numComponents, GL_FLOAT, GL_FALSE, numComponents * sizeof(float), (void*)0);
		checkGLError();
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		checkGLError();

		glBindVertexArray(0);
		checkGLError();
	}

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

#if defined(__APPLE__)
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
			//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


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

	std::unique_ptr<StaticInit> init;

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
		if(!init) init = std::make_unique<StaticInit>();
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

		glfwSetWindowUserPointer(m_window, this);
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
		m_ssqShader = 0;
		m_ssq = 0;
		m_sprite = 0;
		// Destroy window
		glfwDestroyWindow(m_window);
		m_window = 0;
	}


	std::shared_ptr<mikroplot::Texture> Window::loadTexture(const std::string& filename) {
		auto it = m_textures.find(filename);
		if(it != m_textures.end()) {
			return it->second;
		}
		int width,height,bpp;
		width = height = bpp = 0;
		uint8_t *data = stbi_load(filename.c_str(), &width, &height, &bpp, 0);
		if(data==0) {
			return 0;
		}
		auto res = m_textures[filename] = std::make_shared<mikroplot::Texture>(width, height, bpp, data);
		stbi_image_free(data);
		return res;
	}

	bool keyState(const std::map<int, bool>& keyMap, int keyCode) {
		auto it = keyMap.find(keyCode);
		if(it == keyMap.end()){
			return false;
		}
		return it->second;
	}

	int Window::getKeyState(int keyCode) const {
		if(keyState(m_curKeys,keyCode) ){
			return true;
		}
		return false;
	}

	int Window::getKeyPressed(int keyCode) const {
		return keyState(m_curKeys,keyCode) && !keyState(m_prevKeys,keyCode);
	}

	int Window::getKeyReleased(int keyCode) const {
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
		glfwMakeContextCurrent(m_window);
		glfwPollEvents();
		return m_window == 0 || glfwWindowShouldClose(m_window);
	}


	void Window::setTitle(const std::string& title){
		glfwSetWindowTitle(m_window, title.c_str());
	}

	void Window::setScreenSize(int width, int height){
		m_width = width;
		m_height = height;
		glfwSetWindowSize(m_window,m_width,m_height);
	}

	void Window::setScreenPosition(int x, int y){
		glfwSetWindowPos(m_window,x,y);
	}

	int Window::getScreenPositionX() const {
		int x;
		int y;
		glfwGetWindowPos(m_window,&x,&y);
		return x;
	}

	int Window::getScreenPositionY() const {
		int x;
		int y;
		glfwGetWindowPos(m_window,&x,&y);
		return y;
	}


	std::vector<float> Window::setScreen(vec2 pos, vec2 size) {
		float left   = pos.x-(0.5f*size.x);
		float right  = pos.x+(0.5f*size.x);
		float top    = pos.y-(0.5f*size.y);
		float bottom = pos.y+(0.5f*size.y);
		return setScreen(left, right, top, bottom);
	}

	std::vector<float> Window::setScreen(float left, float right, float bottom, float top) {
		glfwMakeContextCurrent(m_window);
		if(m_left==left && m_right==right && m_bottom==bottom && m_top==top){
			return m_projection;
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
			m_ssqShader->setUniformm("P", &m_projection[0]);
		});
		return m_projection;
	}

	void Window::drawAxis(int thickColor, int thinColor, int thick, int thin) {
		glfwMakeContextCurrent(m_window);
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
		glfwMakeContextCurrent(m_window);
		std::vector<uint8_t> mapData;
		int mapWidth = 0;
		int mapHeight = 0;
		for(auto& row : pixels) {
			++mapHeight;
			for(auto index : row) {
				index = index % m_palette.size();
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

	void Window::drawRGB(const RGBAMap& map) {
		glfwMakeContextCurrent(m_window);
		std::vector<uint8_t> mapData;
		int mapWidth = 0;
		int mapHeight = 0;
		for(auto& row : map) {
			++mapHeight;
			for(auto color : row) {
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

	void Window::drawRGB(int width, int height, std::vector<unsigned char> rgb) {
		glfwMakeContextCurrent(m_window);
		assert(width*height*3 == rgb.size());
		Texture texture(width,height,3,&rgb[0]);
		drawScreenSizeQuad(&texture);
	}


	void Window::drawHeatMap(const HeatMap& pixels, const float valueMin, float valueMax) {
		glfwMakeContextCurrent(m_window);
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
		glfwMakeContextCurrent(m_window);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLineWidth(lineWidth);
		auto rgb = m_palette[color];
		glColor4f(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,rgb.a/255.0f);

		glBegin(drawStrips ? GL_LINE_STRIP : GL_LINES);
		for(size_t i=0; i<lines.size(); ++i){
		   glVertex2f(lines[i].x+m_offset[0], lines[i].y+m_offset[1]);
		}
		glEnd();
	}

	void Window::drawSprite(const std::vector< std::vector<float> >& transform, const Grid& pixels, const std::string& surfaceShader, const std::string& globals){
		drawSprite(transform, pixels, {}, surfaceShader, globals);
	}

	void Window::drawSprite(const std::vector< std::vector<float> >& transform, const Grid& pixels, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals){
		glfwMakeContextCurrent(m_window);
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


	void Window::drawSprite(const std::vector< std::vector<float> >& transform, const mikroplot::Texture* texture, const std::string& surfaceShader, const std::string& globals){
		std::vector<float> matModel;
		for(size_t y=0; y<transform.size(); ++y){
			for(size_t x=0; x<transform[y].size(); ++x){
				matModel.push_back(transform[y][x]);
			}
		}
		drawSprite(matModel, texture, {}, surfaceShader, globals);
	}

	void Window::drawFunction(const std::function<float (float)> &f, int color, size_t lineWidth){
		glfwMakeContextCurrent(m_window);
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
			glVertex2f(x+m_offset[0], f(x)+m_offset[1]);
		}
		glEnd();
		//glFinish();
	}

	void Window::drawPoints(const std::vector<vec2>& points, int color, size_t pointSize) {
		glfwMakeContextCurrent(m_window);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPointSize(pointSize);
		auto rgb = m_palette[color];
		glColor4f(rgb.r/255.0f,rgb.g/255.0f,rgb.b/255.0f,rgb.a/255.0f);

		glBegin(GL_POINTS);
		for(size_t i=0; i<points.size(); ++i){
		   glVertex2f(points[i].x+m_offset[0], points[i].y+m_offset[1]);
		}
		glEnd();
		//glFinish();
	}


	void  Window::drawCircle(const vec2& pos, float r, int color, size_t lineWidth, size_t numSegments) {
		glfwMakeContextCurrent(m_window);
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
			glVertex2f(pos.x + x + m_offset[0], pos.y + y + m_offset[1]);
		}
		glEnd();
		//glFinish();
	}

	void Window::shade(const std::string& fragmentShaderMain, const std::string& globals){
		shade(std::vector<Constant>(), fragmentShaderMain, globals);
	}

	void Window::shade(const std::vector<Constant>& inputConstants, const std::string& fragmentShaderMain, const std::string& globals) {
		glfwMakeContextCurrent(m_window);
		Shader shadeShader(shaders::shadeVSSource(), shaders::shadeFSSource(shaders::constants(inputConstants), globals, fragmentShaderMain));
		m_shadeFbo->use([&](){
			shadeShader.use([&](){
				shadeShader.setUniformm("M", &m_projection[0]);

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
		auto result = ma_engine_play_sound(&init->audioEngine, fileName.c_str(), NULL);
		if (result != MA_SUCCESS) {
			throw std::runtime_error("Failed to play sound!");
			return;
		}

	}

	void Window::drawScreenSizeQuad(Texture* texture) {
		glfwMakeContextCurrent(m_window);
		m_ssqShader->use([&]() {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->getTextureId());
			// Render screen size quad
			quad::render(*m_ssq);
		});
	}

	void Window::drawSprite(const std::vector<float>& M, const Texture* texture, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals) {
		glfwMakeContextCurrent(m_window);
		Shader spriteShader(shaders::modelProjectionVSSource(), shaders::textureFSSource(shaders::constants(inputConstants), globals, surfaceShader));
		spriteShader.use([&]() {
			spriteShader.setUniformm("P", &m_projection[0]);
			spriteShader.setUniformm("M", &M[0]);
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
