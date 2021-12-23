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
    bool32  isConnected = 0;
    u16     buttonState = 0; // check button state using AND (e.g. buttonState & GAMEPAD_A)
    u16     previousButtonState = 0;
    float   leftTrigger = 0.f; // 0.0 to 1.0
    float   rightTrigger = 0.f; // 0.0 to 1.0
    vec2    leftThumbStickDir = {0.f,0.f}; // normalized vector representing offset
    vec2    rightThumbStickDir = {0.f,0.f}; // normalized vector representing offset

    bool IsPressed(u16 button /* bitwise-OR-able */) const
    {
        return buttonState & button;
    }

    bool IsReleased(u16 button /* bitwise-OR-able */) const
    {
        return !IsPressed(button);
    }

    bool HasBeenPressed(u16 button) const
    {
        return ((previousButtonState & button) == 0) && (buttonState & button);
    }

    bool HasBeenReleased(u16 button) const
    {
        return (previousButtonState & button) && ((buttonState & button) == 0);
    }
};
