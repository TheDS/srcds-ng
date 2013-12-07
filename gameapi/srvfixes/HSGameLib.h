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

#ifndef _INCLUDE_SRCDS_HSGAMELIB_H_
#define _INCLUDE_SRCDS_HSGAMELIB_H_

#include "GameLib.h"
#include "sm_symtable.h"
#include "am-string.h"

#if defined(PLATFORM_LINUX)
typedef Elf32_Sym RawSymbolTable;
#elif defined(PLATFORM_MACOSX)
#include <mach-o/nlist.h>
typedef struct nlist *RawSymbolTable;
#endif

struct SymbolInfo
{
    const char *name;
    void *address;
};

// GameLib subclass capable of finding symbols hidden via gcc or clangs -fvisibility=hidden option
class HSGameLib : public GameLib
{
public:
    HSGameLib();
    explicit HSGameLib(const char *name);
    
    bool Load(const char *name);
    bool IsValid() const;
    
    template <typename T>
    T ResolveHiddenSymbol(const char *symbol)
    {
        return reinterpret_cast<T>(GetHiddenSymbolAddr(symbol));
    }
    
    size_t ResolveHiddenSymbols(SymbolInfo *list, const char **names);
private:
    void Initialize();
    void Invalidate();
    uintptr_t GetBaseAddress();
    void *GetHiddenSymbolAddr(const char *symbol);
private:
    SymbolTable table_;
    uintptr_t baseAddress_;
    uint32_t lastPosition_;
    RawSymbolTable symbolTable_;
    const char *stringTable_;
    uint32_t symbolCount_;
    bool valid_;
};

#endif // _INCLUDE_SRCDS_HSGAMELIB_H_
