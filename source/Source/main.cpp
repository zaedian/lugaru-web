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

#include "Game.hpp"

#include "Audio/openal_wrapper.hpp"
#include "Graphic/gamegl.hpp"
#include "Menu/Menu.hpp"
#include "Platform/Platform.hpp"
#include "User/Settings.hpp"
#include "Utils/Folders.hpp"
#include "Utils/Input.hpp"
#include "Version.hpp"

#include <fstream>
#include <iostream>
#include <math.h>
#include <set>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace Game;

#ifdef WIN32
#include <shellapi.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

extern "C" void initialize_gl4es();

void DoUpdate();
void CleanUp(void);
static bool IsFocused();

static bool emscripten_gameDone = false;
static bool emscripten_gameFocused = true;

bool g_emscriptenMainLoopRunning = false;

static void emscripten_main_loop()
{
    if (!g_emscriptenMainLoopRunning) {
        g_emscriptenMainLoopRunning = true;
        set_vsync(!disablevsync);
    }

    if (emscripten_gameDone || tryquit) {
        emscripten_cancel_main_loop();
        deleteGame();
        CleanUp();
        return;
    }

    // Rebinding owns the event queue, including while focus is changing.
    if (Menu::isKeyCaptureActive()) {
        deltah = 0;
        deltav = 0;
        DoUpdate();
        return;
    }

    if (IsFocused()) {
        emscripten_gameFocused = true;

        deltah = 0;
        deltav = 0;
        SDL_Event e;
        if (!waiting) {
            // message pump
            while (SDL_PollEvent(&e)) {
                if (!sdlEventProc(e)) {
                    emscripten_gameDone = true;
                    emscripten_cancel_main_loop();
                    deleteGame();
                    CleanUp();
                    return;
                }
            }
        }

        DoUpdate();
    } else {
        if (emscripten_gameFocused) {
            emscripten_gameFocused = false;
            DoUpdate();
        }

        SDL_Event e;
        if (SDL_PollEvent(&e)) {
            if (!sdlEventProc(e)) {
                emscripten_gameDone = true;
                emscripten_cancel_main_loop();
                deleteGame();
                CleanUp();
                return;
            }
        }
    }
}
#endif

extern float multiplier;
extern float realmultiplier;
extern float frameDeltaTime;
extern bool isFirstPhysicsTickOfFrame;
extern bool physicsRanThisFrame;
extern int slomo;
extern bool cellophane;
extern float texdetail;

extern bool freeze;
extern bool stillloading;
extern int mainmenu;

extern float slomospeed;
extern float slomofreq;

extern int difficulty;

extern SDL_Window* sdlwindow;

using namespace std;

set<pair<int, int>> resolutions;

// statics/globals (internal only) ------------------------------------------
static void updateMouseState(bool grabbed);
static void resetPhysicsAccumulator();

// Menu defs

int kContextWidth;
int kContextHeight;

//-----------------------------------------------------------------------------------------------------------------------

// OpenGL Drawing

void initGL()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);
    swap_gl_buffers();

    // clear all states
#ifndef __EMSCRIPTEN__
    // These features don't exist in WebGL
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glDisable(GL_LOGIC_OP);
    glDisable(GL_TEXTURE_1D);
    glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
    glPixelTransferi(GL_RED_SCALE, 1);
    glPixelTransferi(GL_RED_BIAS, 0);
    glPixelTransferi(GL_GREEN_SCALE, 1);
    glPixelTransferi(GL_GREEN_BIAS, 0);
    glPixelTransferi(GL_BLUE_SCALE, 1);
    glPixelTransferi(GL_BLUE_BIAS, 0);
    glPixelTransferi(GL_ALPHA_SCALE, 1);
    glPixelTransferi(GL_ALPHA_BIAS, 0);
#endif
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    // set initial rendering states
    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);
#ifndef __EMSCRIPTEN__
    // Fixed-function pipeline features not in WebGL
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_DITHER);
    glEnable(GL_COLOR_MATERIAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glAlphaFunc(GL_GREATER, 0.5f);
#endif

    if (CanInitStereo(stereomode)) {
        InitStereo(stereomode);
    } else {
        fprintf(stderr, "Failed to initialize stereo, disabling.\n");
        stereomode = stereoNone;
    }
}

