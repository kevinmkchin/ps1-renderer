#define START_IN_EDITOR_MODE_ACTIVE
/** PRIMEVIL

TODO:
Task for today:
    - Load map to game

Backlog:
    - Get "game" working - player controller?
    - Face Texture properties: UV rotation, scale, other stuff
    - toggle snap to grid in level editor -> could even make vertices snap to vertex pos on other geom
    - Add/Remove edges
    - if i'm going to display vertex positions in imgui, then use the transformed vertex pos
    - Create mode, show preview of brush that can be moved around and shit
    - GAME_CLASS_CODE: instead of using bits, just use integer number
    - GameClass reflection + expose subclasses and properties to editor?
    - Dropdown to select brush type
    - Maybe DOUBLE CLICK to place an entity
    - LOAD ALL REQUIRED TEXTURES AT THE START OF THE GAME -> UNLOAD ALL END OF GAME instead of reference counting
    - shaded toggleable - brush can be shaded or not shaded
    - decals
    - fog
    - make gjk col MTV be relative to object we broadcasting to?
    - Text rendering with kc_truetypeassembler.h
    - Drop-down console port from XNGINE (tilde)
    - Animation
    - Audio
    - any floor/wall/slope/sprite can be individually shaded or palette swapped to create numerous variations and make objects really pop out

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
#define PATH_MAX MAX_PATH 
#endif

#include <stdio.h>
#include <direct.h> 
#include <limits.h>
#include <string>
#include <fstream>
#include <array>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#if (defined _WIN32)
    #include <GL/glew.h>
    #define __KEVIN_OPENGL_USING_GLEW__
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
#include "file.h"
#include "core.h"
#include "game.h"

/** Internal resolution : resolution the game is running at
    Back buffer resolution : resolution actually outputted to screen */
GLOBAL_VAR u32 internalResolutionWidth = 320;
GLOBAL_VAR u32 internalResolutionHeight = 240;
GLOBAL_VAR i32 backbufferWidth = INDEX_NONE;
GLOBAL_VAR i32 backbufferHeight = INDEX_NONE;
/** FOV of the game camera */
GLOBAL_VAR float FOV = 90.f;
/** Flag to indicate whether to exit the game */
GLOBAL_VAR bool gIsRunning = true;
/** Delta time since last tick. Updated every tick.  */
GLOBAL_VAR float gDeltaTime;
/** Flag to indicate whether editor mode is active. Do not set directly. */
GLOBAL_VAR bool editorModeActiveFlag;

/** Console backend */
GLOBAL_VAR noclip::console console;
INTERNAL std::mutex __consoleCommandQueueMutex;
INTERNAL std::string __consoleCommandQueue;

/** Shaders */
GLOBAL_VAR Shader sceneShader;
GLOBAL_VAR Shader editorShader;
GLOBAL_VAR Shader wireframeShader;
GLOBAL_VAR Shader mousepickShader;
GLOBAL_VAR Shader billboardShader;
INTERNAL Shader quadShader;

#include "input.cpp"
#include "shader.cpp"
#include "core.cpp"
#include "editor.cpp"
#include "game.cpp"

#pragma region PLATFORM_LAYER

#define WINDOW_TITLE "PRIMEVIL GAME"
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

    return true;
}

#if (defined __APPLE__ || defined _APPLE)
#include <unistd.h>
#include <libgen.h> // for dirname
#include <mach-o/dyld.h> // for _NSGetExecutablePath
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

INTERNAL bool OpenGLInitialize()
{
#ifdef __KEVIN_OPENGL_USING_GLEW__
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        printf("GLEW failed to initialize.\n");
        return false;
    }
    printf("GLEW initialized.\n");
#endif

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

