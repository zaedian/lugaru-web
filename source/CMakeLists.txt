project(lugaru)

cmake_minimum_required(VERSION 3.13)

include(FindPkgConfig)
include(FetchContent)
include(GNUInstallDirs)


### Helpers

set(SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()


### CMake config

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

if(WIN32 AND NOT MINGW) #assuming MSVC
    #/W4 = lots of warnings
    #/EHsc = synchronous exceptions
    add_compile_options("/W4 /EHsc")
    if(CMAKE_BUILD_TYPE MATCHES DEBUG)
        #/Zi = edit and continue (aka hot reload)
        add_compile_options("/Zi")
    else()
        add_compile_options("/O2")
    endif() # check for MSVC debug build
elseif(MINGW OR NOT WIN32) #not MSVC but could be Unix or MSYS2 or MINGW or CYGWIN
    add_compile_options(-Wall -Wextra -Wno-parentheses -pedantic)
    if (CMAKE_BUILD_TYPE MATCHES DEBUG)
        add_compile_options(-g3 -Og)
    else()
        add_compile_options(-O2)
    endif() # check for debug build *nix.
endif() #check if WIN32

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for OSX (Universal Binary)")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
        "Minimum OS X version to target for deployment (at runtime); newer APIs weak linked. Set to empty string for default value")
    set(CMAKE_OSX_SYSROOT "" CACHE PATH
        "The product will be built against the headers and libraries located inside the indicated SDK.")
endif()

if(WIN32 AND MINGW)
    # Recent mingw requires this, see https://sourceforge.net/p/mingw-w64/bugs/818/
    add_compile_options("-fstack-protector")
    add_link_options("-fstack-protector")
endif()

if(LINUX)
    option(SYSTEM_INSTALL "Enable system-wide installation, with hardcoded data directory defined with CMAKE_INSTALL_DATADIR" OFF)
endif()

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

### Version

# Version for the current (stable) or next (development) release
set(LUGARU_VERSION_MAJOR 1)
set(LUGARU_VERSION_MINOR 3)
set(LUGARU_VERSION_PATCH 0)

# Lugaru-Web project release version. This is intentionally separate from the
# inherited Lugaru engine version above.
set(LUGARU_WEB_VERSION "1.3-dev - web port")

# MAJOR.MINOR, or MAJOR.MINOR.PATCH if PATCH != 0
set(LUGARU_VERSION_NUMBER "${LUGARU_VERSION_MAJOR}.${LUGARU_VERSION_MINOR}")
if(LUGARU_VERSION_PATCH)
    set(LUGARU_VERSION_NUMBER "${LUGARU_VERSION_NUMBER}.${LUGARU_VERSION_PATCH}")
endif()

# Inherited Lugaru/fork suffix, retained separately from the Lugaru-Web release version.
set(LUGARU_VERSION_SUFFIX "-lugaru-web")  # Lugaru-Web
#set(LUGARU_VERSION_SUFFIX "-dev")  # development
#set(LUGARU_VERSION_SUFFIX "")  # stable

# Set to 7-char git commit hash if available, otherwise "".
# On stable (tagged) builds, this is ignored.
set(LUGARU_VERSION_HASH "")
if(LUGARU_VERSION_SUFFIX STREQUAL "-dev" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
    find_package(Git)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE "LUGARU_VERSION_HASH"
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "Git commit hash: ${LUGARU_VERSION_HASH}")
    endif()
endif()

set(LUGARU_VERSION_RELEASE "" CACHE STRING "Optional release string, e.g. for distro packages release number")

# User-facing Lugaru-Web release identity. The project release version and
# inherited Lugaru engine version intentionally remain separate.
set(LUGARU_VERSION_STRING "Lugaru-Web v${LUGARU_WEB_VERSION} (Lugaru ${LUGARU_VERSION_NUMBER})")
if(NOT LUGARU_VERSION_HASH STREQUAL "")
    set(LUGARU_VERSION_STRING "${LUGARU_VERSION_STRING} (git ${LUGARU_VERSION_HASH})")
endif()
if(NOT LUGARU_VERSION_RELEASE STREQUAL "")
    set(LUGARU_VERSION_STRING "${LUGARU_VERSION_STRING} [${LUGARU_VERSION_RELEASE}]")
endif()

message(STATUS "Version string: ${LUGARU_VERSION_STRING}")
configure_file(${SRCDIR}/Version.hpp.in ${SRCDIR}/Version.hpp ESCAPE_QUOTES @ONLY)


