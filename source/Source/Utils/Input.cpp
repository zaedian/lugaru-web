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

#include "Utils/Input.hpp"
#include "Level/Campaign.hpp"
#include "Tutorial.hpp"
#include "User/Settings.hpp"

#include <algorithm>
#include <cmath>

extern int mainmenu;
extern int entername;
extern bool winfreeze;
extern bool freeze;
extern float screenwidth, screenheight;

bool keyDown[SDL_NUM_SCANCODES + 6];
bool keyPressed[SDL_NUM_SCANCODES + 6];

namespace {
SDL_GameController* controller = nullptr;
SDL_JoystickID controllerInstance = -1;
bool controllerSubsystemReady = false;
Uint32 nextControllerScan = 0;
Sint16 controllerAxes[SDL_CONTROLLER_AXIS_MAX] = {};
bool controllerButtons[SDL_CONTROLLER_BUTTON_MAX] = {};
#ifndef __EMSCRIPTEN__
int loggedAxes[SDL_CONTROLLER_AXIS_MAX] = {};
#endif
int controllerMoveX = 0;
int controllerMoveY = 0;
// Combined action history belongs to the input tick, not controller lifecycle:
// disconnecting must not invent a new press while keyboard/mouse is still held.
bool previousAttackDown = false;
bool attackPressed = false;
bool controllerPreviousButtons[SDL_CONTROLLER_BUTTON_MAX] = {};
bool controllerUIPressed[SDL_CONTROLLER_BUTTON_MAX] = {};
bool controllerBlockedButtons[SDL_CONTROLLER_BUTTON_MAX] = {};
int controllerMenuContext = -1;
bool controllerResultContext = false;
bool controllerModalContext = false;
float cursorRemainderX = 0.f;
float cursorRemainderY = 0.f;

bool controllerResultScreen()
{
    return !mainmenu && !campaign && winfreeze;
}

void syncControllerContext()
{
    const bool result = controllerResultScreen();
    const bool modal = Game::waiting || entername || Game::console;
    if (controllerMenuContext != mainmenu || controllerResultContext != result || controllerModalContext != modal) {
        // Also called by queries: a menu may resume gameplay in this same tick.
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            controllerBlockedButtons[i] = controllerBlockedButtons[i] || Input::controllerButtonDown(SDL_GameControllerButton(i));
            controllerUIPressed[i] = false;
        }
        controllerMenuContext = mainmenu;
        controllerResultContext = result;
        controllerModalContext = modal;
        cursorRemainderX = cursorRemainderY = 0.f;
    }
}

bool controllerGameplayButtonDown(SDL_GameControllerButton button)
{
    if (!gamepad) {
        return false;
    }
    syncControllerContext();
    return !mainmenu && !controllerResultContext && !controllerModalContext &&
           !controllerBlockedButtons[button] && Input::controllerButtonDown(button);
}

bool controllerUIPress(SDL_GameControllerButton button)
{
    if (!gamepad) {
        return false;
    }
    syncControllerContext();
    return !controllerBlockedButtons[button] && controllerUIPressed[button] && Input::controllerButtonDown(button);
}

int digitalMovementAxis(Sint16 value, int previous)
{
    // Independent axis hysteresis permits diagonals without proportional speed.
    constexpr int activate = 10000;
    constexpr int release = 7000;
    if (value <= -activate) {
        return -1;
    }
    if (value >= activate) {
        return 1;
    }
    if (previous < 0 && value < -release) {
        return -1;
    }
    if (previous > 0 && value > release) {
        return 1;
    }
    return 0;
}

float controllerLookAxis(Sint16 value)
{
    // Rescale continuously from zero outside the center-noise deadzone.
    // Separate signed endpoints keep full deflection symmetric.
    constexpr int deadzone = 8000;
    if (value > deadzone) {
        return float(value - deadzone) / (32767 - deadzone);
    }
    if (value < -deadzone) {
        return float(value + deadzone) / (32768 - deadzone);
    }
    return 0.f;
}

