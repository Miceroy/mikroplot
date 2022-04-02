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

#include <functional>
#include <string>
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <memory>
#include <chrono>
#include <map>

struct GLFWwindow;

namespace mikroplot {

    namespace shaders {
    static const std::string heatmapGradient =
        "float m = sqrt(size.x*size.x+size.y*size.y)/sqrt(2);"
        "color = heatmap(x+y,-m,m);";
    static const std::string mandelbrot =
        "vec4 res = mandelbrot(x,y,100);"
        "if(res.x >= 1.0) discard;"
        "color = res;";
    }
    namespace shader_funcs {
    static const std::string heatmap =
        "vec4 heatmap(float value, float valueMin, float valueMax){"
        "    float hf = 240.0f * (1.0 - ((value-valueMin) / (valueMax-valueMin)));"
        "    int h = int(hf);"
        "    float up = float(h%40) / 40.0f;"
        "    float down = 1.0 - up;"
        "    switch(h/40) {"
        "        case 0: return vec4( 1.0, down, down, 1.0);"
        "        case 1: return vec4( 1.0,   up,  0.0, 1.0);"
        "        case 2: return vec4(down,  1.0,  0.0, 1.0);"
        "        case 3: return vec4( 0.0,  1.0,   up, 1.0);"
        "        case 4: return vec4( 0.0, down,  1.0, 1.0);"
        "        case 5: return vec4( 0.0,  0.0, down, 1.0);"
        "    }"
        "    return vec4(1);"
        "}";
    static const std::string mandelbrot =
        "vec4 mandelbrot(float real, float imag, float maxIters) {"
        "int iterations = 0;"
        "float constReal = real;"
        "float constImag = imag;"
        "while (iterations < maxIters) {"
        "    float oldReal = real;"
        "    float realPow2 = real*real;"
        "    float imagPow2 = imag*imag;"
        "    real = (realPow2 - imagPow2) + constReal;"
        "    imag = (2.0 * oldReal * imag) + constImag;"
        "    float dist = realPow2 + imagPow2;"
        "    if (dist > 4.0) break;"
        "    ++iterations;"
        "}"
        "vec2 n = 1.4f*normalize(vec2(real*real,imag*imag));"
        "return vec4(iterations/maxIters,n.x,n.y,0.9);}";
    }

    typedef std::vector< std::vector<int> > Grid;
    typedef std::vector< std::vector<float> > HeatMap;

    inline Grid gridNM(std::size_t N, std::size_t M,int value) {
        Grid res;
        for(std::size_t i=0; i<M; ++i) {
            res.push_back(std::vector<int>());
            for(std::size_t j=0; j<N; ++j) {
                res[i].push_back(value);
            }
        }
        return res;
    }

    inline Grid gridN(std::size_t N, int value) {
        return gridNM(N,N,value);
    }

    inline HeatMap heatMapNM(std::size_t N, std::size_t M, float value) {
        HeatMap res;
        for(std::size_t i=0; i<M; ++i) {
            res.push_back(std::vector<float>());
            for(std::size_t j=0; j<N; ++j) {
                res[i].push_back(value);
            }
        }
        return res;
    }

    inline HeatMap heatMapN(std::size_t N, float value) {
        return heatMapNM(N,N,value);
    }

