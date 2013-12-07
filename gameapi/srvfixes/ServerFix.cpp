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

#include "ServerFix.h"
#include "GameAPI.h"

// From Source SDK: Tells the dedicated server which libraries to load and which interfaces to add
struct AppSystemInfo_t
{
    const char *m_pModuleName;      // Name of dynamic library
    const char *m_pInterfaceName;   // Game or engine interface name
};

// From Source SDK: Spew type for DedicatedSpewOutputFunc below
enum SpewType_t
{
	SPEW_MESSAGE = 0,
	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,
    
	SPEW_TYPE_COUNT
};

// From Source SDK: Spew return value for DedicatedSpewOutputFunc below
enum SpewRetval_t
{
	SPEW_DEBUGGER = 0,
	SPEW_CONTINUE,
	SPEW_ABORT
};

// Globals that need to be accessible to the detours below
static const AString *g_GameName;
static EngineBranch g_EngineBranch;
static HSGameLib *g_Dedicated;
static HSGameLib *g_Launcher;
static GameAPI &g_GameAPI = GameAPI::GetInstance();
static ErrorReporter *g_Reporter;
static ServerFix *g_ServerFix;

// Path to app bundle
static char *g_AppBundlePath;
static size_t g_AppBundlePathLen;

// Detours required for any server CLI or GUI
static CDetour *sdlInit;
static CDetour *sdlShutdown;
static CDetour *fileSystemLoadModule;
static CDetour *addSearchPath;

// Detours need for GUI
static CDetour *consoleStartup;
static CDetour *consoleOutput;
static CDetour *processInput;
static CDetour *spewMsg;
static CDetour *runFrame;

void *LoadModule(const char *pModuleName, int *flags);

#include <mach-o/dyld.h>

// Detour for function filesystem_stdio or dedicated libraries
DETOUR_DECL_MEMBER3(CBaseFileSystem_AddSearchPath,
                    void, const char *, pPath, const char *, pPathID, int, addType)
{
    // Search paths must be adjusted to account for the fact that the program binary is inside an
    // app bundle on OS X (SrcDS.app/Contents/MacOS). This app bundle path must be removed if it
    // exists in the given pPath.
    //
    // For example, /SteamApps/common/Counter-Strike Source/Srcds.app/Contents/MacOS/cstrike
    // Should become: /SteamApps/common/Counter-Strike Source/cstrike

    // Get the full length of the path
    size_t len = strlen(pPath);

    // Find the app bundle path
    char *removePos = strcasestr(pPath, g_AppBundlePath);
    
    if (removePos)
    {
        // Calculate length of path string before and including app bundle path
        // In the case of /SteamApps/common/Counter-Strike Source/Srcds.app/Contents/MacOS/cstrike,
        // this should be the length of everything up to the "/cstrike" substring.
        size_t prefixLen = removePos - pPath + g_AppBundlePathLen;
        
        // Move the substring after the app bundle path up to the app bundle path's position
        // In the case of the above example, "/cstrike" would be moved to right after the
        // "Counter-Strike Source" substring.
        memmove(removePos, pPath + prefixLen, len - prefixLen + 1);
    }
    
    // Call original function with the modified path
    DETOUR_MEMBER_CALL(CBaseFileSystem_AddSearchPath)(pPath, pPathID, addType);
}

// Detour for function in tier0 library
DETOUR_DECL_STATIC1(Plat_DebugString, void, const char *, str)
{
    // Doing nothing here prevents duplicate message from being printed in the terminal
}

// Detour for function in launcher library
DETOUR_DECL_MEMBER0(CSDLMgr_Init, int)
{
    // Prevent SDL from initializing to avoid invoking OpenGL
    return 1;
}

// Detour for function in launcher library
DETOUR_DECL_MEMBER0(CSDLMgr_Shutdown, void)
{
    // Nothing to do here
}