void toggleFullscreen()
{
    Uint32 flags = SDL_GetWindowFlags(sdlwindow);
    const bool enableFullscreen = (flags & SDL_WINDOW_FULLSCREEN) == 0;

#ifdef __EMSCRIPTEN__
    // Browser-native fullscreen via Fullscreen API
    if (enableFullscreen) {
        emscripten_run_script_string("(function(){var el=document.documentElement||document.body;if(el&&el.requestFullscreen){el.requestFullscreen().catch(function(){});}})();");
    } else {
        emscripten_run_script_string("(function(){if(document.exitFullscreen){document.exitFullscreen().catch(function(){});}})();");
    }
    fullscreen = enableFullscreen;
    updateMouseState(true);
    // Force canvas resize after browser viewport changes to prevent off-screen shift
    emscripten_run_script_string("setTimeout(function(){if(window.resizeCanvas){window.resizeCanvas();}},250);");
    return;
#endif

    if (enableFullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    } else {
        flags &= ~SDL_WINDOW_FULLSCREEN;
    }

    if (SDL_SetWindowFullscreen(sdlwindow, flags) == 0) {
        fullscreen = enableFullscreen;
    }

    updateMouseState(true);
}

SDL_bool sdlEventProc(const SDL_Event& e)
{
    switch (e.type) {
        case SDL_QUIT:
            return SDL_FALSE;

        case SDL_WINDOWEVENT:
#ifdef __EMSCRIPTEN__
            // Browsers can leave fullscreen independently, such as when the
            // user switches tabs or applications. Keep the menu setting in
            // sync with SDL's actual window state.
            fullscreen = (SDL_GetWindowFlags(sdlwindow) & SDL_WINDOW_FULLSCREEN) != 0;

            if (mainmenu == 3) {
                Menu::updateSettingsMenu();
            }

            // Refresh screen dimensions on resize to prevent screen-shift bug
            int w, h;
            SDL_GetWindowSize(sdlwindow, &w, &h);
            screenwidth = w;
            screenheight = h;
            glViewport(0, 0, w, h);
#endif

            if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                return SDL_FALSE;
            } else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED ||
                       e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                updateMouseState(false); // release mouse when minimised or focus lost
                deltah = 0;
                deltav = 0;
                resetPhysicsAccumulator();
                reset_frame_limiter();
            } else if (e.window.event == SDL_WINDOWEVENT_RESTORED ||
                       e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
                       e.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                updateMouseState(true); // restore mouse state when window is active
                deltah = 0;
                deltav = 0;
                resetPhysicsAccumulator();
                reset_frame_limiter();
            }
            break;

        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
            // Refresh immediately instead of waiting for the next scan tick.
            Input::UpdateController();
            break;

        case SDL_MOUSEMOTION:
            deltah += e.motion.xrel;
            deltav += e.motion.yrel;
            break;

        case SDL_KEYDOWN:
            if ((e.key.keysym.scancode == SDL_SCANCODE_G) &&
                (e.key.keysym.mod & KMOD_CTRL)) {
                SDL_bool mode = SDL_TRUE;
                if ((SDL_GetWindowFlags(sdlwindow) & SDL_WINDOW_FULLSCREEN) == 0) {
                    mode = (SDL_GetWindowGrab(sdlwindow) ? SDL_FALSE : SDL_TRUE);
                }
                SDL_SetWindowGrab(sdlwindow, mode);
                SDL_SetRelativeMouseMode(mode);
            } else if ((e.key.keysym.scancode == SDL_SCANCODE_RETURN) && (e.key.keysym.mod & KMOD_ALT)) {
                toggleFullscreen();
            }
            break;
    }
    return SDL_TRUE;
}

// --------------------------------------------------------------------------

static Point gMidPoint;