### Sources

if(EMSCRIPTEN)
    set(PLATFORM_SRCS ${SRCDIR}/Platform/PlatformWeb.cpp)
else()
    set(PLATFORM_SRCS
        ${SRCDIR}/Platform/PlatformUnix.cpp
        ${SRCDIR}/Platform/PlatformWindows.cpp
    )
endif()

set(LUGARU_SRCS
    ${SRCDIR}/main.cpp
    ${PLATFORM_SRCS}
    ${SRCDIR}/Animation/Animation.cpp
    ${SRCDIR}/Animation/Joint.cpp
    ${SRCDIR}/Animation/Muscle.cpp
    ${SRCDIR}/Animation/Skeleton.cpp
    ${SRCDIR}/Audio/openal_wrapper.cpp
    ${SRCDIR}/Audio/Sounds.cpp
    ${SRCDIR}/Devtools/ConsoleCmds.cpp
    ${SRCDIR}/Environment/Lights.cpp
    ${SRCDIR}/Environment/Skybox.cpp
    ${SRCDIR}/Environment/Terrain.cpp
    ${SRCDIR}/Graphic/Decal.cpp
    ${SRCDIR}/Graphic/Models.cpp
    ${SRCDIR}/Graphic/Sprite.cpp
    ${SRCDIR}/Graphic/Stereo.cpp
    ${SRCDIR}/Graphic/Text.cpp
    ${SRCDIR}/Graphic/Texture.cpp
    ${SRCDIR}/Level/Awards.cpp
    ${SRCDIR}/Level/Campaign.cpp
    ${SRCDIR}/Level/Dialog.cpp
    ${SRCDIR}/Level/Hotspot.cpp
    ${SRCDIR}/Math/Frustum.cpp
    ${SRCDIR}/Math/XYZ.cpp
    ${SRCDIR}/Menu/Menu.cpp
    ${SRCDIR}/Objects/Object.cpp
    ${SRCDIR}/Objects/Person.cpp
    ${SRCDIR}/Objects/PersonType.cpp
    ${SRCDIR}/Objects/Weapons.cpp
    ${SRCDIR}/User/Account.cpp
    ${SRCDIR}/User/Settings.cpp
    ${SRCDIR}/Utils/Folders.cpp
    ${SRCDIR}/Utils/ImageIO.cpp
    ${SRCDIR}/Utils/Input.cpp
    ${SRCDIR}/Utils/pack.c
    ${SRCDIR}/Utils/private.c
    ${SRCDIR}/Utils/unpack.c
    ${SRCDIR}/Game.cpp
    ${SRCDIR}/GameDraw.cpp
    ${SRCDIR}/GameInitDispose.cpp
    ${SRCDIR}/GameTick.cpp
    ${SRCDIR}/Globals.cpp
    ${SRCDIR}/Tutorial.cpp
)

set(LUGARU_H
    ${SRCDIR}/Animation/Animation.hpp
    ${SRCDIR}/Animation/Joint.hpp
    ${SRCDIR}/Animation/Muscle.hpp
    ${SRCDIR}/Animation/Skeleton.hpp
    ${SRCDIR}/Audio/openal_wrapper.hpp
    ${SRCDIR}/Audio/Sounds.hpp
    ${SRCDIR}/Devtools/ConsoleCmds.hpp
    ${SRCDIR}/Environment/Lights.hpp
    ${SRCDIR}/Environment/Skybox.hpp
    ${SRCDIR}/Environment/Terrain.hpp
    ${SRCDIR}/Graphic/Decal.hpp
    ${SRCDIR}/Graphic/gamegl.hpp
    ${SRCDIR}/Graphic/Models.hpp
    ${SRCDIR}/Graphic/Sprite.hpp
    ${SRCDIR}/Graphic/Stereo.hpp
    ${SRCDIR}/Graphic/Text.hpp
    ${SRCDIR}/Graphic/Texture.hpp
    ${SRCDIR}/Level/Campaign.hpp
    ${SRCDIR}/Level/Dialog.hpp
    ${SRCDIR}/Level/Hotspot.hpp
    ${SRCDIR}/Math/Frustum.hpp
    ${SRCDIR}/Math/XYZ.hpp
    ${SRCDIR}/Math/Random.hpp
    ${SRCDIR}/Menu/Menu.hpp
    ${SRCDIR}/Objects/Object.hpp
    ${SRCDIR}/Objects/Person.hpp
    ${SRCDIR}/Objects/PersonType.hpp
    ${SRCDIR}/Objects/Weapons.hpp
    ${SRCDIR}/Platform/Platform.hpp
    ${SRCDIR}/Thirdparty/optionparser.h
    ${SRCDIR}/User/Account.hpp
    ${SRCDIR}/User/Settings.hpp
    ${SRCDIR}/Utils/binio.h
    ${SRCDIR}/Utils/Folders.hpp
    ${SRCDIR}/Utils/ImageIO.hpp
    ${SRCDIR}/Utils/Input.hpp
    ${SRCDIR}/Utils/private.h
    ${SRCDIR}/Game.hpp
    ${SRCDIR}/Tutorial.hpp

)

