cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 11)

project(ps1)

include_directories(lib/ASSIMP)
include_directories(lib/ASSIMP/include)
include_directories(lib/ASSIMP/include/assimp)
include_directories(lib/ASSIMP/include/assimp/Compiler)
include_directories(lib/ASSIMP/include/assimp/port)
include_directories(lib/ASSIMP/include/assimp/port/AndroidJNI)
include_directories(lib/GLEW)
include_directories(lib/GLEW/include)
include_directories(lib/GLEW/include/GL)
include_directories(lib/noclip)
include_directories(lib/sexymathlibrary)
include_directories(lib/SDL)
include_directories(lib/SDL/include)
include_directories(lib/stb)

add_compile_definitions(
        INTERNAL_BUILD=1
        SLOW_BUILD=1
)

add_executable(${PROJECT_NAME} code/main.cpp)

target_link_directories(${PROJECT_NAME}
        PRIVATE lib/GLEW/lib/Release/x64
        PRIVATE lib/SDL/lib/x64
        PRIVATE lib/ASSIMP/lib
        )

target_link_libraries(${PROJECT_NAME}
        Shell32.lib
        opengl32.lib
        glew32.lib
        SDL2.lib
        SDL2main.lib
        assimp-vc142-mt.lib
        )