bool SetUp()
{
    LOGFUNC;

    cellophane = 0;
    texdetail = 4;
    slomospeed = 0.25;
    slomofreq = 8012;

    DefaultSettings();

#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO) == -1) {
            fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
            return false;
        }
    }
    if (!LoadSettings()) {
        fprintf(stderr, "Failed to load config, creating default\n");
        SaveSettings();
    }

    if (SDL_GL_LoadLibrary(NULL) == -1) {
        fprintf(stderr, "SDL_GL_LoadLibrary() failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

#ifdef __EMSCRIPTEN__
    // Emscripten exposes only the current canvas size as a display mode.
    // Provide a small set of 16:9 internal render resolutions instead.
    resolutions.insert(make_pair(1280, 720));
    resolutions.insert(make_pair(1600, 900));
    resolutions.insert(make_pair(1920, 1080));
    resolutions.insert(make_pair(2560, 1440));
#else
    for (int displayIdx = 0; displayIdx < SDL_GetNumVideoDisplays(); ++displayIdx) {
        for (int i = 0; i < SDL_GetNumDisplayModes(displayIdx); ++i) {
            SDL_DisplayMode mode;
            if (SDL_GetDisplayMode(displayIdx, i, &mode) == -1) {
                continue;
            }

            if ((mode.w < 640) || (mode.h < 480)) {
                continue; // sane lower limit.
            }

            pair<int, int> resolution(mode.w, mode.h);
            resolutions.insert(resolution);
        }
    }
#endif

    if (resolutions.empty()) {
        const std::string error = "No suitable video resolutions found.";
        cerr << error << endl;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Lugaru init failed!", error.c_str(), NULL);
        SDL_Quit();
        return false;
    }

    if (commandLineOptions[SHOWRESOLUTIONS]) {
        printf("Available resolutions:\n");
        for (auto resolution = resolutions.begin(); resolution != resolutions.end(); resolution++) {
            printf("  %d x %d\n", (int)resolution->first, (int)resolution->second);
        }
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
#endif
    Uint32 sdlflags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (commandLineOptions[FULLSCREEN]) {
        fullscreen = commandLineOptions[FULLSCREEN].last()->type();
    }

#ifdef __EMSCRIPTEN__
    // Browser fullscreen requires a user gesture and cannot be restored
    // reliably during startup. Always begin windowed and let the user
    // enter fullscreen after the page has loaded.
    fullscreen = 0;
#endif

    if (fullscreen) {
        sdlflags |= SDL_WINDOW_FULLSCREEN;
    }
    if (!commandLineOptions[NOMOUSEGRAB].last()->type()) {
        sdlflags |= SDL_WINDOW_INPUT_GRABBED;
    }

    sdlwindow = SDL_CreateWindow("Lugaru-Web", SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0), kContextWidth, kContextHeight, sdlflags);

    if (!sdlwindow) {
        fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        fprintf(stderr, "forcing 640x480...\n");
        kContextWidth = 640;
        kContextHeight = 480;
        sdlwindow = SDL_CreateWindow("Lugaru-Web", SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0), kContextWidth, kContextHeight, sdlflags);
        if (!sdlwindow) {
            fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
            fprintf(stderr, "forcing 640x480 windowed mode...\n");
            sdlflags &= ~SDL_WINDOW_FULLSCREEN;
            sdlwindow = SDL_CreateWindow("Lugaru-Web", SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0), kContextWidth, kContextHeight, sdlflags);

            if (!sdlwindow) {
                fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
                return false;
            }
        }
    }

    SDL_GLContext glctx = SDL_GL_CreateContext(sdlwindow);
    if (!glctx) {
        fprintf(stderr, "SDL_GL_CreateContext() failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_GL_MakeCurrent(sdlwindow, glctx);

#ifdef __EMSCRIPTEN__
    initialize_gl4es();
#endif

    int dblbuf = 0;
    if ((SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &dblbuf) == -1) || (!dblbuf)) {
        fprintf(stderr, "Failed to get a double-buffered context.\n");
        SDL_Quit();
        return false;
    }

    set_vsync(!disablevsync);

    updateMouseState(true);

    initGL();

    GLint width = kContextWidth;
    GLint height = kContextHeight;
    gMidPoint.h = width / 2;
    gMidPoint.v = height / 2;
    screenwidth = width;
    screenheight = height;

    newdetail = detail;
    newscreenwidth = screenwidth;
    newscreenheight = screenheight;

    /* If saved resolution is not in the list, add it to the list (so that it’s selectable in the options) */
    pair<int, int> startresolution(width, height);
    if (resolutions.find(startresolution) == resolutions.end()) {
        resolutions.insert(startresolution);
    }

    InitGame();
    Input::InitController();

    return true;
}