void clearControllerState()
{
    controllerMoveX = 0;
    controllerMoveY = 0;
    controllerMenuContext = -1;
    cursorRemainderX = cursorRemainderY = 0.f;
    for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
        controllerAxes[i] = 0;
#ifndef __EMSCRIPTEN__
        loggedAxes[i] = 0;
#endif
    }
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        controllerButtons[i] = false;
        controllerPreviousButtons[i] = false;
        controllerUIPressed[i] = false;
        controllerBlockedButtons[i] = false;
    }
}
}

void Input::InitController()
{
    if (controllerSubsystemReady) {
        return;
    }
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
        SDL_Log("Controller: initialization failed: %s", SDL_GetError());
        return;
    }
    controllerSubsystemReady = true;
    nextControllerScan = SDL_GetTicks();
    UpdateController();
}

void Input::ShutdownController()
{
    if (controller) {
        SDL_GameControllerClose(controller);
        controller = nullptr;
    }
    controllerInstance = -1;
    clearControllerState();
    if (controllerSubsystemReady) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        controllerSubsystemReady = false;
    }
}

void Input::UpdateController()
{
    if (!controllerSubsystemReady) {
        return;
    }
    // Poll independently of event consumers (including native key capture).
    SDL_GameControllerUpdate();
    if (controller && !SDL_GameControllerGetAttached(controller)) {
#ifndef __EMSCRIPTEN__
        SDL_Log("Controller: disconnected instance %d", int(controllerInstance));
#endif
        SDL_GameControllerClose(controller);
        controller = nullptr;
        controllerInstance = -1;
        clearControllerState();
        nextControllerScan = SDL_GetTicks();
    }
    if (!controller) {
        const Uint32 now = SDL_GetTicks();
        if (!SDL_TICKS_PASSED(now, nextControllerScan)) {
            return;
        }
        // Retry quickly so a freshly connected controller appears in the menu
        // without a noticeable delay, while still avoiding an open attempt
        // every single frame when no controller is present.
        nextControllerScan = now + 250;
        for (int device = 0; device < SDL_NumJoysticks(); ++device) {
            if (!SDL_IsGameController(device)) {
                continue;
            }
#ifndef __EMSCRIPTEN__
            const char* name = SDL_GameControllerNameForIndex(device);
            SDL_Log("Controller: detected device %d (%s)", device, name ? name : "unknown");
#endif
            controller = SDL_GameControllerOpen(device);
            if (!controller) {
                SDL_Log("Controller: open failed: %s", SDL_GetError());
                continue;
            }
            controllerInstance = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
#ifndef __EMSCRIPTEN__
            SDL_Log("Controller: opened instance %d (%s)", int(controllerInstance),
                    SDL_GameControllerName(controller) ? SDL_GameControllerName(controller) : "unknown");
#endif
            break;
        }
    }
    if (!controller) {
        return;
    }
    for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
        controllerAxes[i] = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis(i));
#ifndef __EMSCRIPTEN__
        // Coarse native diagnostics only: retain full raw precision in controllerAxes.
        if (SDL_abs(int(controllerAxes[i]) - loggedAxes[i]) >= 8192) {
            loggedAxes[i] = controllerAxes[i];
            SDL_Log("Controller: axis %s = %d", SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis(i)),
                    int(controllerAxes[i]));
        }
#endif
    }
    controllerMoveX = digitalMovementAxis(controllerAxes[SDL_CONTROLLER_AXIS_LEFTX], controllerMoveX);
    controllerMoveY = digitalMovementAxis(controllerAxes[SDL_CONTROLLER_AXIS_LEFTY], controllerMoveY);
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        const bool down = SDL_GameControllerGetButton(controller, SDL_GameControllerButton(i)) != 0;
#ifndef __EMSCRIPTEN__
        if (down != controllerButtons[i]) {
            SDL_Log("Controller: button %s %s", SDL_GameControllerGetStringForButton(SDL_GameControllerButton(i)),
                    down ? "down" : "up");
        }
#endif
        controllerButtons[i] = down;
        if (!down) {
            controllerBlockedButtons[i] = false;
        }
    }
}

