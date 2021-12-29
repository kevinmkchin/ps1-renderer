#include "input.h"

/** INPUT **/

// Stores the mouse delta accumulated between each frame
GLOBAL_VAR bool gIsLeftMouseDown = false;
GLOBAL_VAR vec2 gMouseDelta;

// Keyboard State - Access via SDL_Scancode
GLOBAL_VAR u8 gCurrentKeyState[256];
GLOBAL_VAR u8 gJustPressedKeyState[256];
GLOBAL_VAR u8 gJustReleasedKeyState[256];

// Gamepad State - Look inside GamepadStatesWrapper and GamepadStates
GLOBAL_VAR GamepadStatesWrapper gGamepadStates;


/** Mouse **/

INTERNAL void ProcessSDLMouseMotionEvent(SDL_MouseMotionEvent mouseMotionEvent)
{
    if(SML_abs(mouseMotionEvent.xrel) < 50 && SML_abs(mouseMotionEvent.yrel) < 50)
    {
        gMouseDelta += vec2((float)mouseMotionEvent.xrel, (float)mouseMotionEvent.yrel);
    }
}


/** Keyboard **/

INTERNAL void ProcessSDLKeyDownEvent(SDL_KeyboardEvent keyEvent)
{
    SDL_Scancode key = keyEvent.keysym.scancode;
    gCurrentKeyState[key] = 1;
    gJustPressedKeyState[key] = 1; 
}

INTERNAL void ProcessSDLKeyUpEvent(SDL_KeyboardEvent keyEvent)
{
    // Never set JustPressed or JustReleased to 0 here.
    SDL_Scancode key = keyEvent.keysym.scancode;
    gCurrentKeyState[key] = 0;
    gJustReleasedKeyState[key] = 1;
}

INTERNAL void ResetKeyboardStates()
{
    memset(gJustPressedKeyState, 0, 256);
    memset(gJustReleasedKeyState, 0, 256);
}


/** Gamepad **/

INTERNAL void ProcessSDLControllerButtonDownEvent(SDL_ControllerButtonEvent gamepadButtonEvent)
{
    /* SDL_GameControllerButton reference:
    SDL_CONTROLLER_BUTTON_A 0
    SDL_CONTROLLER_BUTTON_B 1
    SDL_CONTROLLER_BUTTON_X 2
    SDL_CONTROLLER_BUTTON_Y 3
    SDL_CONTROLLER_BUTTON_BACK 4
    SDL_CONTROLLER_BUTTON_GUIDE 5
    SDL_CONTROLLER_BUTTON_START 6
    SDL_CONTROLLER_BUTTON_LEFTSTICK 7
    SDL_CONTROLLER_BUTTON_RIGHTSTICK 8
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER 9
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 10
    SDL_CONTROLLER_BUTTON_DPAD_UP 11
    SDL_CONTROLLER_BUTTON_DPAD_DOWN 12
    SDL_CONTROLLER_BUTTON_DPAD_LEFT 13
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT 14
    */

    i32 joystickInstanceID = gamepadButtonEvent.which;
    i32 gamepadIndex = gGamepadStates.GamepadIndexFromInstanceID(joystickInstanceID);
    u8 buttonIndex = gamepadButtonEvent.button;

    switch(buttonIndex)
    {
        case 0: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_A;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_A;
        } break;
        case 1: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_B;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_B;
        } break;
        case 2: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_X;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_X;
        } break;
        case 3: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_Y;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_Y;
        } break;
        case 4: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_BACK;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_BACK;
        } break;
        case 6: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_START;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_START;
        } break;
        case 7: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_LTHUMB;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_LTHUMB;
        } break;
        case 8: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_RTHUMB;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_RTHUMB;
        } break;
        case 9: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_LB;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_LB;
        } break;
        case 10: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_RB;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_RB;
        } break;
        case 11: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_UP;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_UP;
        } break;
        case 12: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_DOWN;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_DOWN;
        } break;
        case 13: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_LEFT;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_LEFT;
        } break;
        case 14: { 
            gGamepadStates[gamepadIndex].currentButtonState |= GAMEPAD_DPAD_RIGHT;
            gGamepadStates[gamepadIndex].justPressedButtonState |= GAMEPAD_DPAD_RIGHT;
        } break;
    }
}

INTERNAL void ProcessSDLControllerButtonUpEvent(SDL_ControllerButtonEvent gamepadButtonEvent)
{
    i32 joystickInstanceID = gamepadButtonEvent.which;
    i32 gamepadIndex = gGamepadStates.GamepadIndexFromInstanceID(joystickInstanceID);
    u8 buttonIndex = gamepadButtonEvent.button;

    switch(buttonIndex)
    {
        case 0: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_A;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_A;
        } break;
        case 1: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_B;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_B;
        } break;
        case 2: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_X;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_X;
        } break;
        case 3: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_Y;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_Y;
        } break;
        case 4: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_BACK;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_BACK;
        } break;
        case 6: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_START;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_START;
        } break;
        case 7: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_LTHUMB;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_LTHUMB;
        } break;
        case 8: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_RTHUMB;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_RTHUMB;
        } break;
        case 9: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_LB;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_LB;
        } break;
        case 10: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_RB;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_RB;
        } break;
        case 11: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_UP;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_UP;
        } break;
        case 12: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_DOWN;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_DOWN;
        } break;
        case 13: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_LEFT;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_LEFT;
        } break;
        case 14: { 
            gGamepadStates[gamepadIndex].currentButtonState ^= GAMEPAD_DPAD_RIGHT;
            gGamepadStates[gamepadIndex].justReleasedButtonState |= GAMEPAD_DPAD_RIGHT;
        } break;
    }
}

