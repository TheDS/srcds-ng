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

#ifndef _INCLUDE_SRCDS_GAMEAPI_IMPL_H_
#define _INCLUDE_SRCDS_GAMEAPI_IMPL_H_

#include "IGameAPI.h"
#include "GameLib.h"
#include "GameDetector.h"
#include "ServerFix.h"
#include "FileSystem.h"
#include "ICommandLine.h"
#include "IGameServerData.h"

// IGame implementation
class GameAPI : public IGameAPI
{
public:
    GameAPI();

    void SetListener(IGameListener *listener);
    void SetGame(const char *name);
    void AddParam(const char *param, const char *value);
    
    void BuildGameList(LinkedList<game_t> &gameList);
    void BuildMapListForGame(const char *gameDir, LinkedList<AString> &mapList);

    void RunServer(UIMode mode, int argc, char *argv[]);
    void StopServer();

    void ExecCommand(const char *cmd);
    void SetValue(const char *variable, const char *value);
    void RequestValue(const char *variable);
    float GetFrameTime();
public:
    static inline GameAPI &GetInstance()
    {
        static GameAPI api;
        return api;
    }
    UIMode GetUIMode();
    IGameListener *GetGameListener();
    GameDetector &GetGameDetector();
    ErrorReporter &GetErrorReporter();
    void LoadTier0();
    void ProcessServerResponses();
private:
    bool IsValidGameDirectory(const char *gamedir);
    AString GetGameDescription(const char *gamedir);
private:
    UIMode uimode_;
    IGameListener *listener_;
    ErrorReporter reporter_;
    GameDetector detector_;
    ServerFix serverFix_;
    GameFileSystem fileSystem_;
    GameLib tier0_;
    GameLib dedicated_;

    IGameServerData *serverData_;
    ra_listener_id dataListener_;
    int currentDataRequest_;
private:
    typedef ICommandLine *(*CmdLineFn)(void);
    typedef float (*TimeFn)(void);
    
    CmdLineFn GameCmdLine_;
    TimeFn Plat_FloatTime_;
};

#endif // _INCLUDE_SRCDS_GAMEAPI_IMPL_H_
