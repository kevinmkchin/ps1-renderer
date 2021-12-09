
INTERNAL i16 SDL_JOYSTICK_DEAD_ZONE = 8000;

void SDLUpdateGamepads()
{
    LOCAL_PERSIST SDL_Joystick* gGamepadPointers[MAX_GAMEPAD_COUNT] = {};

    enum SDL_GAMEPAD_BUTTONS
    {
        /* TODO(Kevin): Check these are the same on other platforms like Mac
            e.g. #if ON_WINDOWS then use these values */
        SDL_GAMEPAD_A = 0,
        SDL_GAMEPAD_B = 1,
        SDL_GAMEPAD_X = 2,
        SDL_GAMEPAD_Y = 3,
        SDL_GAMEPAD_LB = 4,
        SDL_GAMEPAD_RB = 5,
        SDL_GAMEPAD_BACK = 6,
        SDL_GAMEPAD_START = 7,
        SDL_GAMEPAD_LTHUMB = 8,
        SDL_GAMEPAD_RTHUMB = 9
    };

    u8 prevConnectedGamepadCount = 0;
    for(int i = 0; i < MAX_GAMEPAD_COUNT; ++i)
    {
        if(gGamepadPointers[i])
        {
            ++prevConnectedGamepadCount;
        }
    }

    if(prevConnectedGamepadCount != SDL_NumJoysticks())
    {
        // reconnect controllers
        for(int i = 0; i < MAX_GAMEPAD_COUNT; ++i)
        {
            if(i < SDL_NumJoysticks())
            {
                bool bWasNull = gGamepadPointers[i] == nullptr;
                gGamepadPointers[i] = SDL_JoystickOpen(0);
                bool bIsNull = gGamepadPointers[i] == nullptr;
                if(bWasNull != bIsNull)
                {
                    printf("Gamepad %d connected.\n", i);
                }
                if(gGamepadPointers[i] == nullptr)
                {
                    printf("Warning: Unable to open gamepad index %d! SDL Error: %s\n", i, SDL_GetError());
                }
            }
            else
            {
                gGamepadPointers[i] = nullptr;
            }
        }
    }

    for(int i = 0; i < MAX_GAMEPAD_COUNT; ++i)
    {
        if(gGamepadPointers[i])
        {
            gGamepadStates[i].isConnected = true;
            gGamepadStates[i].previousButtonState = gGamepadStates[i].buttonState;
            gGamepadStates[i].buttonState = 0;
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_A))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_A;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_B))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_B;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_X))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_X;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_Y))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_Y;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_BACK))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_BACK;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_START))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_START;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_LB))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_LB;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_RB))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_RB;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_LTHUMB))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_LTHUMB;
            }
            if(SDL_JoystickGetButton(gGamepadPointers[0], SDL_GAMEPAD_RTHUMB))
            {
                gGamepadStates[i].buttonState |= GAMEPAD_RTHUMB;
            }

            u8 dpadState = SDL_JoystickGetHat(gGamepadPointers[0], 0);
            if(dpadState & SDL_HAT_UP)
            {
                gGamepadStates[i].buttonState |= GAMEPAD_DPAD_UP;
            }
            if(dpadState & SDL_HAT_DOWN)
            {
                gGamepadStates[i].buttonState |= GAMEPAD_DPAD_DOWN;
            }
            if(dpadState & SDL_HAT_LEFT)
            {
                gGamepadStates[i].buttonState |= GAMEPAD_DPAD_LEFT;
            }
            if(dpadState & SDL_HAT_RIGHT)
            {
                gGamepadStates[i].buttonState |= GAMEPAD_DPAD_RIGHT;
            }

            i16 axisPosition = SDL_JoystickGetAxis(gGamepadPointers[0], 0);
            if(kc_abs(axisPosition) > SDL_JOYSTICK_DEAD_ZONE) // axis 0 = Left Thumb X
            {
                float val = (float) axisPosition;
                val /= 32768.f;
                gGamepadStates[i].leftThumbStickDir.x = val;
            }
            else
            {
                gGamepadStates[i].leftThumbStickDir.x = 0.0f;
            }

            axisPosition = SDL_JoystickGetAxis(gGamepadPointers[0], 1);
            if(kc_abs(axisPosition) > SDL_JOYSTICK_DEAD_ZONE) // axis 1 = Left Thumb Y
            {
                float val = (float) -axisPosition;
                val /= 32768.f;
                gGamepadStates[i].leftThumbStickDir.y = val;
            }
            else
            {
                gGamepadStates[i].leftThumbStickDir.y = 0.0f;
            }

            axisPosition = SDL_JoystickGetAxis(gGamepadPointers[0], 3);
            if(kc_abs(axisPosition) > SDL_JOYSTICK_DEAD_ZONE) // axis 3 = Right Thumb X
            {
                float val = (float) axisPosition;
                val /= 32768.f;
                gGamepadStates[i].rightThumbStickDir.x = val;
            }
            else
            {
                gGamepadStates[i].rightThumbStickDir.x = 0.0f;
            }

            axisPosition = SDL_JoystickGetAxis(gGamepadPointers[0], 4);
            if(kc_abs(axisPosition) > SDL_JOYSTICK_DEAD_ZONE) // axis 4 = Right Thumb Y
            {
                float val = (float) -axisPosition;
                val /= 32768.f;
                gGamepadStates[i].rightThumbStickDir.y = val;
            }
            else
            {
                gGamepadStates[i].rightThumbStickDir.y = 0.0f;
            }

            gGamepadStates[i].leftThumbStickDir = normalize(gGamepadStates[i].leftThumbStickDir);
            gGamepadStates[i].rightThumbStickDir = normalize(gGamepadStates[i].rightThumbStickDir);

            axisPosition = SDL_JoystickGetAxis(gGamepadPointers[0], 2);
            if(axisPosition > -32768 + SDL_JOYSTICK_DEAD_ZONE)
            {
                // axis 2 = Left Trigger -32768 is unpressed, 32768 is completely pressed
                float val = (float) axisPosition;
                val = (val + 32768.f) / 65536.f;
                gGamepadStates[i].leftTrigger = val;
            }
            else
            {
                gGamepadStates[i].leftTrigger = 0.0f;
            }

            axisPosition = SDL_JoystickGetAxis(gGamepadPointers[0], 5);
            if(axisPosition > -32768 + SDL_JOYSTICK_DEAD_ZONE)
            {
                // axis 5 = Right Trigger -32768 is unpressed, 32768 is completely pressed
                float val = (float) axisPosition;
                val = (val + 32768.f) / 65536.f;
                gGamepadStates[i].rightTrigger = val;
            }
            else
            {
                gGamepadStates[i].rightTrigger = 0.0f;
            }
        }
        else
        {
            gGamepadStates[i].isConnected = false;
            gGamepadStates[i].buttonState = 0;
            gGamepadStates[i].previousButtonState = 0;
            gGamepadStates[i].leftTrigger = 0.f;
            gGamepadStates[i].rightTrigger = 0.f;
            gGamepadStates[i].leftThumbStickDir = {0.f, 0.f};
            gGamepadStates[i].leftThumbStickDir = {0.f, 0.f};
        }
    }
}