static void updateMouseState(bool grabbed)
{
    SDL_ShowCursor(0); // hide system cursor since we use custom cursor
    if (!commandLineOptions[NOMOUSEGRAB].last()->type()) {
        SDL_SetRelativeMouseMode(grabbed ? SDL_TRUE : SDL_FALSE);
        SDL_SetWindowGrab(sdlwindow, grabbed ? SDL_TRUE : SDL_FALSE);
    }
    if (sdlwindow) {
        SDL_UpdateWindowSurface(sdlwindow);
    }
}

static void DoMouse()
{

    if (mainmenu || ((abs(deltah) < 10 * realmultiplier * 1000) && (abs(deltav) < 10 * realmultiplier * 1000))) {
        deltah *= usermousesensitivity;
        deltav *= usermousesensitivity;
        mousecoordh += deltah;
        mousecoordv += deltav;
        if (mousecoordh < 0) {
            mousecoordh = 0;
        } else if (mousecoordh >= (int)screenwidth) {
            mousecoordh = (int)screenwidth - 1;
        }
        if (mousecoordv < 0) {
            mousecoordv = 0;
        } else if (mousecoordv >= (int)screenheight) {
            mousecoordv = (int)screenheight - 1;
        }
    }
}

void DoFrameRate(int update)
{
    static long frames = 0;

    static AbsoluteTime time = { 0, 0 };
    static AbsoluteTime frametime = { 0, 0 };
    AbsoluteTime currTime = UpTime();
    double deltaTime = (float)AbsoluteDeltaToDuration(currTime, frametime);

    if (0 > deltaTime) { // if negative microseconds
        deltaTime /= -1000000.0;
    } else { // else milliseconds
        deltaTime /= 1000.0;
    }

    // Calculate FPS from unclamped deltaTime for accurate display
    if (deltaTime > 0.0) {
        fps = 1.0f / deltaTime;
    } else {
        fps = 0.0f;
    }

    multiplier = deltaTime;
    if (multiplier < 0.000001f) {
        multiplier = 0.000001f; // prevent zero deltaTime without capping high FPS - Ryan Deering
    }
    if (multiplier > 10) {
        multiplier = 10;
    }
    if (update) {
        frametime = currTime; // reset for next time interval
    }

    deltaTime = (float)AbsoluteDeltaToDuration(currTime, time);

    if (0 > deltaTime) { // if negative microseconds
        deltaTime /= -1000000.0;
    } else { // else milliseconds
        deltaTime /= 1000.0;
    }
    frames++;
    if (0.001 <= deltaTime) { // has update interval passed
        if (update) {
            time = currTime; // reset for next time interval
            frames = 0;
        }
    }
}

// Global flag for accumulator reset
static bool g_resetAccumulator = false;

// Reset physics accumulator to prevent burst substeps after focus regain or pause
static void resetPhysicsAccumulator()
{
    g_resetAccumulator = true;
}