set(LUGARU_OBJS "")
if(WIN32 AND NOT MINGW)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Lugaru.res
                       COMMAND ${CMAKE_RC_COMPILER}
                       /fo ${CMAKE_CURRENT_BINARY_DIR}/Lugaru.res
                       ${SRCDIR}/Lugaru.rc
                       DEPENDS ${SRCDIR}/Lugaru.rc
    )
    set(LUGARU_OBJS "Lugaru.res")
elseif(CYGWIN OR MINGW)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Lugaru.res
                       COMMAND ${CMAKE_RC_COMPILER}
                       -o ${CMAKE_CURRENT_BINARY_DIR}/Lugaru.res
                       -i${SRCDIR}/Lugaru.rc
                       DEPENDS ${SRCDIR}/Lugaru.rc
    )
    set(LUGARU_OBJS "Lugaru.res")
endif()

if(APPLE)
    set(PLATFORM_LIBS "-framework Carbon -framework Cocoa -framework OpenGL -framework OpenAL")
endif()


### Dependencies

if(EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten/WebAssembly build with GL4ES")

    # Web data is served from the same origin as the generated shell.
    set(DATA_CDN_FLAG "")

    FetchContent_Declare(
    gl4es
    GIT_REPOSITORY https://github.com/ptitSeb/gl4es.git
    GIT_TAG        8b424fa5aeea8a2445961ae98c326dc6061e109a
    PATCH_COMMAND
    ${CMAKE_COMMAND}
    -DGL4ES_SOURCE_DIR=<SOURCE_DIR>
    -DGL4ES_PATCH=${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/gl4es-alpha-to-coverage.patch
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply-gl4es-patch.cmake
)
    set(STATICLIB ON CACHE BOOL "" FORCE)
    set(NOX11 ON CACHE BOOL "" FORCE)
    set(NOEGL ON CACHE BOOL "" FORCE)
    set(NO_LOADER ON CACHE BOOL "" FORCE)
    set(NO_INIT_CONSTRUCTOR ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(gl4es)
    include_directories(${gl4es_SOURCE_DIR}/include)

    FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG        1.9.5
    )
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(jsoncpp)
    include_directories(${jsoncpp_SOURCE_DIR}/include)

    add_compile_options(
        "SHELL:-s USE_SDL=2"
        "SHELL:-s USE_LIBPNG=1"
        "SHELL:-s USE_LIBJPEG=1"
        "SHELL:-s USE_VORBIS=1"
        "SHELL:-s USE_OGG=1"
        -Oz                    
        -fexceptions           
        -DNDEBUG               
        -flto                   
    )

    add_link_options(
        "SHELL:-s FULL_ES2=1"
        "SHELL:-s USE_SDL=2"
        "SHELL:-s USE_LIBPNG=1"
        "SHELL:-s USE_LIBJPEG=1"
        "SHELL:-s USE_VORBIS=1"
        "SHELL:-s USE_OGG=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s MAX_WEBGL_VERSION=2"
        "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
        "SHELL:-s ASSERTIONS=0"
        "SHELL:-s GL_ASSERTIONS=0"
        "SHELL:-s ENVIRONMENT=web"
        "SHELL:-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$addRunDependency"
        "SHELL:--closure 1"
        -lidbfs.js
        -Oz
        -flto
        -fexceptions
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/Data@/Data"
        "SHELL:--pre-js ${CMAKE_SOURCE_DIR}/Misc/web-persistence.js"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/Misc/shell.html"
        ${DATA_CDN_FLAG}
    )

    set(LUGARU_EMSCRIPTEN_LIBS
        GL
        jsoncpp_static
    )

    set(OPENAL_LIBRARY "")
    set(PNG_LIBRARY "")
    set(JPEG_LIBRARY "")
    set(SDL2_LIBRARIES "")
    set(OPENGL_gl_LIBRARY "")
    set(VORBISFILE_LIBRARIES "")
    set(JSONCPP_LIBRARIES "")

    set(OPENAL_INCLUDE_DIR "")
    set(JPEG_INCLUDE_DIR "")
    set(PNG_INCLUDE_DIR "")
    set(OPENGL_INCLUDE_DIR "")
    set(SDL2_INCLUDE_DIRS "")
    set(VORBISFILE_INCLUDE_DIR "")
    set(JSONCPP_INCLUDE_DIRS "")
else()
    # TODO: Work on supporting "GLVND" instead.
    set(OpenGL_GL_PREFERENCE LEGACY)
    find_package(OpenGL REQUIRED)

    if(WIN32)
        pkg_check_modules(OPENAL openal REQUIRED)
        set(OPENAL_LIBRARY ${OPENAL_LIBRARIES})
    else()
        find_package(OpenAL REQUIRED)
    endif()

    if(APPLE)
        find_package(sdl2 REQUIRED)
    elseif(WIN32)
        find_package(SDL2 CONFIG REQUIRED)
    else()
        execute_process(
            COMMAND sdl2-config --cflags
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE "SDL2_INCLUDE_DIRS"
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND sdl2-config --libs
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE "SDL2_LIBRARIES"
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        message("SDL2_INCLUDE_DIRS='${SDL2_INCLUDE_DIRS}'")
        message("SDL2_LIBRARIES='${SDL2_LIBRARIES}'")
        pkg_check_modules(SDL2 sdl2 REQUIRED)
    endif()

    find_package(PNG REQUIRED)
    find_package(JPEG REQUIRED)

    pkg_check_modules(VORBISFILE vorbisfile REQUIRED)
    pkg_check_modules(VORBIS vorbis REQUIRED)
    pkg_check_modules(OGG ogg REQUIRED)
    pkg_check_modules(JSONCPP jsoncpp REQUIRED)
endif()

if(NOT EMSCRIPTEN)
    link_directories(${VORBISFILE_LIBRARY_DIRS} ${VORBIS_LIBRARY_DIRS} ${OGG_LIBRARY_DIRS} ${JSONCPP_LIBRARY_DIRS})
endif()

include_directories(
    ${OPENAL_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/Source
)

set(LUGARU_LIBS ${OPENAL_LIBRARY} ${PNG_LIBRARY} ${JPEG_LIBRARY} ${SDL2_LIBRARIES} ${OPENGL_gl_LIBRARY} ${VORBISFILE_LIBRARIES} ${JSONCPP_LIBRARIES} ${PLATFORM_LIBS})


### Definitions

add_executable(lugaru ${LUGARU_SRCS} ${LUGARU_H} ${LUGARU_OBJS})

if(EMSCRIPTEN)
    set_property(
        TARGET lugaru
        APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/Misc/web-persistence.js"
    )
endif()

target_link_libraries(lugaru ${LUGARU_LIBS})

if(EMSCRIPTEN)
    target_link_libraries(lugaru ${LUGARU_EMSCRIPTEN_LIBS})
endif()

if(WIN32)
    add_definitions(-DBinIO_STDINT_HEADER=<stdint.h>)
    if(MINGW)
        add_definitions(-DWIN32)
    endif()
elseif(EMSCRIPTEN)
    add_definitions(-DPLATFORM_LINUX=1 -DPLATFORM_UNIX=1 -DBinIO_STDINT_HEADER=<stdint.h>)
else()
    add_definitions(-DPLATFORM_LINUX=1 -DPLATFORM_UNIX=1 -DBinIO_STDINT_HEADER=<stdint.h>)
endif()


### Installation

if(NOT CMAKE_INSTALL_PREFIX AND WIN32)
    set(CMAKE_INSTALL_PREFIX "C:/Lugaru")
endif()

# OS-specific installation paths

set(LUGARU_DOCDIR ${CMAKE_INSTALL_PREFIX})

if(APPLE)
    set(LUGARU_APP_ROOT ${CMAKE_INSTALL_PREFIX}/Lugaru.app)
    set(LUGARU_BINDIR ${LUGARU_APP_ROOT}/Contents/MacOS)
    set(LUGARU_RESDIR ${LUGARU_APP_ROOT}/Contents/Resources)
endif()

# Actual installation instructions

if(WIN32)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lugaru.exe DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Data DESTINATION ${CMAKE_INSTALL_PREFIX})
    if(MINGW)
        set(DLL_ROOT ${CMAKE_FIND_ROOT_PATH}/bin)
        # We need the shared libgcc, but it can have different names based on arch/config/version.
        # Ideally we'd check what our deps and lugaru.exe actually require, but for now it's a hack.
        # Based off Mageia/Fedora MinGW toolchain, might not work on other distros or Windows
        if(EXISTS ${DLL_ROOT}/libgcc_s_seh-1.dll)
            set(LIBGCC_S libgcc_s_seh-1.dll)
        elseif(EXISTS ${DLL_ROOT}/libgcc_s_dw2-1.dll)
            set(LIBGCC_S libgcc_s_dw2-1.dll)
        elseif(EXISTS ${DLL_ROOT}/libgcc_s_sjlj-1.dll)
            set(LIBGCC_S libgcc_s_sjlj-1.dll)
        endif()
        # FIXME: Filter out unneeded DLLs when building against some internal deps
        install(FILES ${DLL_ROOT}/${LIBGCC_S}
                      ${DLL_ROOT}/libjpeg-62.dll
                      ${DLL_ROOT}/libjsoncpp.dll
                      ${DLL_ROOT}/libogg-0.dll
                      ${DLL_ROOT}/libpng16-16.dll
                      ${DLL_ROOT}/libssp-0.dll
                      ${DLL_ROOT}/libstdc++-6.dll
                      ${DLL_ROOT}/libvorbis-0.dll
                      ${DLL_ROOT}/libvorbisfile-3.dll
                      ${DLL_ROOT}/libwinpthread-1.dll
                      ${DLL_ROOT}/OpenAL32.dll
                      ${DLL_ROOT}/SDL2.dll
                      ${DLL_ROOT}/zlib1.dll
                DESTINATION ${CMAKE_INSTALL_PREFIX})
    endif()
endif()

if(LINUX)
    if(SYSTEM_INSTALL)
        add_definitions(-DDATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
        set(LUGARU_DOCDIR ${CMAKE_INSTALL_DOCDIR})
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lugaru DESTINATION ${CMAKE_INSTALL_BINDIR})
        # Trailing '/' is significant, it installs and _renames_ Data/ as the destination folder
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/Data/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
        install(FILES ${CMAKE_SOURCE_DIR}/Dist/Linux/lugaru.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
        install(FILES ${CMAKE_SOURCE_DIR}/Dist/Linux/lugaru.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
        install(FILES ${CMAKE_SOURCE_DIR}/Dist/Linux/lugaru.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps)
        install(FILES ${CMAKE_SOURCE_DIR}/Dist/Linux/lugaru.6 DESTINATION ${CMAKE_INSTALL_MANDIR}/man6)
    else()
        message("You are building Lugaru without having enabled the SYSTEM_INSTALL option. It will default to looking for the data in the 'Data' directory next to the binary.")
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lugaru DESTINATION ${CMAKE_INSTALL_PREFIX})
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/Data DESTINATION ${CMAKE_INSTALL_PREFIX})
    endif()
endif()

if(APPLE)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lugaru DESTINATION ${LUGARU_BINDIR})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/Data DESTINATION ${LUGARU_APP_ROOT})
    install(FILES ${CMAKE_SOURCE_DIR}/Dist/OSX/Lugaru.icns DESTINATION ${LUGARU_RESDIR})
    install(FILES ${CMAKE_SOURCE_DIR}/Dist/OSX/Info.plist DESTINATION ${LUGARU_APP_ROOT}/Contents)
endif()

# Documentation

install(FILES ${CMAKE_SOURCE_DIR}/AUTHORS
              ${CMAKE_SOURCE_DIR}/CONTENT-LICENSE.txt
              ${CMAKE_SOURCE_DIR}/COPYING.txt
              ${CMAKE_SOURCE_DIR}/README.md
              ${CMAKE_SOURCE_DIR}/Docs/DEVTOOLS.txt
              ${CMAKE_SOURCE_DIR}/Docs/README.Empire.txt
              ${CMAKE_SOURCE_DIR}/Docs/README.SevenTasks.txt
              ${CMAKE_SOURCE_DIR}/Docs/README.Temple.txt
        DESTINATION ${LUGARU_DOCDIR})
