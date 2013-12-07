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

#include <dlfcn.h>
#include <mach/task.h>
#include <mach-o/dyld_images.h>
#include <mach-o/loader.h>
#include "HSGameLib.h"

static struct dyld_all_image_infos *GetDyldImageInfo()
{
    static struct dyld_all_image_infos *infos = NULL;
    
    if (!infos)
    {
        task_dyld_info_data_t dyld_info;
        mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
        
        if (task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&dyld_info, &count)
            == KERN_SUCCESS)
        {
            infos = (struct dyld_all_image_infos *)dyld_info.all_image_info_addr;
        }
        else
        {
            struct nlist symList[2];
            memset(symList, 0, sizeof(symList));

            symList[0].n_un.n_name = (char *)"_dyld_all_image_infos";
            nlist("/usr/lib/dyld", symList);

            infos = (struct dyld_all_image_infos *)symList[0].n_value;
        }
    }
    
    return infos;
}

HSGameLib::HSGameLib()
    : GameLib(), baseAddress_(0), lastPosition_(0), valid_(false)
{

}

HSGameLib::HSGameLib(const char *name)
    : GameLib(name), baseAddress_(0), lastPosition_(0), valid_(false)
{
    if (!IsLoaded())
        return;

    Initialize();
}

bool HSGameLib::Load(const char *name)
{
    if (IsLoaded())
        Close();
    
    if (GameLib::Load(name))
    {
        Invalidate();
        Initialize();
    }
    
    return IsValid();
}

bool HSGameLib::IsValid() const
{
    return valid_;
}

size_t HSGameLib::ResolveHiddenSymbols(SymbolInfo *list, const char **names)
{
    size_t invalid = 0;
    
    while (*names && *names[0])
    {
        SymbolInfo *info = list++;
        const char *name = *names;

        void *addr = GetHiddenSymbolAddr(name);
        
        info->name = name;
        
        if (addr)
            info->address = addr;
        else
            invalid++;
        
        names++;
    }
    
    return invalid;
}

void HSGameLib::Initialize()
{
    struct mach_header *fileHdr;
    struct load_command *loadCmds;
    struct segment_command *linkEditHdr = nullptr;
    struct symtab_command *symTableHdr = nullptr;
    uint32_t loadCmdCount = 0;
    uintptr_t linkEditAddr = 0;
    
    baseAddress_ = GetBaseAddress();
    
    if (!baseAddress_)
        return;
    
    // Initialize symbol hash table
    table_.Initialize();

    fileHdr = (struct mach_header *)baseAddress_;
    loadCmds = (struct load_command *)(baseAddress_ + sizeof(mach_header));
    loadCmdCount = fileHdr->ncmds;
    
    for (uint32_t i = 0; i < loadCmdCount; i++)
    {
        if (loadCmds->cmd == LC_SEGMENT && !linkEditHdr)
        {
            struct segment_command *seg = (struct segment_command *)loadCmds;
            if (strcmp(seg->segname, "__LINKEDIT") == 0)
            {
                linkEditHdr = seg;
                
                // If the symbol table header has also been found, then the search can stop
                if (symTableHdr)
                    break;
            }
        }
        else if (loadCmds->cmd == LC_SYMTAB)
        {
            symTableHdr = (struct symtab_command *)loadCmds;
            
            if (linkEditHdr)
                break;
        }
        
        loadCmds = (struct load_command *)(uintptr_t(loadCmds) + loadCmds->cmdsize);
    }
    
    if (!linkEditHdr || !symTableHdr || !symTableHdr->symoff || !symTableHdr->stroff)
        return;
    
    linkEditAddr = baseAddress_ + linkEditHdr->vmaddr;
    symbolTable_ = (RawSymbolTable)(linkEditAddr + symTableHdr->symoff - linkEditHdr->fileoff);
    stringTable_ = (const char *)(linkEditAddr + symTableHdr->stroff - linkEditHdr->fileoff);
    symbolCount_ = symTableHdr->nsyms;
    
    valid_ = true;
}

void HSGameLib::Invalidate()
{
    if (!table_.IsEmpty())
        table_.Destroy();

    valid_ = false;
}

uintptr_t HSGameLib::GetBaseAddress()
{
    Dl_info info;
    void *factory;
    uintptr_t base = 0;
    
    factory = (void *)GetFactory();
    
    // First try using dladdr() with the public factory symbol
    if (factory)
    {
        if (dladdr((void *)factory, &info) && info.dli_fbase && info.dli_fname)
            base = (uintptr_t)info.dli_fbase;
    }
    
    // If the library doesn't have a factory symbol or dladdr() failed, try looking through all the
    // libraries loaded in the process for a matching handle.
    if (!base)
    {
        struct dyld_all_image_infos *infos = GetDyldImageInfo();
        
        if (infos)
        {
            uint32_t imageCount = infos->infoArrayCount;
        
            for (uint32_t i = 1; i < imageCount; i++)
            {
                const struct dyld_image_info &info = infos->infoArray[i];
            
                void *handle = dlopen(info.imageFilePath, RTLD_NOLOAD);
                if (handle == handle_)
                {
                    base = (uintptr_t)info.imageLoadAddress;
                    dlclose(handle);
                    break;
                }
            }
        }
    }
    
    return base;
}

void *HSGameLib::GetHiddenSymbolAddr(const char *symbol)
{
    Symbol *entry;

    if (!baseAddress_)
        return nullptr;
    
    // In the best case, the symbol has already been cached
    entry = table_.FindSymbol(symbol, strlen(symbol));
    if (entry)
        return entry->address;
    
    for (uint32_t i = lastPosition_; i < symbolCount_; i++)
    {
        struct nlist &sym = symbolTable_[i];
        
        // Ignore the prepended underscore on all symbols to match dlsym() functionality
        const char *symName = stringTable_ + sym.n_un.n_strx + 1;
        
        // Skip undefined symbls
        if (sym.n_sect == NO_SECT)
            continue;
        
        Symbol *currentSymbol;
        currentSymbol = table_.InternSymbol(symName,
                                            strlen(symName),
                                            (void *)(baseAddress_ + sym.n_value));
        
        if (strcmp(symbol, symName) == 0)
        {
            entry = currentSymbol;
            lastPosition_ = ++i;
            break;
        }
    }
    
    return entry ? entry->address : nullptr;
}


