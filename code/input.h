/** Gamepad **/
#define MAX_GAMEPAD_COUNT   4
#define GAMEPAD_A           0x0001  // bit 0
#define GAMEPAD_B           0x0002  // bit 1
#define GAMEPAD_X           0x0004  // bit 2
#define GAMEPAD_Y           0x0008  // bit 3
#define GAMEPAD_LB          0x0010  // bit 4
#define GAMEPAD_RB          0x0020  // bit 5
#define GAMEPAD_BACK        0x0040  // bit 6
#define GAMEPAD_START       0x0080  // bit 7
#define GAMEPAD_LTHUMB      0x0100  // bit 8
#define GAMEPAD_RTHUMB      0x0200  // bit 9
#define GAMEPAD_DPAD_UP     0x1000  // bit 12
#define GAMEPAD_DPAD_DOWN   0x2000  // bit 13
#define GAMEPAD_DPAD_LEFT   0x4000  // bit 14
#define GAMEPAD_DPAD_RIGHT  0x8000  // bit 15

struct GamepadState
{
    /* Stores the state of an Xbox 360 style controller */
    bool16  isConnected = 0;
    u16     currentButtonState = 0; // check button state using AND (e.g. buttonState & GAMEPAD_A)
    u16     justPressedButtonState = 0;
    u16     justReleasedButtonState = 0;
    float   leftTrigger = 0.f; // 0.0 to 1.0
    float   rightTrigger = 0.f; // 0.0 to 1.0
    vec2    leftThumbStickDir = {0.f,0.f}; // normalized vector representing offset
    vec2    rightThumbStickDir = {0.f,0.f}; // normalized vector representing offset

    bool IsPressed(u16 button /* bitwise-OR-able */) const
    {
        return currentButtonState & button;
    }

    bool IsReleased(u16 button /* bitwise-OR-able */) const
    {
        return !IsPressed(button);
    }

    bool HasBeenPressed(u16 button) const
    {
        return justPressedButtonState & button;
    }

    bool HasBeenReleased(u16 button) const
    {
        return justReleasedButtonState & button;
    }
};

struct GamepadStatesWrapper
{
    /** Wrapper is necessary because SDL_JoystickInstanceID doesn't
        necessarily match the gamepad index. This wrapper also
        shifts gamepad indices down when a controller disconnects.
        e.g. if player 1 disconnects, player 2 becomes player 1. */

    u8 mapInstanceIDtoGamepadIndex[128];
    GamepadState states[MAX_GAMEPAD_COUNT];
    u8 gamepadCount = 0;

    GamepadState& operator[](const i32 gamepadIndex)
    {
        return states[gamepadIndex];
    }

    GamepadState& AtInstanceID(const i32 instanceID)
    {
        return states[mapInstanceIDtoGamepadIndex[instanceID]];
    }

    u8 GamepadIndexFromInstanceID(const i32 instanceID)
    {
        return mapInstanceIDtoGamepadIndex[instanceID];
    }

    void OnConnect(i32 instanceID)
    {
        mapInstanceIDtoGamepadIndex[instanceID] = gamepadCount;
        states[gamepadCount].isConnected = true;
        ++gamepadCount;
    }

    void OnDisconnect(i32 instanceID)
    {
        i32 gamepadIndex = mapInstanceIDtoGamepadIndex[instanceID];

        // shift game states left
        for(i32 i = gamepadIndex; i < gamepadCount-1; ++i)
        {
            states[i] = states[i + 1];
        } 

        // reset last game state. we shift these over so just resetting the last is enough.
        states[gamepadCount-1].isConnected = false;
        states[gamepadCount-1].currentButtonState = 0;
        states[gamepadCount-1].justPressedButtonState = 0;
        states[gamepadCount-1].justReleasedButtonState = 0;
        states[gamepadCount-1].leftTrigger = 0.f;
        states[gamepadCount-1].rightTrigger = 0.f;
        states[gamepadCount-1].leftThumbStickDir = {0.f, 0.f};
        states[gamepadCount-1].leftThumbStickDir = {0.f, 0.f};

        // shift indices left
        for(int i = 0; i < 128; ++i)
        {
            if(gamepadIndex < mapInstanceIDtoGamepadIndex[i])
            {
                --mapInstanceIDtoGamepadIndex[i];
            }
        }

        --gamepadCount;
    }
};