    ///
    /// \brief Red - Green - Blue - Alpha -color.
    struct RGBA {
        RGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 0xff)
            : r(_r), g(_g), b(_b), a(_a) {
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    ///
    /// \brief The vec2 struct
    struct vec2 {
        float x;
        float y;

		vec2(float xvalue, float yvalue) : x(xvalue), y(yvalue) {}
		vec2(const std::vector<float>& v) : x(v[0]), y(v[1]) { assert(v.size() == 2); }
    };

	// Specify default-palette:
	const std::vector<RGBA> DEFAULT_PALETTE = {
		RGBA(0x00,0x00,0x00,0x00),	// 0  = Transparent black
		RGBA(0x00,0x00,0x00),		// 1  = Opaque black
		RGBA(0xFF,0xFF,0xFF),		// 2  = white
		RGBA(50,50,50),				// 3  = Gray1
		RGBA(100,100,100),			// 4  = Gray2
		RGBA(150,150,150),			// 5  = Gray3
		RGBA(200,200,200),			// 6  = Gray4
		RGBA(250,250,250),			// 7  = Gray5
		RGBA(0xFF,0x00,0x00),		// 8  = Red
		RGBA(0x00,0xFF,0x00),		// 9  = Green
		RGBA(0x00,0x00,0xFF),		// 10 = Blue
		RGBA(0x00,0xFF,0xFF),		// 11 = Cyan
		RGBA(0xFF,0x00,0xFF),		// 12 = Magenta
		RGBA(0xFF,0xFF,0x00),		// 13 = Yellow
		RGBA(127,127,127),		    // 14 = Gray
		RGBA(127,0x00,0x00),		// 15 = Dark Red
		RGBA(0x00,127,0x00),		// 16 = Dark Green 
		RGBA(0x00,0x00,127),		// 17 = Dark Blue
		RGBA(0x00,127,127),			// 18 = Dark Cyan
		RGBA(127,0x00,127),			// 19 = Dark Magenta
		RGBA(127,127,0x00),			// 20 = Dark Yellow
	};

    const int DEFAULT_COLOR = 11;

    typedef std::pair<std::string,std::vector<float> >Constant;

    class FrameBuffer;
    class Texture;
    class Shader;


	class Timer {
	public:
		Timer()
			: start(std::chrono::steady_clock::now()) {
		}

		float getDeltaTime() {
			auto end = std::chrono::steady_clock::now();
			float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(end - start).count();
			start = end;
			return deltaTime;
		}

	private:
		Timer(const Timer&) = delete;
		std::chrono::time_point<std::chrono::steady_clock> start;
	};

    namespace mesh {
        struct Mesh {
            unsigned int  vao;
            unsigned int  vbos[2];

            void setVBOData(int index, const std::vector<vec2>& data);
            void release();
        };
    }

    class Window {
    public:
        explicit Window(int sizeX, int sizeY, const std::string& title, const std::vector<RGBA>& palette = DEFAULT_PALETTE, int clearColor = 3);
        ~Window();

        int getKeyState(int keyCode);
        int getKeyPressed(int keyCode);
        int getKeyReleased(int keyCode);

        int update();
        int run();
        void screenshot(const std::string filename);
        bool shouldClose();

        // Screen settings
        void setTitle(const std::string& title);
        // Ortho projection
        void setScreen(vec2 position, float size);
        // Ortho projection
        void setScreen(float left, float right, float bottom, float top);
        // Sets clear color
        void setClearColor(int color=4) { m_clearColor = color; }

        // Draw functions.
        void drawAxis(int thickColor=6, int thinColor=5, int thick=3, int thin=1);
        void drawLines(const std::vector<vec2>& lines, int color=DEFAULT_COLOR, std::size_t lineWidth = 2, bool drawStrips=true);
        void drawPoints(const std::vector<vec2>& points, int color=DEFAULT_COLOR, std::size_t pointSize = 2);
        void drawCircle(const vec2& position, float radius, int color=DEFAULT_COLOR, std::size_t lineWidth = 2, std::size_t numSegments = 50);

        void drawSprite(const std::vector< std::vector<float> >& transform, const Grid& pixels, const std::string& surfaceShader="", const std::string& globals="");
        void drawSprite(const std::vector< std::vector<float> >& transform, const Grid& pixels, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals="");
        void drawFunction(const std::function<float(float)>& f, int color=DEFAULT_COLOR, std::size_t lineWidth = 2);
        void drawPixels(const Grid& pixels);
        void drawHeatMap(const HeatMap& pixels, const float valueMin=0.0f, float valueMax=1.0f);

        void shade(const std::string& fragmentShader, const std::string& globals="");
        void shade(const std::vector<Constant>& inputConstants, const std::string& fragmentShader, const std::string& globals="");

        void playSound(const std::string& fileName);

        template<typename RenderFunc>
        auto runClips(RenderFunc renderClip, int numScenes, float timeToShowSingleClip, int screenshotTime = -1) {
            // Screenshot functionality
			Timer frameTimer;
            int screenshots = -1;
            auto checkScreenshots = [&](int time, int clip){
                if(time == screenshotTime && clip > screenshots){
                    screenshots = clip;
                    printf("Taking screenshot from scene %d\n", clip);
                    screenshot("demo_"+ std::to_string(clip) + ".png");
                }
            };
            float time = 0;
            while(shouldClose() == false) {
                int clip = int(time/timeToShowSingleClip)%numScenes;
                renderClip(*this, clip, time);
                // Swap frame
                update();
                checkScreenshots(uint32_t(time)%uint32_t(timeToShowSingleClip), clip);
                time += frameTimer.getDeltaTime();
            }
        };
    private:
        Window() = delete;
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        void drawScreenSizeQuad(Texture* texture);
        void drawSprite(const std::vector<float>& M, Texture* texture, const std::vector<Constant>& inputConstants, const std::string& surfaceShader, const std::string& globals);
        void takeScreenshot(const std::string filename);

        int                       m_clearColor;
        const int                       m_width;
        const int                       m_height;
        const std::vector<RGBA>         m_palette;
        GLFWwindow*                     m_window;

        std::vector<float> m_projection;
        float                           m_left;
        float                           m_right;
        float                           m_bottom;
        float                           m_top;

        std::unique_ptr<FrameBuffer>    m_shadeFbo;        
        std::unique_ptr<Shader>         m_ssqShader;
        std::unique_ptr<mesh::Mesh>     m_ssq;
        std::unique_ptr<mesh::Mesh>     m_sprite;
        std::string                     m_screenshotFileName;

        std::map<int, bool>         m_prevKeys;
        std::map<int, bool>         m_curKeys;
    };

    static RGBA heatToRGB(float value, const float valueMin, float valueMax) {
        assert( valueMin < valueMax );
        assert( value >= valueMin );
        assert( value <= valueMax );

        unsigned int h = (unsigned int)( (1.0f - ((value-valueMin) / (valueMax-valueMin)) ) * 240.0f);
        float g = float(h%40) / 40.0f;
        float inc = 255.0f;
        unsigned int up =int( g*inc );
        unsigned int down = 0xFF - up;

        switch(int(h/40)) {
            case 0: return RGBA(0xFF, down, down);
            case 1: return RGBA(0xFF,   up, 0x00);
            case 2: return RGBA(down, 0xFF, 0x00);
            case 3: return RGBA(0x00, 0xFF,   up);
            case 4: return RGBA(0x00, down, 0xFF);
            case 5: return RGBA(0x00, 0x00, down);
        }
        return RGBA(255,255,255);
    }

    enum KeyCodes {
        // The unknown key
        KEY_UNKNOWN         = -1,
        // Printable keys
        KEY_SPACE      = 32,
        KEY_APOSTROPHE = 39,  // '
        KEY_COMMA      = 44,  // ,
        KEY_MINUS      = 45,  // -
        KEY_PERIOD     = 46,  // .
        KEY_SLASH      = 47,  // /
        KEY_0          = 48,
        KEY_1          = 49,
        KEY_2          = 50,
        KEY_3          = 51,
        KEY_4          = 52,
        KEY_5          = 53,
        KEY_6          = 54,
        KEY_7          = 55,
        KEY_8          = 56,
        KEY_9          = 57,
        KEY_SEMICOLON  = 59,  // ;
        KEY_EQUAL      = 61,  // =
        KEY_A          = 65,
        KEY_B          = 66,
        KEY_C          = 67,
        KEY_D          = 68,
        KEY_E          = 69,
        KEY_F          = 70,
        KEY_G          = 71,
        KEY_H          = 72,
        KEY_I          = 73,
        KEY_J          = 74,
        KEY_K          = 75,
        KEY_L          = 76,
        KEY_M          = 77,
        KEY_N          = 78,
        KEY_O          = 79,
        KEY_P          = 80,
        KEY_Q          = 81,
        KEY_R          = 82,
        KEY_S          = 83,
        KEY_T          = 84,
        KEY_U          = 85,
        KEY_V          = 86,
        KEY_W          = 87,
        KEY_X          = 88,
        KEY_Y          = 89,
        KEY_Z          = 90,
        KEY_LEFT_BRACKET = 91,  // [
        KEY_BACKSLASH    = 92,
        KEY_RIGHT_BRACKET= 93,  // ]
        KEY_GRAVE_ACCENT = 96, //
        KEY_WORLD_1      = 161, // non-US #1
        KEY_WORLD_2      = 162, // non-US #2
        // Function keys
        KEY_ESCAPE            = 256,
        KEY_ENTER             = 257,
        KEY_TAB               = 258,
        KEY_BACKSPACE         = 259,
        KEY_INSERT            = 260,
        KEY_DELETE            = 261,
        KEY_RIGHT             = 262,
        KEY_LEFT              = 263,
        KEY_DOWN              = 264,
        KEY_UP                = 265,
        KEY_PAGE_UP           = 266,
        KEY_PAGE_DOWN         = 267,
        KEY_HOME              = 268,
        KEY_END               = 269,
        KEY_CAPS_LOCK         = 280,
        KEY_SCROLL_LOCK       = 281,
        KEY_NUM_LOCK          = 282,
        KEY_PRINT_SCREEN      = 283,
        KEY_PAUSE             = 284,
        KEY_F1                = 290,
        KEY_F2                = 291,
        KEY_F3                = 292,
        KEY_F4                = 293,
        KEY_F5                = 294,
        KEY_F6                = 295,
        KEY_F7                = 296,
        KEY_F8                = 297,
        KEY_F9                = 298,
        KEY_F10               = 299,
        KEY_F11               = 300,
        KEY_F12               = 301,
        KEY_F13               = 302,
        KEY_F14               = 303,
        KEY_F15               = 304,
        KEY_F16               = 305,
        KEY_F17               = 306,
        KEY_F18               = 307,
        KEY_F19               = 308,
        KEY_F20               = 309,
        KEY_F21               = 310,
        KEY_F22               = 311,
        KEY_F23               = 312,
        KEY_F24               = 313,
        KEY_F25               = 314,
        KEY_KP_0              = 320,
        KEY_KP_1              = 321,
        KEY_KP_2              = 322,
        KEY_KP_3              = 323,
        KEY_KP_4              = 324,
        KEY_KP_5              = 325,
        KEY_KP_6              = 326,
        KEY_KP_7              = 327,
        KEY_KP_8              = 328,
        KEY_KP_9              = 329,
        KEY_KP_DECIMAL        = 330,
        KEY_KP_DIVIDE         = 331,
        KEY_KP_MULTIPLY       = 332,
        KEY_KP_SUBTRACT       = 333,
        KEY_KP_ADD            = 334,
        KEY_KP_ENTER          = 335,
        KEY_KP_EQUAL          = 336,
        KEY_LEFT_SHIFT        = 340,
        KEY_LEFT_CONTROL      = 341,
        KEY_LEFT_ALT          = 342,
        KEY_LEFT_SUPER        = 343,
        KEY_RIGHT_SHIFT       = 344,
        KEY_RIGHT_CONTROL     = 345,
        KEY_RIGHT_ALT         = 346,
        KEY_RIGHT_SUPER       = 347,
        KEY_MENU              = 348,
        KEY_LAST              = KEY_MENU
    };
} // End - mmikroplot