// Detour for function in dedicated library
DETOUR_DECL_STATIC1(ConsoleStartup, bool, CreateInterfaceFn, dedicatedFactory)
{
    // Dispatch GUI events
    g_GameAPI.GetGameListener()->OnGameFrame();

    return true;
}

// Detour for function in dedicated library
DETOUR_DECL_MEMBER1(CSys_ConsoleOutput, void, const char *, string)
{
    g_GameAPI.GetGameListener()->OnConsoleOutput(string);
    
    // Dispatch GUI events
    g_GameAPI.GetGameListener()->OnGameFrame();
}

// Detour for function in dedicated library
DETOUR_DECL_STATIC0(ProcessConsoleInput, void)
{
    // Nothing to do here
}

// Detour for function in dedicated library
DETOUR_DECL_STATIC2(DedicatedSpewOutputFunc, SpewRetval_t, SpewType_t, spewType, const char *, msg)
{
    if (spewType == SPEW_ERROR)
    {
        g_GameAPI.GetGameListener()->OnError(msg);
        return SPEW_ABORT;
    }
    
    return DETOUR_STATIC_CALL(DedicatedSpewOutputFunc)(spewType, msg);
}

// Detour for function in engine library
DETOUR_DECL_MEMBER0(CDedicatedServerAPI_RunFrame, bool)
{
    static bool waiting = true;
    static int count = 0;
    
    // Wait until all the game/engine libraries have been loaded to trigger this
    if (waiting && ++count == 4)
    {
        g_GameAPI.GetGameListener()->OnServerStarted();
        waiting = false;
    }
    
    // Run the listener's frame first
    g_GameAPI.GetGameListener()->OnGameFrame();
    
    // Run original frame
    bool res = DETOUR_MEMBER_CALL(CDedicatedServerAPI_RunFrame)();
    
    if (res == false)
    {
        // This was the last frame, so notify listener
        g_GameAPI.GetGameListener()->OnServerStopped();
        return res;
    }
    
    g_GameAPI.ProcessServerResponses();
    
    // Dispatch GUI events
    g_GameAPI.GetGameListener()->OnGameFrame();
    
    return res;
}

// Detour for function in filesystem_stdio or dedicated libraries
DETOUR_DECL_STATIC2(Sys_LoadModuleFlags, void *, const char *, pModuleName, int, flags)
{
    return LoadModule(pModuleName, &flags);
}

// Detour for function in filesystem_stdio or dedicated libraries
DETOUR_DECL_STATIC1(Sys_LoadModule, void *, const char *, pModuleName)
{
    return LoadModule(pModuleName, nullptr);
}

// Function to call for the two Sys_LoadModule detours to avoid duplication of logic
void *LoadModule(const char *pModuleName, int *flags)
{
    void *handle = nullptr;
    
    // Avoid NSAutoreleasepool leaks from libcef
    if (strstr(pModuleName, "chromehtml"))
        return nullptr;
    
    // TODO: Fix library extension if it doesn't end with correct platform extension. This is
    //       necessary for Left 4 Dead 1.
    // char module[PATH_MAX];
    // pModuleName = FixLibraryExt(pModuleName, module, sizeof(module));
    
#if defined(PLATFORM_MACOSX)
    if (g_EngineBranch == Engine_CSGO && strstr(pModuleName, "matchmaking_ds.dylib"))
        pModuleName = "matchmaking.dylib";
#endif
    
    if (flags)
        handle = DETOUR_STATIC_CALL(Sys_LoadModuleFlags)(pModuleName, *flags);
    else
        handle = DETOUR_STATIC_CALL(Sys_LoadModule)(pModuleName);
    
    return handle;
}

