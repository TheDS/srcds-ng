/**
 * vim: set ts=4 :
 * =============================================================================
 * Source Dedicated Server NG - Game API Library
 * Copyright (C) 2011-2013 Scott Ehlert and AlliedModders LLC.
 * All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "Steamworks SDK," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.
 */

#ifndef _INCLUDE_SRCDS_FILESYSTEM_H_
#define _INCLUDE_SRCDS_FILESYSTEM_H_

#include "HSGameLib.h"

typedef int FileFindHandle_t;

enum SearchPathAdd_t
{
    PATH_ADD_TO_HEAD,                // First path searched
    PATH_ADD_TO_TAIL,                // Last path searched
};

// Wrapper for game filesystem functions
class GameFileSystem
{
public:
    GameFileSystem();
    ~GameFileSystem();
    
    void Initialize();
    bool IsInitialized();
    
    void AddSearchPath(const char *path, const char *pathID, SearchPathAdd_t addType);
    
    const char *FindFirst(const char *wildcard, FileFindHandle_t *handle);
    const char *FindNext(FileFindHandle_t handle);
    bool FindIsDirectory(FileFindHandle_t handle);
    void FindClose(FileFindHandle_t handle);
private:
    void InitializeFunctions();
private:
    HSGameLib *lib_;
    void *pFileSystem_;

    typedef bool *(*AddSearchPathFn)(void *, const char *, const char *, SearchPathAdd_t);
    typedef const char *(*FindFirstFn)(void *, const char *, FileFindHandle_t *);
    typedef const char *(*FindNextFn)(void *, FileFindHandle_t);
    typedef bool (*FindIsDirectoryFn)(void *, FileFindHandle_t);
    typedef void (*FindCloseFn)(void *, FileFindHandle_t);
    
    AddSearchPathFn AddSearchPath_;
    FindFirstFn FindFirst_;
    FindNextFn FindNext_;
    FindIsDirectoryFn FindIsDirectory_;
    FindCloseFn FindClose_;
};

#endif // _INCLUDE_SRCDS_FILESYSTEM_H_
