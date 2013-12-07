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

#ifndef _INCLUDE_SRCDS_IGAMEAPI_H_
#define _INCLUDE_SRCDS_IGAMEAPI_H_

#include "platform.h"
#include "am-string.h"
#include "am-linkedlist.h"

using namespace ke;

// Subclass this in order to listen to game server events
class IGameListener
{
public:
    virtual ~IGameListener() {}
    
    // Called after the dedicated server library has been loaded.
    // This is where command line parameters should be added.
    virtual void OnServerLoaded() {}
    
    // Called when the server has started it's first frame
    virtual void OnServerStarted() {}
    
    // Called when the server has been stopped
    virtual void OnServerStopped() {}
    
    // Called every server frame
    virtual void OnGameFrame() {}
    
    // Called when a requested variable's value has been received
    virtual void OnValueReceived(const char *variable, const char *value) {}
    
    // Called when some piece of server data has been updated - usually the map or player count
    virtual void OnDataUpdated(const char *data) {}
    
    // Called when server has sent a message to the console
    virtual void OnConsoleOutput(const char *msg) {}
    
    // Called when a non-fatal error has occurred
    virtual void OnWarning(const char *msg) {}

    // Called when a fatal error has a occurred. The program will be terminated afterward.
    virtual void OnError(const char *msg) {}
};

enum UIMode
{
    UIMode_Console,             // Command line interface
    UIMode_GUI                  // Graphical user interface
};

struct game_t
{
    AString gameDescription;    // User-friendly game name
    AString gameDirectory;      // Directory in which game resides
};

class IGameAPI
{
public:
    // Hooks up a game listener
    virtual void SetListener(IGameListener *listener) = 0;
    
    // Sets the game directory name for the server
    virtual void SetGame(const char *name) = 0;
    
    // Adds a command line parameter
    virtual void AddParam(const char *param, const char *value) = 0;
    
    // Returns a list of games (descriptions and directory names)
    virtual void BuildGameList(LinkedList<game_t> &gameList) = 0;
    
    // Returns a list of maps for the specified game directory
    virtual void BuildMapListForGame(const char *gameDir, LinkedList<AString> &mapList) = 0;
    
    // Starts up the game server
    virtual void RunServer(UIMode mode, int argc, char *argv[]) = 0;
    
    // Shuts down the game server
    virtual void StopServer() = 0;
    
    // Executes a command on the server
    virtual void ExecCommand(const char *cmd) = 0;
    
    // Sets the value of a console variable
    virtual void SetValue(const char *convar, const char *value) = 0;
    
    // Requests the value of console variable.
    // The value will be returned through IGameListner::OnValueReceived
    virtual void RequestValue(const char *variable) = 0;
    
    // Returns current server frame time
    virtual float GetFrameTime() = 0;
};

// Returns a pointer to the game API interface
SRCDS_API IGameAPI *GetGameAPI();

#endif /* defined(_INCLUDE_SRCDS_IGAMEAPI_H_) */