// Detour for function in dedicated library.
// This detour is particularly important because it sets up many of the other detours above.
DETOUR_DECL_MEMBER1(CSys_LoadModules, bool, void *, appsys)
{
    typedef void (*AddSystemFunc)(void *, void *, const char *);
    typedef bool (*AddSystemsFunc)(void *, AppSystemInfo_t *);
    typedef void *(*CreateMgrFunc)(void);
    
    HSGameLib *fileSystem = nullptr;

    g_Launcher = new HSGameLib("launcher");
    if (!g_Launcher->IsValid())
    {
        g_Reporter->Error("Failed to get symbol table for launcher library.\n");
        return false;
    }
    
    // Notify listener that the server has been loaded
    g_GameAPI.GetGameListener()->OnServerLoaded();
    
    SymbolInfo info[5];
    memset(info, 0, sizeof(info));
    const char *symbols[] =
    {
        "_ZN15CAppSystemGroup9AddSystemEP10IAppSystemPKc",
        "_ZN15CAppSystemGroup10AddSystemsEP15AppSystemInfo_t",
        "_Z12CreateSDLMgrv",
        "_ZN7CSDLMgr4InitEv",
        "_ZN7CSDLMgr8ShutdownEv",
        nullptr
    };
    
    // Find the above hidden symbols
    size_t notFound = g_Launcher->ResolveHiddenSymbols(info, symbols);
    size_t neededSymbols = ARRAY_LENGTH(symbols) - 1;
    bool usesSdl = true;
    
    // Some games may have an alternative symbol that should be used instead
    if (!info[2].address)
    {
        const char *altSymbol = "_Z15CreateCCocoaMgrv";
        void *altAddress = g_Launcher->ResolveHiddenSymbol<void *>(altSymbol);
        
        if (altAddress)
        {
            info[2].name = altSymbol;
            info[2].address = altAddress;
            notFound -= 3;
            neededSymbols -= 2;
            usesSdl = false;
        }
    }
    
    if (notFound != 0)
    {
        g_ServerFix->SymbolError(info, neededSymbols, "launcher");
        return false;
    }
    
    AddSystemFunc addSystem = (AddSystemFunc)info[0].address;
    AddSystemsFunc addSystems = (AddSystemsFunc)info[1].address;
    CreateMgrFunc createManager = (CreateMgrFunc)info[2].address;
    
    // If the game uses SDL, block a couple functions to avoid invoking OpenGL
    if (usesSdl)
    {
        sdlInit = DETOUR_CREATE_MEMBER(CSDLMgr_Init, info[3].address);
        if (!sdlInit)
        {
            g_Reporter->Error("Failed to create detour for CSDLMgr::Init");
            return false;
        }
        
        sdlShutdown = DETOUR_CREATE_MEMBER(CSDLMgr_Shutdown, info[4].address);
        if (!sdlShutdown)
        {
            g_Reporter->Error("Failed to create detour for CSDLMgr::Shutdown");
            return false;
        }
        
        sdlInit->EnableDetour();
        sdlShutdown->EnableDetour();
    }
    
    // Create SDL or Cocoa manager interface for the engine
    void *managerInterface = createManager();
    
    if (!managerInterface)
    {
        g_Reporter->Error("Failed to create %s manager interface", usesSdl ? "SDL" : "Cocoa");
        return false;
    }
    
    // Add SDL or Cocoa manager interface to the game's app system
    if (usesSdl)
        addSystem(appsys, managerInterface, "SDLMgrInterface001");
    else
        addSystem(appsys, managerInterface, "CocoaMgrInterface006");
    
    // Only detour filesystem_stdio functions if the dedicated library isn't being overridden
    if (!fileSystemLoadModule && g_Dedicated->IsOverridden())
    {
        void *p = nullptr;
        fileSystem = new HSGameLib("filesystem_stdio");
        
        p = fileSystem->ResolveHiddenSymbol<void *>("_Z14Sys_LoadModulePKc");
        if (p)
        {
            fileSystemLoadModule = DETOUR_CREATE_STATIC(Sys_LoadModule, p);
        }
        else
        {
            p = fileSystem->ResolveHiddenSymbol<void *>("_Z14Sys_LoadModulePKc9Sys_Flags");
            if (p)
                fileSystemLoadModule = DETOUR_CREATE_STATIC(Sys_LoadModuleFlags, p);
        }
        
        if (p)
            fileSystemLoadModule->EnableDetour();
        else
            g_Reporter->Warning("Failed to create detour for Sys_LoadModule in filesystem_stdio\n");
        
        // TODO: Detour this in dedicated.dylib if it exists
        if (!addSearchPath)
        {
            p = fileSystem->ResolveHiddenSymbol<void *>
                                   ("_ZN15CBaseFileSystem13AddSearchPathEPKcS1_15SearchPathAdd_t");

            if (!p)
            {
                g_Reporter->Error("Failed to locate symbol for CBaseFileSystem::AddSearchPath\n");
                return nullptr;
            }
            
            addSearchPath = DETOUR_CREATE_MEMBER(CBaseFileSystem_AddSearchPath, p);
            if (!addSearchPath)
            {
                g_Reporter->Error("Failed to create detour for CBaseFileSystem::AddSearchPath\n");
                return nullptr;
            }
            
            addSearchPath->EnableDetour();
        }
    }
    
    // Add additional systems before calling the original function if dedicated lib not overridden
    if (!g_Dedicated->IsOverridden())
    {
        // FIXME: DYLIB extension is OS X specific
        AppSystemInfo_t preSystems[] =
        {
            {"inputsystem.dylib", "InputSystemVersion001"},
            {"", ""},
            {"", ""},
        };
        
        if (g_EngineBranch == Engine_CSGO)
        {
            preSystems[1].m_pModuleName = "soundemittersystem.dylib";
            preSystems[1].m_pInterfaceName = "VSoundEmitter003";
        }
        
        if (!addSystems(appsys, preSystems))
            return false;
    }
    
    // Call original version of this function to load other app systems
    if (!DETOUR_MEMBER_CALL(CSys_LoadModules)(appsys))
        return false;
    
    delete fileSystem;
    
    HSGameLib engine("engine");
    void **engineManagerInterface = nullptr;
    
    // There are two possible names for this interface pointer
    engineManagerInterface = engine.ResolveHiddenSymbol<void **>("g_pLauncherMgr");
    if (!engineManagerInterface)
        engineManagerInterface = engine.ResolveHiddenSymbol<void **>("g_pLauncherCocoaMgr");
    
    if (!engineManagerInterface)
    {
        g_Reporter->Error("Failed to locate symbol for engine's pointer to %s manager interface",
                          usesSdl ? "SDL" : "Cocoa");
        return false;
    }
    
    // Set pointer to SDL/Cocoa manager interface in engine to prevent a crash
    *engineManagerInterface = managerInterface;
    
    // Set up frame detour for GUI mode
    if (g_GameAPI.GetUIMode() == UIMode_GUI)
    {
        void *frameFunc = engine.ResolveHiddenSymbol<void *>("_ZN19CDedicatedServerAPI8RunFrameEv");
        if (frameFunc)
        {
            runFrame = DETOUR_CREATE_MEMBER(CDedicatedServerAPI_RunFrame, frameFunc);
            if (runFrame)
            {
                runFrame->EnableDetour();
            }
            else
            {
                g_Reporter->Error("Failed to create detour for CDedicatedServerAPI::RunFrame in "
                                  "engine library\n");
                return false;
            }
        }
    }
    
    if (!g_Dedicated->IsOverridden())
    {
        // FIXME: DYLIB extension is OS X specific
        AppSystemInfo_t postSystems[] =
        {
            {"vguimatsurface.dylib", "VGUI_Surface030"},
            {"vgui2.dylib",          "VGUI_ivgui008"},
            {"", ""}
        };
        
        // TODO: Increment the interface version rather than relying on this in the future
        if (g_EngineBranch == Engine_NuclearDawn ||
            g_EngineBranch == Engine_Left4Dead2 ||
            g_EngineBranch == Engine_CSGO)
            postSystems[0].m_pInterfaceName = "VGUI_Surface031";
        
        // Add more systems after the ones added by the original functions
        if (!addSystems(appsys, postSystems))
            return false;
    }

    // HACK: Get current executable path and strip off both the app bundle path and the executable
    //       name. This is done to refer to the parent directory of the app bundle (SrcDS.app). The
    //       working directory is then changed to this new path in order to avoid a problem where
    //       the game is unable to find its files and crash.
    char exePath[PATH_MAX];
    uint32_t size = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &size) == 0)
    {
        // The path returned by _NSGetExecutablePath could be a symbolic link, so get the real path
        char linkPath[PATH_MAX];
        if (realpath(exePath, linkPath))
        {
            int numSeps = 0;
            size_t pathLen = strlen(linkPath);
            
            // Starting from the end of the path string, find the 4th path separator
            for (size_t i = pathLen - 1; i < pathLen; i--)
            {
                if (linkPath[i] == PLATFORM_SEP_CHAR)
                {
                    numSeps++;
                    
                    if (numSeps == 1)
                    {
                        // Strip the executable name.
                        // This is required because the app bundle path will be copied later
                        linkPath[i] = '\0';
                    }
                    else if (numSeps == 4)
                    {
                        // Copy the app bundle path
                        g_AppBundlePathLen = strlen(&linkPath[i]);
                        g_AppBundlePath = new char[g_AppBundlePathLen + 1];
                        strcpy(g_AppBundlePath, &linkPath[i]);

                        // Now strip the app bundle path
                        linkPath[i] = '\0';
                        
                        // Change working directory to this new path
                        chdir(linkPath);
                        
                        break;
                    }
                }
            }
        }
    }

    return true;
}


