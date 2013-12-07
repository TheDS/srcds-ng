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

#include <stdio.h>
#include "stringutil.h"

#include "GameAPI.h"
#include "GameLib.h"
#include "ICommandLine.h"
#include "IGameServerData.h"
#include "ByteBuffer.h"
#include "platform.h"

GameAPI::GameAPI()
    : uimode_(UIMode_GUI),
      detector_(&reporter_),
      serverFix_(&reporter_),
      dataListener_(INVALID_LISTENER_ID),
      currentDataRequest_(0),
      GameCmdLine_(nullptr),
      Plat_FloatTime_(nullptr)
{

}

void GameAPI::SetListener(IGameListener *listener)
{
    listener_ = listener;
    reporter_.SetListener(listener);
}

void GameAPI::SetGame(const char *name)
{
    detector_.OverrideGameName(name);
}

void GameAPI::AddParam(const char *param, const char *value)
{
    GameCmdLine_()->AppendParm(param, value);
}

void GameAPI::BuildGameList(LinkedList<game_t> &gameList)
{
    game_t game;
    FileFindHandle_t handle;
    
    // Load filesystem library and add current directory to search path if necessary
    if (!fileSystem_.IsInitialized())
    {
        fileSystem_.Initialize();
        fileSystem_.AddSearchPath(".", "MAIN", PATH_ADD_TO_HEAD);
    }

    const char *file = fileSystem_.FindFirst("*.*", &handle);
    
    while (file)
    {
        // Find all valid game directories and add them to the list
        if (file[0] != '.' && fileSystem_.FindIsDirectory(handle) && IsValidGameDirectory(file))
        {
            game_t game;
            game.gameDirectory = file;
            game.gameDescription = GetGameDescription(file);
            gameList.append(game);
            
        }
        file = fileSystem_.FindNext(handle);
    }
    
    fileSystem_.FindClose(handle);
}

void GameAPI::BuildMapListForGame(const char *gameDir, LinkedList<AString> &mapList)
{
    FileFindHandle_t handle;
    AString wildcard(gameDir);
    
    wildcard.append(PLATFORM_SEP "maps" PLATFORM_SEP "*.bsp");
    
    // Load filesystem library and add current directory to search path if necessary
    if (!fileSystem_.IsInitialized())
    {
        fileSystem_.Initialize();
        fileSystem_.AddSearchPath(".", "MAIN", PATH_ADD_TO_HEAD);
    }
    
    // Find all maps and add them to the list
    const char *file = fileSystem_.FindFirst(wildcard.chars(), &handle);
    
    while (file)
    {
        // Store map file name without .bsp (4 characters)
        AString map(file, strlen(file) - 4);
        
        mapList.append(map);
        
        file = fileSystem_.FindNext(handle);
    }
    
    fileSystem_.FindClose(handle);
}

bool GameAPI::IsValidGameDirectory(const char *gamedir)
{
    FileFindHandle_t handle;
    AString wildcard(gamedir);
    
    wildcard.append(PLATFORM_SEP "*.*");
    
    const char *file = fileSystem_.FindFirst(wildcard.chars(), &handle);
    
    bool isValid = false;
    bool hasBinaries = false;
    bool hasGameInfo = false;
    
    // A valid game directory contains a 'bin' directory and a file named 'gameinfo.txt'
    while (file)
    {
        if (strcasecmp(file, "bin") == 0)
        {
            if (fileSystem_.FindIsDirectory(handle))
                hasBinaries = true;
            else
                break;
        }
        else if (strcasecmp(file, "gameinfo.txt") == 0)
        {
            if (!fileSystem_.FindIsDirectory(handle))
                hasGameInfo = true;
            else
                break;
        }
        
        if (hasBinaries && hasGameInfo)
        {
            isValid = true;
            break;
        }
        
        file = fileSystem_.FindNext(handle);
    }
    
    fileSystem_.FindClose(handle);
    
    return isValid;
}

// Returns the user-friendly game name from the gameinfo.txt file in the specifed game directory
AString GameAPI::GetGameDescription(const char *gamedir)
{
    AString gameinfo(gamedir);
    
    gameinfo.append("/gameinfo.txt");
    
    FILE *fp = fopen(gameinfo.chars(), "rt");
    
    if (!fp)
        return AString();
    
    char buffer[256], key[128], value[128];
    int level = 0;
    
    // TODO: Need a real parser to handle edge cases
    while (!feof(fp) && fgets(buffer, sizeof(buffer), fp) != nullptr)
    {
        char *pBuf = buffer;
        pBuf = strip_comments(pBuf);
        pBuf = strtrim(pBuf);
        
        if (*pBuf == '{')
            level++;
        else if (*pBuf == '}' && level > 0)
            level--;
        else
        {
            splitkv(pBuf, key, sizeof(key), value, sizeof(value));
        
            // The game name is within the first set of curly braces
            if (level == 1 && strcasecmp(key, "game") == 0)
                return AString(value);
        }
    }
    
    fclose(fp);
    
    return AString();
}

