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

#ifndef _INCLUDE_SRCDS_SERVERFIX_H_
#define _INCLUDE_SRCDS_SERVERFIX_H_

#include "am-string.h"
#include "GameLib.h"
#include "GameDetector.h"
#include "ErrorReporter.h"
#include "HSGameLib.h"
#include "CDetour/detours.h"

using namespace ke;

// Detours game library functions and performs fixes necessary to run a dedicated server in either
// a command line or GUI mode
class ServerFix
{
public:
    ServerFix(ErrorReporter *reporter);
    ~ServerFix();
    
    void Initialize(const AString *game, EngineBranch engine);
    void Shutdown();

    void SymbolError(const SymbolInfo info[], size_t len, const char *libName);
private:
    ErrorReporter *reporter_;
    CDetour *sysLoadModules_;
    CDetour *loadModule_;
    CDetour *debugString_;
};

#endif // _INCLUDE_SRCDS_SERVERFIX_H_
