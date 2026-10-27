/*
Copyright (C) 2003, 2010 - Wolfire Games
Copyright (C) 2010-2017 - Lugaru contributors (see AUTHORS file)

This file is part of Lugaru, maintained as part of the Loupgarenne fork.
See README and AUTHORS for project details.

Lugaru is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Lugaru is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _INPUT_HPP_
#define _INPUT_HPP_

#include "Game.hpp"

#include <SDL.h>

/**> CONSTANT DECLARATIONS <**/
#define MOUSEBUTTON_LEFT (SDL_NUM_SCANCODES + SDL_BUTTON_LEFT)
#define MOUSEBUTTON_RIGHT (SDL_NUM_SCANCODES + SDL_BUTTON_RIGHT)
#define MOUSEBUTTON_MIDDLE (SDL_NUM_SCANCODES + SDL_BUTTON_MIDDLE)
#define MOUSEBUTTON_X1 (SDL_NUM_SCANCODES + SDL_BUTTON_X1)
#define MOUSEBUTTON_X2 (SDL_NUM_SCANCODES + SDL_BUTTON_X2)

/**> FUNCTION PROTOTYPES <**/
class Input
{
public:
    static void InitController();
    static void ShutdownController();
    static void UpdateController();
    static bool hasController();
    static const char* controllerName();
    // Raw standardized SDL values; no gameplay deadzone or bindings applied.
    static Sint16 controllerAxis(SDL_GameControllerAxis axis);
    static bool controllerButtonDown(SDL_GameControllerButton button);

    struct ControllerMovement {
        bool forward;
        bool back;
        bool left;
        bool right;
    };
    // Digital left-stick directions, separate from keyboard/mouse bindings.
    static ControllerMovement controllerMovement();

    struct ControllerLook {
        float x;
        float y;
    };
    // Right-stick rate fractions in [-1, 1], after an axial deadzone.
    static ControllerLook controllerLook();

    // UI edges are separate from raw mouse/keyboard input.
    static bool controllerMenuConfirmPressed();
    static bool controllerMenuBackPressed();
    static bool controllerPausePressed();
    static void MoveControllerMenuCursor(float seconds);

    // Triangle advances only during active tutorial gameplay, once per press.
    static bool controllerTutorialAdvancePressed();

    // Fixed gameplay buttons combined with the existing keyboard/mouse bindings.
    static bool isJumpDown();
    static bool isCrouchDown();
    static bool isDrawDown();
    static bool isThrowDown();
    static bool isAttackDown();
    static bool isAttackPressed();

    static void Tick();
    static bool isKeyDown(int k);
    static bool isKeyPressed(int k);
    static const char* keyToChar(unsigned short which);
    static bool MouseClicked();
};

#endif