INTERNAL i16 SDL_JOYSTICK_DEAD_ZONE = 8000;
INTERNAL i16 SDL_JOYSTICK_MAX = 32767;

INTERNAL void ProcessSDLControllerAxisEvent(SDL_ControllerAxisEvent gamepadAxisEvent)
{
    i32 joystickInstanceID = gamepadAxisEvent.which;
    i32 gamepadIndex = gGamepadStates.GamepadIndexFromInstanceID(joystickInstanceID);
    u8 axisIndex = gamepadAxisEvent.axis;
    i16 axisValue = gamepadAxisEvent.value;

    switch(axisIndex)
    {
        case SDL_CONTROLLER_AXIS_LEFTX:{
            if(SML_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE)
            {
                vec2 leftAxisDir = gGamepadStates[gamepadIndex].leftThumbStickDir * (float)SDL_JOYSTICK_MAX;
                leftAxisDir.x = (float)axisValue;
                leftAxisDir /= (float)SDL_JOYSTICK_MAX;
                gGamepadStates[gamepadIndex].leftThumbStickDir = leftAxisDir;
            }
            else
            {
                gGamepadStates[gamepadIndex].leftThumbStickDir.x = 0.f;
            }
        }break;
        case SDL_CONTROLLER_AXIS_LEFTY:{
            if(SML_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE)
            {
                vec2 leftAxisDir = gGamepadStates[gamepadIndex].leftThumbStickDir * (float)SDL_JOYSTICK_MAX;
                leftAxisDir.y = -(float)axisValue;
                leftAxisDir /= (float)SDL_JOYSTICK_MAX;
                gGamepadStates[gamepadIndex].leftThumbStickDir = leftAxisDir;
            }
            else
            {
                gGamepadStates[gamepadIndex].leftThumbStickDir.y = 0.f;
            }
        }break;
        case SDL_CONTROLLER_AXIS_RIGHTX:{
            if(SML_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE)
            {
                vec2 rightAxisDir = gGamepadStates[gamepadIndex].rightThumbStickDir * (float)SDL_JOYSTICK_MAX;
                rightAxisDir.x = (float)axisValue;
                rightAxisDir /= (float)SDL_JOYSTICK_MAX;
                gGamepadStates[gamepadIndex].rightThumbStickDir = rightAxisDir;
            }
            else
            {
                gGamepadStates[gamepadIndex].rightThumbStickDir.x = 0.f;
            }
        }break;
        case SDL_CONTROLLER_AXIS_RIGHTY:{
            if(SML_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE)
            {
                vec2 rightAxisDir = gGamepadStates[gamepadIndex].rightThumbStickDir * (float)SDL_JOYSTICK_MAX;
                rightAxisDir.y = -(float)axisValue;
                rightAxisDir /= (float)SDL_JOYSTICK_MAX;
                gGamepadStates[gamepadIndex].rightThumbStickDir = rightAxisDir;
            }
            else
            {
                gGamepadStates[gamepadIndex].rightThumbStickDir.y = 0.f;
            }
        }break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:{
            if(SML_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE)
            {
                gGamepadStates[gamepadIndex].leftTrigger = (axisValue + ((float)SDL_JOYSTICK_MAX)) / (2*((float)SDL_JOYSTICK_MAX));
            }
            else
            {
                gGamepadStates[gamepadIndex].leftTrigger = 0.f;
            }
        }break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:{
            if(SML_abs(axisValue) > SDL_JOYSTICK_DEAD_ZONE)
            {
                gGamepadStates[gamepadIndex].rightTrigger = (axisValue + ((float)SDL_JOYSTICK_MAX)) / (2*((float)SDL_JOYSTICK_MAX));
            }
            else
            {
                gGamepadStates[gamepadIndex].rightTrigger = 0.f;
            }
        }break;
    }
}

INTERNAL void ResetControllerStates()
{
    for(int i = 0; i < MAX_GAMEPAD_COUNT; ++i)
    {
        gGamepadStates[i].justPressedButtonState = 0;
        gGamepadStates[i].justReleasedButtonState = 0;
    }
}

INTERNAL void SDLControllerConnected(i32 deviceIndex)
{
    SDL_GameController* deviceHandle = SDL_GameControllerOpen(deviceIndex);
    i32 joystickInstanceID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(deviceHandle));
    gGamepadStates.OnConnect(joystickInstanceID);

    printf("Gamepad connected. Instance ID: %d\n", joystickInstanceID);
}

INTERNAL void SDLControllerRemoved(i32 joystickInstanceID)
{
    gGamepadStates.OnDisconnect(joystickInstanceID);
    SDL_GameControllerClose(SDL_GameControllerFromInstanceID(joystickInstanceID));

    printf("Gamepad disconnected. Instance ID: %d\n", joystickInstanceID);
}