void DoUpdate()
{
    Input::UpdateController();

    // Fixed timestep at 1000Hz - matches physics constants tuned for high FPS
    const float PHYSICS_DT = 0.001f;
    static float physicsAccumulator = 0.f;

    if (g_resetAccumulator) {
        physicsAccumulator = 0.f;
        g_resetAccumulator = false;
    }

    DoFrameRate(1);

    // Guard against giant deltaTime from stalls/focus loss
    if (multiplier > 0.25f) {
        multiplier = 0.25f;
    }
    // Clamp for gameplay speed (slomo, difficulty still applied later)
    if (multiplier > .6) {
        multiplier = .6;
    }

    // Apply gameplay modifiers to frame time
    float frameTime = multiplier;
    realmultiplier = frameTime;
    frameTime *= gamespeed;
    if (difficulty == 1) {
        frameTime *= .9;
    }
    if (difficulty == 0) {
        frameTime *= .8;
    }
    if (loading == 4) {
        frameTime *= .00001;
    }
    if (slomo && !mainmenu) {
        frameTime *= slomospeed;
    }

    // Set multiplier for non-physics code (DoMouse, TickOnce, TickOnceAfter)
    multiplier = frameTime;
    frameDeltaTime = frameTime;  // For exponential damping in Tick()

    // Accumulate time for fixed timestep
    physicsAccumulator += frameTime;

    // Clamp accumulator to prevent spiral of death when physics can't keep up
    // Cap at 20 steps: supports 60fps at ~100% speed, 50fps+ at full speed
    // Below 50fps, game runs in slow motion (acceptable degradation)
    const int MAX_PHYSICS_STEPS = 20;
    const float MAX_ACCUMULATOR = MAX_PHYSICS_STEPS * PHYSICS_DT;
    if (physicsAccumulator > MAX_ACCUMULATOR) {
        physicsAccumulator = MAX_ACCUMULATOR;
    }

    DoMouse();
    Input::MoveControllerMenuCursor(realmultiplier);

    TickOnce();

    // Run physics at fixed 1000Hz timestep
    // At 60fps: ~17 steps/frame (99.6% speed)
    // At 144fps: ~7 steps/frame (99.4% speed)
    // At 240fps+: 1-4 steps/frame (100% speed)
    int steps = 0;
    isFirstPhysicsTickOfFrame = true;  // Mark first tick for head tracking
    physicsRanThisFrame = false;  // Track if any physics ran
    while (physicsAccumulator >= PHYSICS_DT && steps < MAX_PHYSICS_STEPS) {
        multiplier = PHYSICS_DT;  // Always use fixed dt for physics
        Tick();
        isFirstPhysicsTickOfFrame = false;  // Clear after first tick
        physicsRanThisFrame = true;  // At least one tick ran
        physicsAccumulator -= PHYSICS_DT;
        steps++;
    }

    // Restore multiplier to frame time for non-physics code
    multiplier = frameTime;

    TickOnceAfter();
    /* - Debug code to test how many channels were active on average per frame
        static long frames = 0;

        static AbsoluteTime start = {0,0};
        AbsoluteTime currTime = UpTime ();
        static int num_channels = 0;

        num_channels += OPENAL_GetChannelsPlaying();
        double deltaTime = (float) AbsoluteDeltaToDuration (currTime, start);

        if (0 > deltaTime)  // if negative microseconds
            deltaTime /= -1000000.0;
        else                // else milliseconds
            deltaTime /= 1000.0;

        ++frames;

        if (deltaTime >= 1)
        {
            start = currTime;
            float avg_channels = (float)num_channels / (float)frames;

            ofstream opstream("log.txt",ios::app);
            opstream << "Average frame count: ";
            opstream << frames;
            opstream << " frames - ";
            opstream << avg_channels;
            opstream << " per frame.\n";
            opstream.close();

            frames = 0;
            num_channels = 0;
        }
    */
    if (stereomode == stereoNone) {
        DrawGLScene(stereoCenter);
    } else {
        DrawGLScene(stereoLeft);
        DrawGLScene(stereoRight);
    }
}

// --------------------------------------------------------------------------

void CleanUp(void)
{
    LOGFUNC;

    delete[] commandLineOptionsBuffer;

    Input::ShutdownController();
    SDL_Quit();
}

// --------------------------------------------------------------------------

static bool IsFocused()
{
    return ((SDL_GetWindowFlags(sdlwindow) & SDL_WINDOW_INPUT_FOCUS) != 0);
}

