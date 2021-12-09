/** Playstation 1 style renderer

TODO:
    - make sure texture, mesh, mesh group aren't leaking memory.
    - read about framebuffers https://learnopengl.com/Advanced-OpenGL/Framebuffers

Rules:
     - unit vector (x: 1, y: 0, z: 0), aka positive X, is the "forward" direction for un-rotated objects
     - vec3(1, 0, 0) rotated by the object's orientation (represented by quaternion) is the object's forward direction
     - if quaternion is used to represent an orientation, then the quaternion represents the rotation from the WORLD FORWARD VECTOR
     - RIGHT HAND RULE for everything

BUILD MODES:
    INTERNAL_BUILD
        0 - Build for public release
        1 - Build for developer only
    SLOW_BUILD
        0 - No slow code allowed
        1 - Slow code fine
*/

#include <stdio.h>
#include <string>
#include <fstream>
#include <unordered_map>
#include <thread>

#include <GL/glew.h>
#include <SDL.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <noclip.h>
#define KC_MATH_IMPLEMENTATION
#include "kc_math.h"

#include "gamedefines.h"

INTERNAL bool gIsRunning = true;
INTERNAL float gDeltaTime;
INTERNAL const u8* gKeystate = nullptr; // Stores keyboard state this frame. Access via gKeystate[SDL_Scancode].
INTERNAL i32 gMouseDeltaX;
INTERNAL i32 gMouseDeltaY;

INTERNAL noclip::console console;
INTERNAL std::string consoleCommandQueue;

#include "file.cpp"
#include "shader.cpp"
#include "core.cpp"
#include "game.cpp"

#define WINDOW_TITLE "ps1 style renderer"
#define WINDOW_INITIAL_WIDTH 960
#define WINDOW_INITIAL_HEIGHT 720

INTERNAL SDL_Window* window = nullptr;
INTERNAL SDL_GLContext openglContext = nullptr;
INTERNAL i32 backbufferWidth = INDEX_NONE;
INTERNAL i32 backbufferHeight = INDEX_NONE;
INTERNAL u32 internalFBO = 0;
INTERNAL u32 internalColorTexture = 0;
INTERNAL u32 internalDepthBuffer = 0;
INTERNAL Mesh finalOutputQuad;
INTERNAL Shader quadShader;

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
            WINDOW_INITIAL_WIDTH,
            WINDOW_INITIAL_HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    )) == nullptr)
    {
        printf("SDL window failed to create.\n");
        return false;
    }
    SDL_SetWindowResizable(window, SDL_TRUE);
    SDL_GL_GetDrawableSize(window, &backbufferWidth, &backbufferHeight);

    if ((openglContext = SDL_GL_CreateContext(window)) == nullptr)
    {
        printf("Failed to create OpenGL context with SDL.\n");
        return false;
    }
    printf("SDL Window & OpenGL context created.\n");

    gKeystate = SDL_GetKeyboardState(nullptr);
    SDL_SetRelativeMouseMode(SDL_TRUE);

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

    return true;
}

INTERNAL void SDLWindowSizeChanged()
{
    SDL_GL_GetDrawableSize(window, &backbufferWidth, &backbufferHeight);
}

INTERNAL void SDLFullscreen(int mode)
{
    switch(mode)
    {
        case 0:{SDL_SetWindowFullscreen(window, 0);}break;
        case 1:{SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);}break;
        case 2:{SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);}break;
        default:{
            printf("Invalid fullscreen mode; 0 = windowed, 1 = fullscreen, 2 = fullscreen-windowed\n");
        }break;
    }
}

INTERNAL void SDLProcessEvents()
{
    // Store Mouse state
    gMouseDeltaX = 0;
    gMouseDeltaY = 0;

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEMOTION:
            {
                SDL_MouseMotionEvent mouseMotion = event.motion;
                gMouseDeltaX = mouseMotion.xrel;
                gMouseDeltaY = mouseMotion.yrel;
            } break;

            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    {
                        SDLWindowSizeChanged();
                    } break;
                }
            } break;

            case SDL_QUIT:
            {
                gIsRunning = false;
            } break;

            case SDL_KEYDOWN:
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    gIsRunning = false;
                }
                if (event.key.keysym.sym == SDLK_1)
                {
                    SDLFullscreen(0);
                }
                if (event.key.keysym.sym == SDLK_2)
                {
                    SDLFullscreen(1);
                }
                if (event.key.keysym.sym == SDLK_3)
                {
                    SDLFullscreen(2);
                }
            } break;
        }
    }
}

