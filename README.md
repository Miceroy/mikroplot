# MikRoPlot
MikRoPlot - C++ Plotting made easy.

# Building
```
mkdir build 
cd build
cmake ../
cmake --build .
```

# Minimal example

main.cpp:
```
#include <mikroplot/window.h>
int main() {
  using namespace mikroplot;
  Window window(800, 800, "Simple Draw demo");
  while(window.shouldClose() == false) {
    window.setScreen(-5,5,-5,5);
    window.drawAxis();
    window.drawFunction([&](float x)->float{
      return x*x*x;
    });
    
    window.update();
  }
  return 0;
}

```

CMakeLists.txt:

```
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(CXX)

# Enable following line and regenerate cmake if you want to see mikroplot examples
# option(MIKROPLOT_BUILD_EXAMPLES "" ON )

add_subdirectory("ext/mikroplot")

project(my_project)

add_executable(mikroplot_test main.cpp)
target_link_libraries(mikroplot_test PUBLIC mikroplot)

```

# Examples

See file: [main_mikroplot_demo.cpp](https://github.com/Miceroy/mikroplot/blob/main/examples/main_mikroplot_demo.cpp) for more examples how to draw different primitives with mikroplot.

# License 

[MIT License](LICENSE)



