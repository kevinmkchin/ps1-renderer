/** Playstation 1 style renderer

TODO:
    - Level Editor
        - creates the Visible level geometry and Collidable level geometry
        - Save level geom as OPCODE + STRUCT DATA so we know what type of struct we are reading

        Types of static level objects in game:
        - visible meshgroup object no collider              : cosmetic/decorative complex level geometry
        - visible mesh object no collider                   : cosmetic/decorative simple level geometry
        - visible meshgroup object with collider            : complex level geometry with collisions
        - visible mesh object with collider                 : simple level geometry with collisions
        - hidden mesh (or no mesh?) object with collider    : trigger volumes, level geometry bounds

        How to represent the above in editor?:


        Additionally, editor can specify other objects too e.g. power ups and their locations:
        - We can drop an editor object (no mesh, just a billboard facing the gameCamera indicating what power up it is)


        When importing/exporting to/from editor and importing to game, these objects need to be saved or read from disk.



    - Animation
    - Audio
    - Text rendering with kc_truetypeassembler.h

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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string>
#include <fstream>
#include <array>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>

#if (defined _WIN64)
    #include <GL/glew.h>
#elif (defined __APPLE__ || defined _APPLE)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#endif
#include <SDL.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <noclip.h>
#define SEXY_MATH_IMPLEMENTATION
#include <sexymathlibrary.h>

#include "gamedefines.h"
#include "shader.h"
#include "core.h"
#include "game.h"

GLOBAL_VAR bool gIsRunning = true;
GLOBAL_VAR float gDeltaTime;
GLOBAL_VAR const u8* gKeyState = nullptr; // Stores keyboard state this frame. Access via gKeystate[SDL_Scancode].
GLOBAL_VAR i32 gMouseDeltaX;
GLOBAL_VAR i32 gMouseDeltaY;
GLOBAL_VAR GamepadState gGamepadStates[MAX_GAMEPAD_COUNT] = {};

GLOBAL_VAR noclip::console console;
INTERNAL std::mutex __consoleCommandQueueMutex;
INTERNAL std::string __consoleCommandQueue;

#include "input.cpp"
#include "file.cpp"
#include "shader.cpp"
#include "core.cpp"
#include "level.cpp"
#include "game.cpp"

#pragma region PLATFORM_LAYER

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
INTERNAL bool editorModeActiveFlag;

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

    gKeyState = SDL_GetKeyboardState(nullptr);

    return true;
}

#if (defined __APPLE__ || defined _APPLE)
#include <unistd.h> // for chdir
#include <libgen.h> // for dirname
#include <mach-o/dyld.h> // for _NSGetExecutablePath
#include <limits.h> // for PATH_MAX?
INTERNAL void SetWorkingDirectory()
{
    char path[PATH_MAX];
    uint32_t pathLen = sizeof(path);
    int err = _NSGetExecutablePath(path, &pathLen);
    assert(!err);

    // Switch to the directory of the actual binary
    chdir(dirname(path));
    // and then go up three directories to get to the folder of the .app bundle
    chdir("../Resources");
}
#else
INTERNAL void SetWorkingDirectory()
{

}
#endif

INTERNAL bool GLEWInitialize()
{
    // glewExperimental = GL_TRUE;
    // if (glewInit() != GLEW_OK)
    // {
    //     printf("GLEW failed to initialize.\n");
    //     return false;
    // }
    // printf("GLEW initialized.\n");

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
    SDLUpdateGamepads();

    // Store Mouse state
    gMouseDeltaX = 0;
    gMouseDeltaY = 0;

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEBUTTONUP:
            {
                SDL_MouseButtonEvent mouseUp = event.button;
                if(mouseUp.button == SDL_BUTTON_RIGHT && mouseUp.state == SDL_RELEASED)
                {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    gameCamera.bMouseControl = false;
                    editorCamera.bMouseControl = false;
                }
            }

            case SDL_MOUSEBUTTONDOWN:
            {
                SDL_MouseButtonEvent mouseDown = event.button;
                if(mouseDown.button == SDL_BUTTON_RIGHT && mouseDown.state == SDL_PRESSED)
                {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    gameCamera.bMouseControl = true;
                    editorCamera.bMouseControl = true;
                }

                if(mouseDown.button == SDL_BUTTON_LEFT && mouseDown.state == SDL_PRESSED)
                {
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    float ix = (float) x * ((float) INTERNAL_RESOLUTION_WIDTH / (float)backbufferWidth);
                    float iy = (float) y * ((float) INTERNAL_RESOLUTION_HEIGHT / (float)backbufferHeight);
                    EditorMousePick((int) ix, (int) iy);
                }
            }

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
            } break;
        }
    }

    if(gGamepadStates[0].isConnected && gGamepadStates[0].HasBeenPressed(GAMEPAD_START))
    {
        gIsRunning = false;
    }
}

INTERNAL void SwitchToEditor(bool activateEditor)
{
    if(activateEditor != editorModeActiveFlag)
    {
        if(activateEditor)
        {
            GameUnload();
            editorModeActiveFlag = true;
            EditorLoad();
        }
        else
        {
            EditorUnload();
            editorModeActiveFlag = false;
            GameLoad();
        }
    }
}

INTERNAL void PollConsoleInput()
{
    while(gIsRunning)
    {
        std::string input;
        std::cin >> input;

        std::lock_guard<std::mutex> lock(__consoleCommandQueueMutex);
        __consoleCommandQueue += " " + input;
    }
}

INTERNAL void CheckConsoleCommandQueue()
{
    std::lock_guard<std::mutex> lock(__consoleCommandQueueMutex);
    if(!__consoleCommandQueue.empty())
    {
        console.execute(__consoleCommandQueue, std::cout);
        __consoleCommandQueue = std::string();
    }
}

INTERNAL bool CreateInternalFramebuffer()
{
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
            return false;
        }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Shader::GLLoadShaderProgramFromFile(quadShader, "shaders/quad.vert", "shaders/quad.frag");
    MeshCreate(finalOutputQuad, refQuadVertices, refQuadIndices, 16, 6, 2, 2, 0);

    return true;
}

INTERNAL void DrawInternalBufferToBackBuffer()
{
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
}

INTERNAL void BindConsoleCommandsAndVariables()
{
    console.bind_cmd("fullscreen", SDLFullscreen);

    console.bind_cmd("editor", [](std::istream& is, std::ostream& os){
        SwitchToEditor(true);
    });

    console.bind_cmd("game", [](std::istream& is, std::ostream& os){
        SwitchToEditor(false);
    });

    console.bind_cmd("camstats", [](std::istream& is, std::ostream& os){
        printf("gameCamera position x: %f, y: %f, z: %f \n", gameCamera.position.x, gameCamera.position.y, gameCamera.position.z);
        printf("gameCamera rotation roll: %f, yaw: %f, pitch: %f \n", gameCamera.rotation.x, gameCamera.rotation.y, gameCamera.rotation.z);
        printf("gameCamera speed: %f\n", gameCamera.moveSpeed);
        printf("gameCamera sensitivity: %f\n", gameCamera.turnSpeed);
    });
}

int main(int argc, char* argv[])
{
    SetWorkingDirectory();

    if(!SDLInitialize() || !GLEWInitialize() || !CreateInternalFramebuffer())
    {
        return -1;
    }

    BindConsoleCommandsAndVariables();
    std::thread consoleInputThread(PollConsoleInput);

    EditorInitOnStartUp();
    GameInitOnStartUp();
    GameLoad();
    editorModeActiveFlag = false;

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

            editorModeActiveFlag ? EditorUpdate() : GameUpdate();

            glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
            editorModeActiveFlag ? EditorRender() : GameRender();

            DrawInternalBufferToBackBuffer();

            SDL_GL_SwapWindow(window);
        }
    }

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(openglContext);
    SDL_Quit();

    consoleInputThread.detach();

    return 0;
}

#pragma endregion //PLATFORM_LAYER