void PollConsoleInput()
{
    while(gIsRunning)
    {
        std::string input;
        std::cin >> input;
        consoleCommandQueue = consoleCommandQueue + " " + input;
    }
}

void CheckConsoleCommandQueue()
{
    if(!consoleCommandQueue.empty())
    {
        console.execute(consoleCommandQueue, std::cout);
        consoleCommandQueue = std::string();
    }
}

int main(int argc, char* argv[])
{
    if(SDLInitialize() == false || GLEWInitialize() == false)
    {
        return -1;
    }

    glGenFramebuffers(1, &internalFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
        glGenTextures(1, &internalColorTexture);
        glBindTexture(GL_TEXTURE_2D, internalColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internalColorTexture, 0);
        u32 colorAttachments[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, colorAttachments);
        glGenRenderbuffers(1, &internalDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, internalDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, internalDepthBuffer);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("Internal FrameBuffer Object failed to create.");
            return -1;
        }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Shader::GLLoadShaderProgramFromFile(quadShader, "shaders/quad.vert", "shaders/quad.frag");
    MeshCreate(finalOutputQuad, refQuadVertices, refQuadIndices, 16, 6, 2, 2, 0);

    std::thread consoleInputThread(PollConsoleInput);

    GameInit();

    float msPerFrame = 1000.f / (float)TARGET_FPS;
    float msLastTick = (float)SDL_GetTicks();
    float msSinceLastUpdate = 0.f;
    while(gIsRunning)
    {
        float msThisTick = (float)SDL_GetTicks();
        msSinceLastUpdate += msThisTick - msLastTick;
        msLastTick = msThisTick;

        while(msSinceLastUpdate > msPerFrame)
        {
            msSinceLastUpdate -= msPerFrame;

            gDeltaTime = msPerFrame / 1000.f;

            CheckConsoleCommandQueue();
            SDLProcessEvents();
            if(gIsRunning == false) { break; }

            GameUpdate();

            glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
            GameRender();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glViewport(0, 0, backbufferWidth, backbufferHeight);
            quadShader.UseShader();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, internalColorTexture);
                float internal_ratio = (float)INTERNAL_RESOLUTION_WIDTH / (float)INTERNAL_RESOLUTION_HEIGHT;
                float screen_ratio = (float)backbufferWidth / (float)backbufferHeight;
                float finalOutputQuadVertices[16] = {
                    -1.f, -1.f, 0.f, 0.f,
                    1.f, -1.f, 1.f, 0.f,
                    -1.f, 1.f, 0.f, 1.f,
                    1.f, 1.f, 1.f, 1.f
                };
                if(screen_ratio > internal_ratio)
                {
                    float w = backbufferHeight * internal_ratio;
                    float f = (backbufferWidth - w) / backbufferWidth;
                    finalOutputQuadVertices[0] = -1.f + f;
                    finalOutputQuadVertices[4] = 1.f - f;
                    finalOutputQuadVertices[8] = -1.f + f;
                    finalOutputQuadVertices[12] = 1.f - f;
                }
                else
                {
                    float h = backbufferWidth / internal_ratio;
                    float f = (backbufferHeight - h) / backbufferHeight;
                    finalOutputQuadVertices[1] = -1.f + f;
                    finalOutputQuadVertices[5] = -1.f + f;
                    finalOutputQuadVertices[9] = 1.f - f;
                    finalOutputQuadVertices[13] = 1.f - f;
                }
                finalOutputQuad.RebindBufferObjects(finalOutputQuadVertices, refQuadIndices, 16, 6);
                finalOutputQuad.RenderMesh();
            glUseProgram(0);

            SDL_GL_SwapWindow(window);
        }
    }

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(openglContext);
    SDL_Quit();

    consoleInputThread.detach();

    return 0;
}