#ifndef WIN32
// (code lifted from physfs: http://icculus.org/physfs/ ... zlib license.)
static char* findBinaryInPath(const char* bin, char* envr)
{
    size_t alloc_size = 0;
    char* exe = NULL;
    char* start = envr;
    char* ptr;

    do {
        size_t size;
        ptr = strchr(start, ':'); /* find next $PATH separator. */
        if (ptr) {
            *ptr = '\0';
        }
        size = strlen(start) + strlen(bin) + 2;
        if (size > alloc_size) {
            char* x = (char*)realloc(exe, size);
            if (x == NULL) {
                if (exe != NULL) {
                    free(exe);
                }
                return (NULL);
            } /* if */

            alloc_size = size;
            exe = x;
        } /* if */

        /* build full binary path... */
        strcpy(exe, start);
        if ((exe[0] == '\0') || (exe[strlen(exe) - 1] != '/')) {
            strcat(exe, "/");
        }
        strcat(exe, bin);

        if (access(exe, X_OK) == 0) { /* Exists as executable? We're done. */
            strcpy(exe, start);       /* i'm lazy. piss off. */
            return (exe);
        } /* if */

        start = ptr + 1; /* start points to beginning of next element. */
    } while (ptr != NULL);

    if (exe != NULL) {
        free(exe);
    }

    return (NULL); /* doesn't exist in path. */
} /* findBinaryInPath */

char* calcBaseDir(const char* argv0)
{
    /* If there isn't a path on argv0, then look through the $PATH for it. */
    char* retval;
    char* envr;

    if (strchr(argv0, '/')) {
        retval = strdup(argv0);
        if (retval) {
            *((char*)strrchr(retval, '/')) = '\0';
        }
        return (retval);
    }

    envr = getenv("PATH");
    if (!envr) {
        return NULL;
    }
    envr = strdup(envr);
    if (!envr) {
        return NULL;
    }
    retval = findBinaryInPath(argv0, envr);
    free(envr);
    return (retval);
}

static inline void chdirToAppPath(const char* argv0)
{
    char* dir = calcBaseDir(argv0);
    if (dir) {
#if (defined(__APPLE__) && defined(__MACH__))
        // Chop off /Contents/MacOS if it's at the end of the string, so we
        //  land in the base of the app bundle.
        const size_t len = strlen(dir);
        const char* bundledirs = "/Contents/MacOS";
        const size_t bundledirslen = strlen(bundledirs);
        if (len > bundledirslen) {
            char* ptr = (dir + len) - bundledirslen;
            if (strcasecmp(ptr, bundledirs) == 0)
                *ptr = '\0';
        }
#endif
        errno = 0;
        if (chdir(dir) != 0) {
            printf("Error changing dir to '%s' (%s).\n", dir, strerror(errno));
        }
        free(dir);
    }
}
#endif

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", option::Arg::None, "USAGE: lugaru [options]\n\n"
                                             "Options:" },
    { VERSION, 0, "v", "version", option::Arg::None, " -v, --version     Print version and exit." },
    { HELP, 0, "h", "help", option::Arg::None, " -h, --help        Print usage and exit." },
    { FULLSCREEN, 1, "f", "fullscreen", option::Arg::None, " -f, --fullscreen  Start the game in fullscreen mode." },
    { FULLSCREEN, 0, "w", "windowed", option::Arg::None, " -w, --windowed    Start the game in windowed mode (default)." },
    { NOMOUSEGRAB, 1, "", "nomousegrab", option::Arg::None, " --nomousegrab     Disable mousegrab." },
    { NOMOUSEGRAB, 0, "", "mousegrab", option::Arg::None, " --mousegrab       Enable mousegrab (default)." },
    { SOUND, 1, "", "nosound", option::Arg::None, " --nosound         Disable sound." },
    { OPENALINFO, 0, "", "openal-info", option::Arg::None, " --openal-info     Print info about OpenAL at launch." },
    { SHOWRESOLUTIONS, 0, "", "showresolutions", option::Arg::None, " --showresolutions List the resolutions found by SDL at launch." },
    { DEVTOOLS, 0, "d", "devtools", option::Arg::None, " -d, --devtools    Enable dev tools: console, level editor and debug info." },
    { CMD, 0, "c", "command", option::Arg::Optional, " -c, --command    Run this command at game start. May be used to load a map." },
    { 0, 0, 0, 0, 0, 0 }
};

option::Option commandLineOptions[commandLineOptionsNumber];
option::Option* commandLineOptionsBuffer;

