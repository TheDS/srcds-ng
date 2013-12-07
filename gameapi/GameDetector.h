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

#ifndef _INCLUDE_SRCDS_GAMEDETECTOR_H_
#define _INCLUDE_SRCDS_GAMEDETECTOR_H_

#include "am-string.h"
#include "ErrorReporter.h"

using namespace ke;

enum EngineBranch
{
    Engine_Unknown = -1,
    Engine_Source2013,      // AKA Orange Box or Source 2009
    Engine_Left4Dead,
    Engine_Left4Dead2,
    Engine_NuclearDawn,
    //Engine_Portal2        // TODO: Add support
    Engine_CSGO,
    Engine_Insurgency,
    //Engine_DOTA2          // TODO: Add support
};

// Detects game and Source engine branch
class GameDetector
{
public:
    GameDetector(ErrorReporter *reporter);
    
    void Initialize(int argc, char *argv[]);
    void OverrideGameName(const char *name);
    
    bool IsInitialized() const;
    
    const AString &GetGameName() const;
    const char *GetEngineString() const;
    EngineBranch GetEngineBranch() const;
private:
    void DetectGameName(int argc, char *argv[]);
    void DetectGameEngine();
private:
    bool initialized_;
    EngineBranch gameEngine_;
    AString gameName_;
    ErrorReporter *reporter_;
};


#endif // _INCLUDE_SRCDS_GAMEDETECTOR_H_
