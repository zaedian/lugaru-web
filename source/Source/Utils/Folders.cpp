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

#include "Folders.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#if PLATFORM_UNIX || defined(__EMSCRIPTEN__)
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if _WIN32
#include <shlobj.h> // to get paths related functions
#include <windows.h>
#endif

const std::string Folders::dataDir = DATA_DIR;

std::string Folders::getScreenshotDir()
{
    std::string screenshotDir = getUserDataPath() + "/Screenshots";
    makeDirectory(screenshotDir);
    return screenshotDir;
}

std::string Folders::getUserDataPath()
{
    std::string userDataPath;
#ifdef _WIN32
    char path[MAX_PATH];
    // %APPDATA% (%USERPROFILE%\Application Data)
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        userDataPath = std::string(path) + "/Lugaru/";
    } else {
        return dataDir;
    }
#elif (defined(__APPLE__) && defined(__MACH__))
    const char* homePath = getHomeDirectory();
    if (homePath == NULL) {
        userDataPath = ".";
    } else {
        userDataPath = std::string(homePath) + "/Library/Application Support/Lugaru";
    }
#elif defined(__EMSCRIPTEN__)
    userDataPath = "/home/web_user/.local/share/lugaru";
#else // Linux
    userDataPath = getGenericDirectory("XDG_DATA_HOME", ".local/share");
#endif
    makeDirectory(userDataPath);
    return userDataPath;
}

std::string Folders::getConfigFilePath()
{
    std::string configFolder;
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
    configFolder = getUserDataPath();
#else // Linux
    configFolder = getGenericDirectory("XDG_CONFIG_HOME", ".config");
    makeDirectory(configFolder);
#endif
    return configFolder + "/config.txt";
}

#if PLATFORM_LINUX
/* Generic code for XDG ENVVAR test and fallback */
std::string Folders::getGenericDirectory(const char* ENVVAR, const std::string& fallback)
{
    const char* path = getenv(ENVVAR);
    std::string ret;
    if ((path != NULL) && (strlen(path) != 0)) {
        ret = std::string(path) + "/lugaru";
    } else {
        const char* homedir = getHomeDirectory();
        if ((homedir != NULL) && (strlen(homedir) != 0)) {
            ret = std::string(homedir) + '/' + fallback + "/lugaru";
        } else {
            ret = ".";
        }
    }
    return ret;
}
#endif

#if PLATFORM_UNIX
const char* Folders::getHomeDirectory()
{
    const char* homedir = getenv("HOME");
    if (homedir != NULL) {
        return homedir;
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw != NULL) {
        return pw->pw_dir;
    }
    return NULL;
}
#endif

bool Folders::makeDirectory(const std::string& path)
{
#ifdef _WIN32
    int status = CreateDirectory(path.c_str(), NULL);
    return ((status != 0) || (GetLastError() == ERROR_ALREADY_EXISTS));
#else
    errno = 0;
    int status = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return ((status == 0) || (errno == EEXIST));
#endif
}

FILE* Folders::openMandatoryFile(const std::string& filename, const char* mode)
{
    std::string actualPath = findCaseInsensitiveFile(filename);
    FILE* tfile = fopen(actualPath.c_str(), mode);
    if (tfile == NULL) {
        throw FileNotFoundException(filename);
    }
    return tfile;
}

bool Folders::file_exists(const std::string& filepath)
{
    FILE* file;
    file = fopen(filepath.c_str(), "rb");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

std::string Folders::findCaseInsensitiveFile(const std::string& filepath)
{
    // First try the exact path
    if (file_exists(filepath)) {
        return filepath;
    }

#ifdef __EMSCRIPTEN__
    // For Emscripten, try to find the file with case-insensitive matching
    // Split path into directory and filename
    size_t lastSlash = filepath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return filepath; // No directory, just return original
    }

    std::string directory = filepath.substr(0, lastSlash);
    std::string filename = filepath.substr(lastSlash + 1);

    // Open directory and search for case-insensitive match
    DIR* dir = opendir(directory.c_str());
    if (dir == NULL) {
        return filepath; // Can't open directory, return original
    }

    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    struct dirent* entry;
    std::string result = filepath;
    while ((entry = readdir(dir)) != NULL) {
        std::string entryName = entry->d_name;
        std::string lowerEntry = entryName;
        std::transform(lowerEntry.begin(), lowerEntry.end(), lowerEntry.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (lowerEntry == lowerFilename) {
            result = directory + "/" + entryName;
            break;
        }
    }
    closedir(dir);

    return result;
#else
    // For non-Emscripten, just return the original path
    return filepath;
#endif
}
