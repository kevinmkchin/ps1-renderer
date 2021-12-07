/*

Project details:
- Playstation 1 style renderer
- Single Translation Unit Build

Rules:
     - unit vector (x: 1, y: 0, z: 0), aka positive X, is the "forward" direction for objects
     - direction != orientation
     - orientation ~= rotation
     - vec3(1, 0, 0) rotated by the object's orientation (represented by quaternion) is the object's forward direction
     - if quaternion is used to represent an orientation, then the quaternion represents the rotation from the WORLD FORWARD VECTOR
     - RIGHT HAND RULE for everything
     - size (unless using STL containers) means memory size, count means number of elements

BUILD MODES

    INTERNAL_BUILD:
        0 - Build for public release
        1 - Build for developer only

    SLOW_BUILD:
        0 - No slow code allowed
        1 - Slow code fine

*/

#include <stdio.h>
#include <string>
#include <fstream>
#include <unordered_map>

#include <GL/glew.h>
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "gamedefines.h"

INTERNAL bool gIsRunning = true;
INTERNAL float gDeltaTime;
INTERNAL const u8* gKeystate = nullptr; // Stores keyboard state this frame. Access via gKeystate[SDL_Scancode].

#include "file.cpp"
#include "shader.cpp"
#include "mesh.cpp"
#include "game.cpp"

#define WINDOW_TITLE "ps1 style renderer"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

INTERNAL SDL_Window* window = nullptr;
INTERNAL SDL_GLContext openglContext = nullptr;

INTERNAL bool SDLInitialize()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL failed to initialize.\n");
        return false;
    }

    // OpenGL Context Attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // version major
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3); // version minor
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); // core means not backward compatible. not using deprecated code.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // allow forward compatibility
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // double buffering is on by default, but let's just call this anyway
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); // depth buffer precision of 24 bits

    // Setup SDL Window
    if ((window = SDL_CreateWindow(
            WINDOW_TITLE,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    )) == nullptr)
    {
        printf("SDL window failed to create.\n");
        return false;
    }

    if ((openglContext = SDL_GL_CreateContext(window)) == nullptr)
    {
        printf("Failed to create OpenGL Context with SDL.\n");
        return false;
    }

    gKeystate = SDL_GetKeyboardState(nullptr);

    return true;
}

INTERNAL bool GLEWInitialize()
{
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        printf("GLEW failed to initialize.\n");
        return false;
    }
    printf("GLEW initialized.\n");

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha blending func: a * (rgb) + (1 - a) * (rgb) = final color output
    glBlendEquation(GL_FUNC_ADD);
    glEnable(GL_CULL_FACE);

    return true;
}

INTERNAL void SDLProcessEvents()
{
    SDL_Event event;
    if (SDL_PollEvent(&event) > 0)
    {
        switch (event.type)
        {
            case SDL_QUIT:
            {
                gIsRunning = false;
                break;
            }
            case SDL_KEYDOWN:
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    gIsRunning = false;
                    break;
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if(SDLInitialize() == false || GLEWInitialize() == false)
    {
        return -1;
    }

    gameInit();

    float msPerFrame = 1000.f / (float)TARGET_FPS;
    float msLastTick = (float)SDL_GetTicks();
    float msSinceLastUpdate = 0.f;
    while (gIsRunning)
    {
        float msThisTick = (float)SDL_GetTicks();
        msSinceLastUpdate += msThisTick - msLastTick;
        msLastTick = msThisTick;

        while (msSinceLastUpdate > msPerFrame)
        {
            msSinceLastUpdate -= msPerFrame;

            gDeltaTime = msPerFrame / 1000.f;

            SDLProcessEvents();
            if (gIsRunning == false) { break; }
            gameUpdate();
            gameRender();
            SDL_GL_SwapWindow(window);
        }
    }

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(openglContext);
    SDL_Quit();

    return 0;
}