ServerFix::ServerFix(ErrorReporter *reporter)
    : reporter_(reporter)
{

}

ServerFix::~ServerFix()
{
    delete g_Dedicated;
    delete g_Launcher;
    delete g_AppBundlePath;
}

void ServerFix::Initialize(const AString *game, EngineBranch engine)
{
    g_Dedicated = new HSGameLib("dedicated");
    g_GameName = game;
    g_EngineBranch = engine;
    
    g_ServerFix = this;
    g_Reporter = reporter_;
    
    if (!g_Dedicated->IsValid())
    {
        reporter_->Error("Failed to get symbol table for dedicated library.\n");
        return;
    }
    
    SymbolInfo info[6];
    memset(&info, 0, sizeof(info));
    const char *symbols[] =
    {
        "_ZN4CSys11LoadModulesEP24CDedicatedAppSystemGroup",
        "_Z14Sys_LoadModulePKc",
        "_Z14ConsoleStartupPFPvPKcPiE",
        "_ZN4CSys13ConsoleOutputEPKc",
        "_Z19ProcessConsoleInputv",
        "_Z23DedicatedSpewOutputFunc10SpewType_tPKc",
        nullptr
    };

    size_t notFound = g_Dedicated->ResolveHiddenSymbols(info, symbols);
    
    // If Sys_LoadModule wasn't found, try an alternative version
    bool altLoadModule = false;
    if (!info[1].address)
    {
        const char *altSymbol = "_Z14Sys_LoadModulePKc9Sys_Flags";
        void *altAddress = g_Dedicated->ResolveHiddenSymbol<void *>(altSymbol);
        
        if (altAddress)
        {
            info[1].name = altSymbol;
            info[1].address = altAddress;
            altLoadModule = true;
            notFound--;
        }
    }
    
    if (notFound > 0)
    {
        SymbolError(info, ARRAY_LENGTH(info), "dedicated");
        return;
    }
    
    // If CreateCCocoaMgr() or CreateSDLMgr() exist in the dedicated library, then the following
    // hooks and fixes are probably not necessary
    if (!g_Dedicated->ResolveHiddenSymbol<void *>("_Z15CreateCCocoaMgrv") &&
        !g_Dedicated->ResolveHiddenSymbol<void *>("_Z12CreateSDLMgrv"))
    {
        sysLoadModules_ = DETOUR_CREATE_MEMBER(CSys_LoadModules, info[0].address);
        if (!sysLoadModules_)
        {
            reporter_->Error("Failed to create detour for CSys::LoadModules\n");
            return;
        }
        sysLoadModules_->EnableDetour();
    }
    
    if (altLoadModule)
        loadModule_ = DETOUR_CREATE_STATIC(Sys_LoadModuleFlags, info[1].address);
    else
        loadModule_ = DETOUR_CREATE_STATIC(Sys_LoadModule, info[1].address);
    
    if (!loadModule_)
    {
        reporter_->Error("Failed to create detour for Sys_LoadModule in dedicated library\n");
        return;
    }
    loadModule_->EnableDetour();
    
    // Set up detours need for the GUI
    if (g_GameAPI.GetUIMode() == UIMode_GUI)
    {
        consoleStartup = DETOUR_CREATE_STATIC(ConsoleStartup, info[2].address);
        if (consoleStartup)
        {
            consoleStartup->EnableDetour();
        }
        else
        {
            reporter_->Error("Failed to create detour for ConsoleStartup in dedicated library\n");
            return;
        }
        
        consoleOutput = DETOUR_CREATE_MEMBER(CSys_ConsoleOutput, info[3].address);
        if (consoleOutput)
        {
            consoleOutput->EnableDetour();
        }
        else
        {
            reporter_->Error("Failed to create detour for CSys::ConsoleOutput in dedicated "
                             "library\n");
            return;
        }
        
        processInput = DETOUR_CREATE_STATIC(ProcessConsoleInput, info[4].address);
        if (processInput)
        {
            processInput->EnableDetour();
        }
        else
        {
            reporter_->Error("Failed to create detour for ProcessConsoleInput in dedicated "
                             "library\n");
            return;
        }
        
        spewMsg = DETOUR_CREATE_STATIC(DedicatedSpewOutputFunc, info[5].address);
        if (spewMsg)
        {
            spewMsg->EnableDetour();
        }
        else
        {
            reporter_->Error("Failed to create detour for DedicatedSpewOutputFunc in dedicated "
                             "library\n");
            return;
        }
    }

    GameLib tier0("tier0");
    if (tier0.IsLoaded())
    {
        void *debugStringAddr = tier0.ResolveSymbol<void *>("Plat_DebugString");
        debugString_ = DETOUR_CREATE_STATIC(Plat_DebugString, debugStringAddr);
        
        if (debugString_)
            debugString_->EnableDetour();
    }
}

