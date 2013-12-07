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

#include "FileSystem.h"
#include "GameAPI.h"

GameFileSystem::GameFileSystem() : pFileSystem_(nullptr)
{

}

GameFileSystem::~GameFileSystem()
{
    delete lib_;
}

void GameFileSystem::Initialize()
{
    CreateInterfaceFn factory = nullptr;
    
    lib_ = new HSGameLib("dedicated");
    
    if (lib_->IsValid())
        factory = lib_->ResolveHiddenSymbol<CreateInterfaceFn>("_Z17FileSystemFactoryPKcPi");
    
    if (!factory)
    {
        lib_->Load("filesystem_stdio");
        
        if (lib_->IsValid())
            factory = lib_->GetFactory();
    }
    
    if (!factory)
    {
        ErrorReporter &reporter = GameAPI::GetInstance().GetErrorReporter();
        reporter.Error("Failed to find the game's filesystem library. Please make sure that the "
                       "app is located in the main folder of a Source engine based game.\n");
        return;
    }
    
    // TODO: Account for other games that may have a different version number
    pFileSystem_ = factory("VFileSystem022", nullptr);
    
    if (pFileSystem_ == nullptr)
    {
        ErrorReporter &reporter = GameAPI::GetInstance().GetErrorReporter();
        reporter.Error("Failed to find the game's filesystem interface.\n");
    }
    
    // Initialize pointers to filesystem functions
    InitializeFunctions();
}

bool GameFileSystem::IsInitialized()
{
    return pFileSystem_ != nullptr;
}

void GameFileSystem::AddSearchPath(const char *path, const char *pathID, SearchPathAdd_t addType)
{
    AddSearchPath_(pFileSystem_, path, pathID, addType);
}

const char *GameFileSystem::FindFirst(const char *wildcard, FileFindHandle_t *handle)
{
    return FindFirst_(pFileSystem_, wildcard, handle);
}

const char *GameFileSystem::FindNext(FileFindHandle_t handle)
{
    return FindNext_(pFileSystem_, handle);
}

bool GameFileSystem::FindIsDirectory(FileFindHandle_t handle)
{
    return FindIsDirectory_(pFileSystem_, handle);
}

void GameFileSystem::FindClose(FileFindHandle_t handle)
{
    FindClose_(pFileSystem_, handle);
}

void GameFileSystem::InitializeFunctions()
{
    const char *symbols[] =
    {
        "_ZN15CBaseFileSystem13AddSearchPathEPKcS1_15SearchPathAdd_t",
        "_ZN15CBaseFileSystem9FindFirstEPKcPi",
        "_ZN15CBaseFileSystem8FindNextEi",
        "_ZN15CBaseFileSystem15FindIsDirectoryEi",
        "_ZN15CBaseFileSystem9FindCloseEi",
        nullptr
    };
    
    SymbolInfo info[5];
    memset(&info, 0, sizeof(info));
    
    size_t notFound = lib_->ResolveHiddenSymbols(info, symbols);
    
    if (notFound > 0)
    {
        ErrorReporter &reporter = GameAPI::GetInstance().GetErrorReporter();
        
        AString symbols("");
        
        // Build list of symbols for error string below
        for (size_t i = 0; i < ARRAY_LENGTH(info); i++)
        {
            if (!info[i].address && info[i].name)
            {
                symbols.append(info[i].name);
                symbols.append("\n");
            }
        }
        
        reporter.Error("Failed to locate the following symbols for %s:\n%s",
                        lib_->GetName().chars(),
                        symbols.chars());

        return;
    }
    
    AddSearchPath_ = (AddSearchPathFn)info[0].address;
    FindFirst_ = (FindFirstFn)info[1].address;
    FindNext_ = (FindNextFn)info[2].address;
    FindIsDirectory_ = (FindIsDirectoryFn)info[3].address;
    FindClose_ = (FindCloseFn)info[4].address;
}