INTERNAL vec2 ScreenSpaceToScreenNDC(int x, int y)
{
    float xNDC = ((float) x - ((float)backbufferWidth/2.f)) / ((float)backbufferWidth/2.f);
    float yNDC = ((float) y - ((float)backbufferHeight/2.f)) / ((float)-backbufferHeight/2.f);
    return vec2(xNDC, yNDC);
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

INTERNAL void SDLProcessEventsRealtime()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch(event.type)
        {
            case SDL_MOUSEBUTTONUP:{
                SDL_MouseButtonEvent mouseUp = event.button;
                if(mouseUp.button == SDL_BUTTON_LEFT && mouseUp.state == SDL_RELEASED)
                {
                    gIsLeftMouseDown = false;
                }
                if(mouseUp.button == SDL_BUTTON_RIGHT && mouseUp.state == SDL_RELEASED)
                {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_WarpMouseInWindow(window, backbufferWidth/2, backbufferHeight/2);
                    gameCamera.bMouseControl = false;
                    editorCamera.bMouseControl = false;
                }
            }break;

            case SDL_MOUSEBUTTONDOWN:{
                SDL_MouseButtonEvent mouseDown = event.button;
                if(mouseDown.button == SDL_BUTTON_LEFT && mouseDown.state == SDL_PRESSED)
                {
                    gIsLeftMouseDown = true;

                    if(editorModeActiveFlag)
                    {
                        if(!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
                        {
                            int x, y;
                            SDL_GetMouseState(&x, &y);
                            EditorMousePick(x, backbufferHeight - y);
                        }
                    }
                }
                if(mouseDown.button == SDL_BUTTON_RIGHT && mouseDown.state == SDL_PRESSED)
                {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    gameCamera.bMouseControl = true;
                    editorCamera.bMouseControl = true;
                }
            }break;

            case SDL_MOUSEMOTION:{
                ProcessSDLMouseMotionEvent(event.motion);
            }break;

            case SDL_KEYDOWN:{
                ProcessSDLKeyDownEvent(event.key);

                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    gIsRunning = false;
                }
            }break;
            case SDL_KEYUP:{
                ProcessSDLKeyUpEvent(event.key);
            }break;


            case SDL_CONTROLLERBUTTONDOWN:{
                ProcessSDLControllerButtonDownEvent(event.cbutton);
            }break;
            case SDL_CONTROLLERBUTTONUP:{
                ProcessSDLControllerButtonUpEvent(event.cbutton);
            }break;
            case SDL_CONTROLLERAXISMOTION:{
                ProcessSDLControllerAxisEvent(event.caxis);
            }break;
            case SDL_CONTROLLERDEVICEADDED:{
                SDLControllerConnected(event.cdevice.which);
            }break;
            case SDL_CONTROLLERDEVICEREMOVED:{
                SDLControllerRemoved(event.cdevice.which);
            }break;


            case SDL_WINDOWEVENT:{
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:{
                        SDLWindowSizeChanged();
                    }break;
                }
            }break;

            case SDL_QUIT:{
                gIsRunning = false;
            }break;
        }
    }
}

INTERNAL void ResetInputDeltas()
{
    /* We accumulate input delta in realtime, but we only read/check
     * the delta on tick. This means after we read/check the input delta,
     * we should reset them back to their initial values. */
    gMouseDelta = vec2(0.f,0.f);

    ResetKeyboardStates();
    ResetControllerStates();
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
    //ImGui::StyleColorsLight();
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
    SetWorkingDirectory();

    if(!SDLInitialize() || !OpenGLInitialize() || !CreateInternalFramebuffer())
    {
        return -1;
    }

    DearImGuiInit();

    BindConsoleCommandsAndVariables();
    std::thread consoleInputThread(PollConsoleInput);

    Shader::GLLoadShaderProgramFromFile(quadShader, "shaders/quad.vert", "shaders/quad.frag");
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    Shader::GLLoadShaderProgramFromFile(editorShader, "shaders/editor.vert", "shaders/editor.frag");
    Shader::GLLoadShaderProgramFromFile(wireframeShader, "shaders/wireframe.vert", "shaders/wireframe.frag");
    Shader::GLLoadShaderProgramFromFile(mousepickShader, "shaders/mousepick.vert", "shaders/mousepick.frag");
    Shader::GLLoadShaderProgramFromFile(billboardShader, "shaders/billboard.vert", "shaders/billboard.frag");

    EditorInitOnStartUp();
    GameInitOnStartUp();
#ifndef START_IN_EDITOR_MODE_ACTIVE
    editorModeActiveFlag = false;
    GameLoad();
#else
    editorModeActiveFlag = true;
    EditorLoad("");
#endif

    float msPerFrame = 1000.f / (float)TARGET_FPS;
    float msLastTick = (float)SDL_GetTicks();
    float msSinceLastUpdate = 0.f;
    while(gIsRunning)
    {
        float msThisTick = (float)SDL_GetTicks();
        msSinceLastUpdate += msThisTick - msLastTick;
        msLastTick = msThisTick;

        SDLProcessEventsRealtime();

        while(msSinceLastUpdate > msPerFrame)
        {
            msSinceLastUpdate -= msPerFrame;
            gDeltaTime = msPerFrame / 1000.f;

            CheckConsoleCommandQueue();
            if(gIsRunning == false) { break; }

            DearImGuiBeginNewFrame();

            editorModeActiveFlag ? EditorUpdate() : GameUpdate();
            if(!editorModeActiveFlag) glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
            editorModeActiveFlag ? EditorRender() : GameRender();
            if(!editorModeActiveFlag) DrawInternalBufferToBackBuffer();

            DearImGuiEndNewFrame();

            SDL_GL_SwapWindow(window);

            ResetInputDeltas();
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
