cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 11)

project(ps1)

add_compile_definitions(
        INTERNAL_BUILD=1
        SLOW_BUILD=1
)

add_executable(${PROJECT_NAME} code/main.cpp)

include_directories(lib/noclip)
include_directories(lib/gmath)
include_directories(lib/stb)

if(WIN32)
    include_directories(lib/ASSIMP)
    include_directories(lib/ASSIMP/include)
    include_directories(lib/ASSIMP/include/assimp)
    include_directories(lib/ASSIMP/include/assimp/Compiler)
    include_directories(lib/ASSIMP/include/assimp/port)
    include_directories(lib/ASSIMP/include/assimp/port/AndroidJNI)
    include_directories(lib/GLEW)
    include_directories(lib/GLEW/include)
    include_directories(lib/GLEW/include/GL)
    include_directories(lib/SDL)
    include_directories(lib/SDL/include)

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
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_FRAMEWORK_IDENTIFIER org.cmake.${PROJECT_NAME})

    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
        message(STATUS "SDL2 found.")
        include_directories(${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
    endif()

    find_package(OpenGL REQUIRED)
    if(OpenGL_FOUND)
        message(STATUS "OpenGL found.")
        target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
    else()
        message(FATAL_ERROR "CMake couldn't find OpenGL on MacOSX.")
    endif()

    find_package(assimp REQUIRED)
    if(assimp_FOUND)
        message(STATUS "Assimp found.")
        include_directories(${ASSIMP_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${ASSIMP_LIBRARIES})
    endif()
endif()



