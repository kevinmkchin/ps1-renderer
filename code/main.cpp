/** Playstation 1 style renderer

TODO:
    - Save texture data to disk with map
    - Load map to game
    - Level Editor IMGUI to create new geometry and modify existing geom
    - Level Editor
        - creates the Visible level geometry and Collidable level geometry
        Types of static level objects in game:
        - visible meshgroup object no collider              : cosmetic/decorative complex level geometry
        - visible mesh object no collider                   : cosmetic/decorative simple level geometry
        - visible meshgroup object with collider            : complex level geometry with collisions
        - visible mesh object with collider                 : simple level geometry with collisions
        - hidden mesh (or no mesh?) object with collider    : trigger volumes, level geometry bounds
        Additionally, editor can specify other objects too e.g. power ups and their locations:
        - We can drop an editor object (no mesh, just a billboard facing the gameCamera indicating what power up it is)
    - Text rendering with kc_truetypeassembler.h
    - Drop-down console port from XNGINE
    - Animation
    - Audio

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

#include <GL/glew.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

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

GLOBAL_VAR u32 internalResolutionWidth = 320;
GLOBAL_VAR u32 internalResolutionHeight = 240;
GLOBAL_VAR i32 backbufferWidth = INDEX_NONE;
GLOBAL_VAR i32 backbufferHeight = INDEX_NONE;
GLOBAL_VAR float FOV = 90.f;
GLOBAL_VAR bool gIsRunning = true;
GLOBAL_VAR float gDeltaTime;
GLOBAL_VAR const u8* gKeyState = nullptr; // Stores keyboard state this frame. Access via gKeystate[SDL_Scancode].
GLOBAL_VAR i32 gMouseDeltaX;
GLOBAL_VAR i32 gMouseDeltaY;
GLOBAL_VAR GamepadState gGamepadStates[MAX_GAMEPAD_COUNT] = {};
GLOBAL_VAR bool editorModeActiveFlag;

GLOBAL_VAR noclip::console console;
INTERNAL std::mutex __consoleCommandQueueMutex;
INTERNAL std::string __consoleCommandQueue;

GLOBAL_VAR Shader sceneShader;
GLOBAL_VAR Shader editorShader;
INTERNAL Shader quadShader;

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
INTERNAL u32 internalFBO = 0;
INTERNAL u32 internalColorTexture = 0;
INTERNAL u32 internalDepthBuffer = 0;
INTERNAL Mesh finalOutputQuad;

INTERNAL bool SDLInitialize()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL failed to initialize.\n");
        return false;
    }

    // OpenGL Context Attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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
    editorCamera.CameraCalculatePerspectiveMatrix((float)backbufferWidth/(float)backbufferHeight);
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

INTERNAL vec2 ScreenSpaceToInternalNDC(int x, int y)
{
    float xCentered = (float) x - ((float)backbufferWidth/2.f);
    float yCentered = -((float) y - ((float)backbufferHeight/2.f));
    float internal_ratio = (float)internalResolutionWidth / (float)internalResolutionHeight;
    float screen_ratio = (float)backbufferWidth / (float)backbufferHeight;
    if(screen_ratio > internal_ratio)
    {
        float ratio = (float)backbufferHeight / (float)internalResolutionHeight;
        float internalResScreenPixelWidth = ratio * (float)internalResolutionWidth;
        float xNDC = xCentered / (internalResScreenPixelWidth / 2.f);
        float yNDC = yCentered / ((float)backbufferHeight / 2.f);
        return vec2(xNDC, yNDC);
    }
    else
    {
        float ratio = (float)backbufferWidth / (float)internalResolutionWidth;
        float internalResScreenPixelHeight = ratio * (float)internalResolutionHeight;
        float xNDC = xCentered / ((float)backbufferWidth / 2.f);
        float yNDC = yCentered / (internalResScreenPixelHeight / 2.f);
        return vec2(xNDC, yNDC);
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
            EditorLoad("");
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, internalResolutionWidth, internalResolutionHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, internalColorTexture, 0);
            u32 colorAttachments[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, colorAttachments);
        glGenRenderbuffers(1, &internalDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, internalDepthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, internalResolutionWidth, internalResolutionHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, internalDepthBuffer);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("Internal FrameBuffer Object failed to create.");
            return false;
        }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    MeshCreate(finalOutputQuad, refQuadVertices, refQuadIndices, 16, 6, 2, 2, 0);

    return true;
}

INTERNAL void UpdateInternalFramebufferSize()
{
    glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
    glBindTexture(GL_TEXTURE_2D, internalColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, internalResolutionWidth, internalResolutionHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, internalDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, internalResolutionWidth, internalResolutionHeight);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Failed to change size of Internal FrameBuffer Object.\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
            float internal_ratio = (float)internalResolutionWidth / (float)internalResolutionHeight;
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

INTERNAL void ChangeInternalResolution(u32 width, u32 height)
{
    internalResolutionWidth = width;
    internalResolutionHeight = height;
    UpdateInternalFramebufferSize();
    gameCamera.CameraCalculatePerspectiveMatrix();
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
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch(event.type)
        {
            case SDL_MOUSEBUTTONUP:
            {
                SDL_MouseButtonEvent mouseUp = event.button;
                if(mouseUp.button == SDL_BUTTON_RIGHT && mouseUp.state == SDL_RELEASED)
                {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_WarpMouseInWindow(window, backbufferWidth/2, backbufferHeight/2);
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
                    if(editorModeActiveFlag)
                    {
                        int x, y;
                        SDL_GetMouseState(&x, &y);
                        vec2 internalNDC = ScreenSpaceToInternalNDC(x,y);
                        if(SML_abs(internalNDC.x) <= 1.f && SML_abs(internalNDC.y) <= 1.f)
                        {
                            EditorMousePick(internalNDC.x, internalNDC.y);
                        }
                    }
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

INTERNAL void BindConsoleCommandsAndVariables()
{
    console.bind_cmd("fullscreen", SDLFullscreen);

    console.bind_cmd("internal_resolution_size", ChangeInternalResolution);

    console.bind_cmd("game", [](std::istream& is, std::ostream& os){
        SwitchToEditor(false);
    });

    console.bind_cmd("editor", [](std::istream& is, std::ostream& os){
        SwitchToEditor(true);
    });
    console.bind_cmd("export", EditorExportMap);
    console.bind_cmd("import", EditorSwitchMap);

    console.bind_cmd("camstats", [](std::istream& is, std::ostream& os){
        printf("gameCamera position x: %f, y: %f, z: %f \n", gameCamera.position.x, gameCamera.position.y, gameCamera.position.z);
        printf("gameCamera rotation roll: %f, yaw: %f, pitch: %f \n", gameCamera.rotation.x, gameCamera.rotation.y, gameCamera.rotation.z);
        printf("gameCamera speed: %f\n", gameCamera.moveSpeed);
        printf("gameCamera sensitivity: %f\n", gameCamera.turnSpeed);
    });
}

INTERNAL void DearImGuiInit()
{
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsLight();
    ImGui_ImplSDL2_InitForOpenGL(window, openglContext);
    ImGui_ImplOpenGL3_Init("#version 330");
}

INTERNAL void DearImGuiBeginNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

INTERNAL void DearImGuiEndNewFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

INTERNAL void DearImGuiShutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

int main(int argc, char* argv[])
{
    if(!SDLInitialize() || !GLEWInitialize() || !CreateInternalFramebuffer())
    {
        return -1;
    }

    DearImGuiInit();

    BindConsoleCommandsAndVariables();
    std::thread consoleInputThread(PollConsoleInput);

    Shader::GLLoadShaderProgramFromFile(quadShader, "shaders/quad.vert", "shaders/quad.frag");
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    Shader::GLLoadShaderProgramFromFile(editorShader, "shaders/editor.vert", "shaders/editor.frag");

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

            DearImGuiBeginNewFrame();

            editorModeActiveFlag ? EditorUpdate() : GameUpdate();
            if(!editorModeActiveFlag) glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
            editorModeActiveFlag ? EditorRender() : GameRender();
            if(!editorModeActiveFlag) DrawInternalBufferToBackBuffer();

            DearImGuiEndNewFrame();

            SDL_GL_SwapWindow(window);
        }
    }

    DearImGuiShutdown();

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(openglContext);
    SDL_Quit();

    consoleInputThread.detach();

    return 0;
}

#pragma endregion //PLATFORM_LAYER

#include "imgui/imgui.cpp"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_impl_sdl.cpp"
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui/imgui_impl_opengl3.cpp"