void ServerFix::Shutdown()
{
    // Remove all detours if necessary
    
    if (sdlInit)
        sdlInit->Destroy();
    if (sdlShutdown)
        sdlShutdown->Destroy();
    if (fileSystemLoadModule)
        fileSystemLoadModule->Destroy();
    if (addSearchPath)
        addSearchPath->Destroy();
    
    // Detours for GUI mode
    if (g_GameAPI.GetUIMode() == UIMode_GUI)
    {
        if (consoleStartup)
            consoleStartup->Destroy();
        if (consoleOutput)
            consoleOutput->Destroy();
        if (processInput)
            processInput->Destroy();
        if (spewMsg)
            spewMsg->Destroy();
        if (runFrame)
            runFrame->Destroy();
    }
    
    if (sysLoadModules_)
        sysLoadModules_->Destroy();
    if (loadModule_)
        loadModule_->Destroy();
    if (debugString_)
        debugString_->Destroy();
    
}

void ServerFix::SymbolError(const SymbolInfo info[], size_t len, const char *libName)
{
    AString symbols("");
    
    for (size_t i = 0; i < len; i++)
    {
        if (!info[i].address && info[i].name)
        {
            symbols.append(info[i].name);
            symbols.append("\n");
        }
    }
    
    reporter_->Error("Failed to locate the following symbols for %s library:\n%s",
                     libName,
                     symbols.chars());
}
