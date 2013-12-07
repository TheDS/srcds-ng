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

#include "GameDetector.h"
#include "GameLib.h"

static const char *engineStrings[] =
{
    "s2013",
    "l4d",
    "l4d2",
    "nd",
    //"portal2",
    "csgo",
    "ins",
    //"dota2"
};

GameDetector::GameDetector(ErrorReporter *reporter)
    : initialized_(false), gameEngine_(Engine_Unknown), reporter_(reporter)
{

}

void GameDetector::Initialize(int argc, char *argv[])
{
    if (!initialized_)
    {
        DetectGameName(argc, argv);
        DetectGameEngine();
        initialized_ = true;
    }
}

void GameDetector::OverrideGameName(const char *name)
{
    gameName_ = name;
    
    if (!initialized_)
    {
        DetectGameEngine();
        initialized_ = true;
    }
}

bool GameDetector::IsInitialized() const
{
    return initialized_;
}

const AString& GameDetector::GetGameName() const
{
    return gameName_;
}

EngineBranch GameDetector::GetEngineBranch() const
{
    return gameEngine_;
}

const char *GameDetector::GetEngineString() const
{
    if (gameEngine_ == Engine_Unknown)
        return nullptr;

    return engineStrings[gameEngine_];
}

void GameDetector::DetectGameName(int argc, char *argv[])
{
    // Look for game name on the command line
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-game") == 0 && ++i != argc)
        {
            gameName_ = argv[i];
            break;
        }
    }
}

void GameDetector::DetectGameEngine()
{
    GameLib engine("engine");
    GameLib vstdlib("vstdlib");
    CreateInterfaceFn engineFactory;
    CreateInterfaceFn vstdlibFactory;
    
    if (!engine)
    {
        reporter_->Error("Failed to load engine library.\n");
        return;
    }
    if (!vstdlib)
    {
        reporter_->Error("Failed to load vstdlib library.\n");
        return;
    }
    if ((engineFactory = engine.GetFactory()) == nullptr)
    {
        reporter_->Error("Failed to find engine factory.\n");
        return;
    }
    if ((vstdlibFactory = vstdlib.GetFactory()) == nullptr)
    {
        reporter_->Error("Failed to load vstdlib library.\n");
        return;
    }
    
    // Search for certain interface versions to determine the engine branch
    if (engineFactory("VEngineServer023", nullptr))
    {
        if (engineFactory("IEngineSoundServer004", nullptr))
            gameEngine_ = Engine_Insurgency;
        else
            gameEngine_ = Engine_CSGO;
    }
    else if (engineFactory("VEngineServer022", nullptr) &&
             vstdlibFactory("VEngineCvar007", nullptr))
    {
        GameLib datacache("datacache");
        CreateInterfaceFn datacacheFactory = datacache.GetFactory();
        
        if (datacacheFactory && !datacacheFactory("VPrecacheSystem001", nullptr))
            gameEngine_ = Engine_Left4Dead;
        else if (gameName_.compare("nucleardawn") == 0)
            gameEngine_ = Engine_NuclearDawn;
        else
            gameEngine_ = Engine_Left4Dead2;
    }
    else if (engineFactory("VEngineServer021", nullptr) &&
             engineFactory("VModelInfoServer003", nullptr) &&
             vstdlibFactory("VEngineCvar004", nullptr))
    {
        gameEngine_ = Engine_Source2013;
    }
}