void GameAPI::RunServer(UIMode mode, int argc, char *argv[])
{
    typedef void (*DedicateMainFn)(int, char **);
    DedicateMainFn DedicatedMain;
    
    uimode_ = mode;
    
    // Initialize the game/engine detector
    detector_.Initialize(argc, argv);
    
    const AString& game = detector_.GetGameName();
    if (game.chars() == NULL || game.compare("") == 0)
    {
        reporter_.Error("Could not detect game name. Did you forget to add the -game option?\n");
        return;
    }
    
    if (detector_.GetEngineBranch() == Engine_Unknown)
    {
        reporter_.Error("Unknown game engine detected.\n");
    }
    
    dedicated_.Load("dedicated");
    if (!dedicated_.IsLoaded())
    {
        reporter_.Error("Failed to load dedicated library.\n");
        return;
    }

    serverFix_.Initialize(&detector_.GetGameName(), detector_.GetEngineBranch());
    
    DedicatedMain = dedicated_.ResolveSymbol<DedicateMainFn>("DedicatedMain");
    if (!DedicatedMain)
    {
        reporter_.Error("Failed to locate symbol: DedicatedMain\n");
        return;
    }
    
    // Retrieve the game server data interface from the engine library
    GameLib engine("engine");
    CreateInterfaceFn factory = engine.GetFactory();
    serverData_ = (IGameServerData *)factory(GAMESERVERDATA_INTERFACE_VERSION, NULL);
    
    // Register this as a listener to server data updates
    dataListener_ = serverData_->GetNextListenerID(false);
    serverData_->RegisterAdminUIID(dataListener_);
    
    // Run the dedicated server entry point function
    DedicatedMain(argc, argv);
    
    serverFix_.Shutdown();
}

void GameAPI::StopServer()
{
    // Send the quit command to the server
    ExecCommand("quit");
    
    // Dispatch GUI events
    listener_->OnGameFrame();
}

void GameAPI::ExecCommand(const char *cmd)
{
    ByteBufferWriter buf;
    
    buf.WriteInt(currentDataRequest_++);
    buf.WriteInt(SERVERDATA_EXECCOMMAND);
    buf.WriteString(cmd);
    buf.WriteByte('\0');
    
    serverData_->WriteDataRequest(dataListener_, buf.GetBase(), buf.GetBytesWritten());
}

void GameAPI::SetValue(const char *variable, const char *value)
{
    ByteBufferWriter buf;
    
    buf.WriteInt(currentDataRequest_++);
    buf.WriteInt(SERVERDATA_SETVALUE);
    buf.WriteString(variable);
    buf.WriteString(value);
    
    serverData_->WriteDataRequest(dataListener_, buf.GetBase(), buf.GetBytesWritten());
}

void GameAPI::RequestValue(const char *variable)
{
    ByteBufferWriter buf;
    
    buf.WriteInt(currentDataRequest_++);
    buf.WriteInt(SERVERDATA_REQUESTVALUE);
    buf.WriteString(variable);
    buf.WriteByte('\0');
    
    serverData_->WriteDataRequest(dataListener_, buf.GetBase(), buf.GetBytesWritten());
}

float GameAPI::GetFrameTime()
{
    return Plat_FloatTime_();
}

// Load tier0 library and get pointers to functions used by the API
void GameAPI::LoadTier0()
{
    if (tier0_.IsLoaded())
        return;
    
    tier0_.Load("tier0");
    
    if (!tier0_.IsLoaded())
    {
        reporter_.Error("Failed to load tier0 library.\n");
        return;
    }
    
    // TODO: Add support for other games that have renamed this function (like DOTA 2)
    if ((GameCmdLine_ = tier0_.ResolveSymbol<CmdLineFn>("CommandLine_Tier0")) == nullptr)
    {
        reporter_.Error("Failed to find CommandLine_Tier0 in tier0 library.\n");
        return;
    }
    
    if ((Plat_FloatTime_ = tier0_.ResolveSymbol<TimeFn>("Plat_FloatTime")) == nullptr)
    {
        reporter_.Error("Failed to find Plat_FloatTime in tier0 library.\n");
    }
}

void GameAPI::ProcessServerResponses()
{
    static char rawBuffer[4096];

    while (true)
    {
        int bytesReceived = serverData_->ReadDataResponse(dataListener_,
                                                          rawBuffer, sizeof(rawBuffer));

        if (bytesReceived < 1)
            break;              // All responses have been processed
        
        ByteBufferReader buf(rawBuffer, bytesReceived);
        
        // Skip request number
        buf.Seek(sizeof(int));

        int responseType = buf.ReadInt();
        const char *variable = buf.ReadString();
        
        if (!variable)
            continue;
        
        switch (responseType)
        {
            // Notify the listener that a requested variable value has been received
            case SERVERDATA_RESPONSE_VALUE:
                {
                    int valueSize = buf.ReadInt();
                    ByteBufferWriter valueBuf(valueSize);
                    
                    if (valueSize > 0)
                        valueBuf.Write(buf.GetCurrent(), valueSize);
                    else
                        valueBuf.WriteByte('\0');
                    
                    listener_->OnValueReceived(variable, valueBuf.GetBase());
                }
                break;
            
            // Notify the listener that some kind of server data has been updated
            case SERVERDATA_UPDATE:
                listener_->OnDataUpdated(variable);
                
            default:
                assert(responseType == SERVERDATA_RESPONSE_VALUE ||
                       responseType == SERVERDATA_UPDATE);
                break;
        }
    }
}

UIMode GameAPI::GetUIMode()
{
    return uimode_;
}

GameDetector &GameAPI::GetGameDetector()
{
    return detector_;
}

ErrorReporter &GameAPI::GetErrorReporter()
{
    return reporter_;
}

IGameListener *GameAPI::GetGameListener()
{
    return listener_;
}

IGameAPI *GetGameAPI()
{
    GameAPI *api = &GameAPI::GetInstance();
    
    // Make sure tier0 is loaded for some of the API functions
    api->LoadTier0();
    
    return api;
}