bool Input::hasController()
{
    return controller && SDL_GameControllerGetAttached(controller);
}

const char* Input::controllerName()
{
    return hasController() ? SDL_GameControllerName(controller) : nullptr;
}

Sint16 Input::controllerAxis(SDL_GameControllerAxis axis)
{
    return hasController() && axis >= 0 && axis < SDL_CONTROLLER_AXIS_MAX ? controllerAxes[axis] : 0;
}

bool Input::controllerButtonDown(SDL_GameControllerButton button)
{
    return hasController() && button >= 0 && button < SDL_CONTROLLER_BUTTON_MAX && controllerButtons[button];
}

Input::ControllerMovement Input::controllerMovement()
{
    if (!gamepad || !hasController()) {
        return {};
    }
    return {controllerMoveY < 0, controllerMoveY > 0, controllerMoveX < 0, controllerMoveX > 0};
}

Input::ControllerLook Input::controllerLook()
{
    if (!gamepad) {
        return {0.f, 0.f};
    }
    return {controllerLookAxis(controllerAxis(SDL_CONTROLLER_AXIS_RIGHTX)) * gamepadsensitivity,
            controllerLookAxis(controllerAxis(SDL_CONTROLLER_AXIS_RIGHTY)) * gamepadsensitivity};
}

bool Input::isJumpDown()
{
    return isKeyDown(Game::jumpkey) || controllerGameplayButtonDown(SDL_CONTROLLER_BUTTON_A);
}

bool Input::isCrouchDown()
{
    return isKeyDown(Game::crouchkey) || controllerGameplayButtonDown(SDL_CONTROLLER_BUTTON_B);
}

bool Input::isDrawDown()
{
    return isKeyDown(Game::drawkey) || controllerGameplayButtonDown(SDL_CONTROLLER_BUTTON_X);
}

bool Input::isThrowDown()
{
    return isKeyDown(Game::throwkey) || controllerGameplayButtonDown(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
}

bool Input::isAttackDown()
{
    return isKeyDown(Game::attackkey) || controllerGameplayButtonDown(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
}

bool Input::isAttackPressed()
{
    return attackPressed && isAttackDown();
}

bool Input::controllerMenuConfirmPressed()
{
    return (mainmenu || controllerResultScreen()) && controllerUIPress(SDL_CONTROLLER_BUTTON_A);
}

bool Input::controllerMenuBackPressed()
{
    return (mainmenu || controllerResultScreen()) && controllerUIPress(SDL_CONTROLLER_BUTTON_B);
}

bool Input::controllerPausePressed()
{
    return Game::gameon && !Game::waiting && !Game::console &&
           ((mainmenu == 0 && !winfreeze) || mainmenu == 2) &&
           controllerUIPress(SDL_CONTROLLER_BUTTON_START);
}

bool Input::controllerTutorialAdvancePressed()
{
    // Reuse the sampled press edge and release guards without synthesizing Tab.
    return Tutorial::active && Game::gameon && !freeze && !winfreeze &&
           controllerGameplayButtonDown(SDL_CONTROLLER_BUTTON_Y) &&
           controllerUIPress(SDL_CONTROLLER_BUTTON_Y);
}

void Input::MoveControllerMenuCursor(float seconds)
{
    syncControllerContext();
    if (!gamepad || !mainmenu || Game::waiting || entername || !hasController() || screenwidth <= 0 || screenheight <= 0) {
        cursorRemainderX = cursorRemainderY = 0.f;
        return;
    }

    // Reuse the continuous 8000-axis deadzone response, on the LEFT stick.
    float x = controllerLookAxis(controllerAxis(SDL_CONTROLLER_AXIS_LEFTX));
    float y = controllerLookAxis(controllerAxis(SDL_CONTROLLER_AXIS_LEFTY));
    constexpr float cursorPixelsPerSecond = 320.f; // Logical 640x480 UI pixels.
    constexpr float finePixelsPerSecond = 80.f;
    float speed = cursorPixelsPerSecond * gamepadsensitivity;
    const bool dpad = controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
                      controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
                      controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                      controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    if (dpad) {
        // D-pad takes precedence so stick noise cannot spoil precision motion.
        x = int(controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) - int(controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_LEFT));
        y = int(controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_DOWN)) - int(controllerButtonDown(SDL_CONTROLLER_BUTTON_DPAD_UP));
        speed = finePixelsPerSecond;
    }
    const float length = std::sqrt(x * x + y * y);
    if (length > 1.f) {
        x /= length;
        y /= length;
    }
    if (x == 0.f) cursorRemainderX = 0.f;
    if (y == 0.f) cursorRemainderY = 0.f;
    cursorRemainderX += x * speed * seconds * screenwidth / 640.f;
    cursorRemainderY += y * speed * seconds * screenheight / 480.f;
    const int dx = int(cursorRemainderX);
    const int dy = int(cursorRemainderY);
    cursorRemainderX -= dx;
    cursorRemainderY -= dy;
    const int maxX = int(screenwidth) - 1;
    const int maxY = int(screenheight) - 1;
    Game::mousecoordh = std::max(0, std::min(maxX, Game::mousecoordh + dx));
    Game::mousecoordv = std::max(0, std::min(maxY, Game::mousecoordv + dy));
    // Discard only outward motion: slow inward motion still needs to accumulate
    // a whole pixel before the shared integer cursor can leave a boundary.
    if ((Game::mousecoordh == 0 && cursorRemainderX < 0.f) ||
        (Game::mousecoordh == maxX && cursorRemainderX > 0.f)) cursorRemainderX = 0.f;
    if ((Game::mousecoordv == 0 && cursorRemainderY < 0.f) ||
        (Game::mousecoordv == maxY && cursorRemainderY > 0.f)) cursorRemainderY = 0.f;
}

