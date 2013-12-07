/**
 * vim: set ts=4 :
 * =============================================================================
 * Source Dedicated Server NG
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

#include "console.h"

#import <Cocoa/Cocoa.h>

int main(int argc, char *argv[])
{
    bool noGui = false;
    char *execPath = nullptr;

    // Search for the -console command line option if it exists
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-console") == 0)
        {
            noGui = true;
            break;
        }
    }
    
    // Prevent problem where files can't be found when executable path contains spaces by putting
    // quotation marks around it. This can be a problem when running the server under certain
    // versions of GDB. It can also occur under LLDB if only the app bundle path is given as the
    // executable to debug.
    if (argv[0][0] != '"' && strstr(argv[0], " "))
    {
        // Allocate space for path + two quotation marks
        size_t len = strlen(argv[0]) + 3;
        execPath = new char[len];
        
        // Copy original path
        strcpy(&execPath[1], argv[0]);
        
        // Add quotation marks and null terminate
        execPath[0] = execPath[len - 2] = '"';
        execPath[len - 1] = '\0';
        
        argv[0] = execPath;
    }
    
    @autoreleasepool
    {
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        NSString *parentPath = [bundlePath stringByDeletingLastPathComponent];
        
        // Ensure that the current working directory is the path in which the app bundle is located
        chdir([parentPath UTF8String]);
    }
    
    if (noGui)
    {
        // Since the program is running in console mode, ensure that it is considered a background
        // application. This prevents the Steam libraries from inadvertantly causing the SrcDS
        // icon to appear on the OS X dock.
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
        
        return ConsoleMain(argc, argv);
    }
    
    return NSApplicationMain(argc, (const char **)argv);
}