int main(int argc, char** argv)
{
    argc -= (argc > 0);
    argv += (argc > 0); // skip program name argv[0] if present
    option::Stats stats(true, usage, argc, argv);
    if (commandLineOptionsNumber != stats.options_max) {
        std::cerr << "Found incorrect command line option number" << std::endl;
        return 1;
    }
    commandLineOptionsBuffer = new option::Option[stats.buffer_max];
    option::Parser parse(true, usage, argc, argv, commandLineOptions, commandLineOptionsBuffer);

    if (parse.error()) {
        delete[] commandLineOptionsBuffer;
        return 1;
    }

    // Always start by printing the version and info to the stdout
    std::cout << "--------------------------------------------------------------------------\n"
              << "Lugaru-Web - Lugaru in the browser\n"
              << "Based on Wolfire Games, OSS Lugaru, and Ryan Deering's Loupgarenne WebAssembly work.\n\n"
              << "Licensed under the GPL 2.0+ and CC-BY-SA 3.0 and 4.0 licenses.\n"
              << "Project: https://github.com/loganwalkerdev/lugaru-web\n"
              << "Upstream: https://gitlab.com/osslugaru/lugaru\n"
              << std::endl;

    std::cout << "Version " + VERSION_STRING + " -- " + VERSION_BUILD_TYPE + " build\n"
              << "--------------------------------------------------------------------------\n"
              << std::endl;

    if (commandLineOptions[VERSION]) {
        // That was enough, quit.
        delete[] commandLineOptionsBuffer;
        return 0;
    }

    if (commandLineOptions[HELP]) {
        option::printUsage(std::cout, usage);
        delete[] commandLineOptionsBuffer;
        return 0;
    }

    if (option::Option* opt = commandLineOptions[UNKNOWN]) {
        std::cerr << "Unknown option: " << opt->name << "\n";
        option::printUsage(std::cerr, usage);
        delete[] commandLineOptionsBuffer;
        return 1;
    }

// !!! FIXME: we could use a Win32 API for this.  --ryan.
#ifndef WIN32
    chdirToAppPath(argv[0]);
#endif

    LOGFUNC;

#ifdef NDEBUG
    try {
#endif
        {
            newGame();

            if (!SetUp()) {
                delete[] commandLineOptionsBuffer;
                return 42;
            }

            if (commandLineOptions[DEVTOOLS]) {
                devtools = true;
            }

            srand(time(nullptr));

            if (commandLineOptions[CMD].count() > 0) {
                devtools = true;
                Menu::startChallengeLevel(1);
                for (option::Option* opt = commandLineOptions[CMD]; opt; opt = opt->next()) {
                    if (opt->arg && (strlen(opt->arg) > 0)) {
                        cmd_dispatch(opt->arg);
                    }
                }
            }

#ifdef __EMSCRIPTEN__
            emscripten_set_main_loop(emscripten_main_loop, 0, 1);
#else
            bool gameDone = false;
            bool gameFocused = true;

            while (!gameDone && !tryquit) {
                if (IsFocused()) {
                    gameFocused = true;

                    // check windows messages

                    deltah = 0;
                    deltav = 0;
                    SDL_Event e;
                    if (!waiting) {
                        // message pump
                        while (SDL_PollEvent(&e)) {
                            if (!sdlEventProc(e)) {
                                gameDone = true;
                                break;
                            }
                        }
                    }

                    // game
                    DoUpdate();
                } else {
                    if (gameFocused) {
                        // allow game chance to pause
                        gameFocused = false;
                        DoUpdate();
                    }

                    // game is not in focus, give CPU time to other apps by waiting for messages instead of 'peeking'
                    SDL_Event e;
                    if (SDL_WaitEvent(&e)) {
                        if (!sdlEventProc(e)) {
                            gameDone = true;
                        }
                    }
                }
            }

            deleteGame();
#endif
        }

#ifndef __EMSCRIPTEN__
        CleanUp();
#endif

        return 0;
#ifdef NDEBUG
    } catch (const std::exception& error) {
        CleanUp();

        std::string e = "Caught exception: ";
        e += error.what();

        LOG(e);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Exception catched", error.what(), NULL);

        return -1;
    }
#endif
}