void Input::Tick()
{
    SDL_PumpEvents();
    int numkeys;
    const Uint8* keyState = SDL_GetKeyboardState(&numkeys);
    for (int i = 0; i < numkeys; i++) {
        keyPressed[i] = !keyDown[i] && keyState[i];
        keyDown[i] = keyState[i];
    }
    Uint8 mb = SDL_GetMouseState(NULL, NULL);
    for (int i = 1; i < 6; i++) {
        keyPressed[SDL_NUM_SCANCODES + i] = !keyDown[SDL_NUM_SCANCODES + i] && (mb & SDL_BUTTON(i));
        keyDown[SDL_NUM_SCANCODES + i] = (mb & SDL_BUTTON(i));
    }
    syncControllerContext();
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        const bool down = controllerButtonDown(SDL_GameControllerButton(i));
        controllerUIPressed[i] = down && !controllerPreviousButtons[i] && !controllerBlockedButtons[i];
        controllerPreviousButtons[i] = down;
    }
    // Like keyboard presses, expose the edge for one input/physics tick.
    const bool attackDown = isAttackDown();
    attackPressed = attackDown && !previousAttackDown;
    previousAttackDown = attackDown;
}

bool Input::isKeyDown(int k)
{
    if (k >= SDL_NUM_SCANCODES + 6) {
        return false;
    }
    return keyDown[k];
}

bool Input::isKeyPressed(int k)
{
    if (k >= SDL_NUM_SCANCODES + 6) {
        return false;
    }
    return keyPressed[k];
}

const char* Input::keyToChar(unsigned short i)
{
    if (i < SDL_NUM_SCANCODES) {
        return SDL_GetKeyName(SDL_GetKeyFromScancode(SDL_Scancode(i)));
    } else if (i == MOUSEBUTTON_LEFT) {
        return "mouse left button";
    } else if (i == MOUSEBUTTON_RIGHT) {
        return "mouse right button";
    } else if (i == MOUSEBUTTON_MIDDLE) {
        return "mouse middle button";
    } else if (i == MOUSEBUTTON_X1) {
        return "mouse button 4";
    } else if (i == MOUSEBUTTON_X2) {
        return "mouse button 5";
    } else {
        return "unknown";
    }
}

bool Input::MouseClicked()
{
    return isKeyPressed(MOUSEBUTTON_LEFT);
}
