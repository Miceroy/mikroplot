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
#include <random>


int main() {
    using namespace mikroplot;
	// Heatmap values: 0.0 ... 2.0
	HeatMap heatMap = heatMapN(128, 0.0f);
	for (std::size_t y = 0; y < heatMap.size(); ++y) {
		for (std::size_t x = 0; x < heatMap[y].size(); ++x) {
			float h = heatMap.size();
			float w = heatMap[y].size();
			float valY = float(y) / h;
			float valX = float(x) / w;
			heatMap[y][x] = valX + valY;
			assert(heatMap[y][x] >= 0.0f);
			assert(heatMap[y][x] <= 2.0f);
		}
	}

	// All Pixels.
	Grid pixels = gridNM(16, 16, 0);
	for (std::size_t i = 0; i < DEFAULT_PALETTE.size(); ++i) {
		std::size_t x = i % 16;
		std::size_t y = i / 16;
		pixels[y][x] = i;
	}

    auto renderScene = [&](Window& window, int scene, float time) {
        if(scene==7){
            window.setTitle("Draw coordinate axis (-10, 10)");
            window.setScreen(-10,10,-10,10);
            window.setClearColor(5);
            window.drawAxis(7,6);
        } else if(scene==1) {
            window.setTitle("Draw basic primitives (-3, 3)");
            window.setScreen(-3,3,-3,3);
            window.setClearColor();
            window.drawAxis();
            window.drawPoints({{1,1}, {-1,1}, {-1,-1}, {1,-1}}, 17, 7);
            window.drawPoints({{1.5,1.5}, {1.2,1.6}, {1.4,1.3}, {1.3,1.2}}, 11, 7);
            window.drawLines({{-5,3}, {-3,2}, {-1,-1},{1,-2}, {3,1}, {4,0}, {5,1}}, DEFAULT_PALETTE.size()-1, 3);
            window.drawCircle({-1,-1},1.5f);
        } else if(scene == 2) {
            window.setTitle("Draw functions (-5, 5)");
            window.setScreen(-5,5,-5,5);
            window.setClearColor();
            window.drawAxis();
            window.drawFunction([](float x){
                return x*x - 2;
            });
            window.drawFunction([](float x){
                x = 0.5f*x;
                return -0.8f*x*x*x + 1.3f*x*x;
            }, 15, 4);
        } else if(scene==3) {
            window.setTitle("Draw circle shading (-3, 3)");
            window.setScreen(-3,3,-3,3);
            window.setClearColor();
            window.drawAxis();
            window.shade({Constant("t",{time})},
                "if(length(vec2(x,y)) > 1.0) discard;"
                "color = vec4(sin(2*t),sin(0.333*t),sin(t),1.0);"
            );
        } else if(scene == 4) {
            window.setTitle("Draw pixel grid (-8, 8)");
            window.setClearColor();
            window.setScreen(-8,8,-8,8);
            window.drawAxis(5,5,1,1);
            window.drawPixels(pixels);
        } else if(scene == 5) {
            window.setTitle("Draw heatmap (-8, 8, -8, 8)");
            window.setClearColor();
            window.setScreen(-8,8,-8,8);
            window.drawAxis(5,5,1,1);
            window.drawHeatMap(heatMap, 0.0f, 2.0f);
        } else if(scene == 6) {
            window.setTitle("Shade heatmap (-8, 8, -8, 8)");
            window.setClearColor();
            window.setScreen(-8,8,-8,8);
            window.drawAxis();
            window.shade(shaders::heatmapGradient, shader_funcs::heatmap);
        }  else if(scene == 0) {
            window.setTitle("Shade sphere r=1.5 to position (2.0,1.0)");
            window.setClearColor();
            window.setScreen(-8,8,-8,8);
            window.drawAxis();
            window.shade( {
                Constant("pos",{2.0f, 1.0f}),
                Constant("r",{1.5f})
                },
                "vec2 delta = vec2(x,y)-pos;"
                "if(length(delta) > r) {"
                "  discard;"
                "} else {"
                "  vec2 normal = normalize(delta);"
                "  color = vec4(abs(delta.x), abs(delta.y), 0.1f, 0.9f);"
                "}"
            );
        } else if(scene==8){
            window.setTitle("Mandelbrot");
            window.setClearColor();
            window.setScreen(-2,1,-1.5,1.5);
            window.drawAxis();
            window.shade(shaders::mandelbrot, shader_funcs::mandelbrot);
        } else if(scene==9){
            window.setTitle("Draw Sprites");
            window.setClearColor();
            window.setScreen({0.0f,0.0f}, 10.0f);
            window.drawAxis();
            std::vector< std::vector<float> > transform = {
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {1, 2, 0, 1}
            };
            Grid pixels = { {10,11}, {12,13}};
            window.drawSprite(transform,pixels);
            transform[3][0] = -1;
            transform[3][1] = -2;
            pixels = { {14,15}, {16,17}};
            window.drawSprite(transform,pixels,{Constant("t",{time})},"color *= vec4(abs(sin(t)), abs(sin(t)), abs(sin(t)), 1.0f);");
            transform[3][0] = -3;
            transform[3][1] = -1;
            pixels = { {18,19}, {20,20}};
            window.drawSprite(transform,pixels);
            transform[3][0] = -3;
            transform[3][1] = 2;
            window.drawSprite(transform,pixels,{Constant("t",{time})},"color = vec4(max(color.r,abs(sin(10*t))), color.g, color.b, color.a);");
        }
        if(window.getKeyPressed(KeyCodes::KEY_SPACE)){
            try {
                printf("Playing bell.wav... ");
                window.playSound("bell.wav");
                printf("... done!\n");
            }  catch (const std::exception& e) {
                printf("\nFailed to play bell.wav!\n");
            }
        }
    };

    // Create window and run it.
    Window window(512, 512, "Mikroplot demo");

    window.runClips(renderScene, 10, 5.0f, 2);

    return 0;
}
