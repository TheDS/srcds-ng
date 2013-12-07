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

#import <Cocoa/Cocoa.h>
#import "SDGameAPIProxy.h"

//
// Controller for ServerWindow
//

@interface ServerWindowController : NSWindowController
                                    <NSWindowDelegate,
                                    NSTableViewDataSource,
                                    SDGameDelegate>
{
@private
    IBOutlet NSTabView *tabView_;
    
    // Read-only fields on Main tab
    IBOutlet NSTextField *gameDescriptionField_;
    IBOutlet NSTextField *ipAddressField_;
    IBOutlet NSTextField *playerCountField_;
    IBOutlet NSTextField *uptimeField_;
    IBOutlet NSTableView *variableTable_;
    
    // Controls on Console tab
    IBOutlet NSTextView *consoleView_;
    IBOutlet NSTextField *commandField_;
    IBOutlet NSButton *submitButton_;
    
    // Data source for variable table view
    NSArray *variableTableContents_;
    // Timer for refreshing variables values
    NSTimer *refreshTimer_;

    // Current player count
    NSString *currentPlayerCount_;
    
    // Last uptime value received
    NSInteger uptimeReceived_;
    // Last uptime value displayed
    NSInteger uptimeDisplayed_;
    // Frame time of last uptime value received
    float lastUptimeReceiveTime_;
    // Determines if the uptime field should be updated
    BOOL canUpdateUptime_;
    
    // Game API proxy object
    SDGameAPIProxy *gameAPI_;
    // Attributes for console text (font name, size, etc.)
    NSDictionary *consoleTextAttr_;
    // Determines if the app should exit yet
    BOOL shouldQuitApp_;
}

// Selects first tab view
- (IBAction)selectMainTab:(id)sender;

// Selects second tab view
- (IBAction)selectConsoleTab:(id)sender;

// Sends command to server
- (IBAction)submitCommand:(id)sender;

// Begins server shutdown process. Returns whether or not the application should quit.
- (BOOL)startServerShutdown;

// Server warning and error notifications
- (void)gameServerOutputWarning:(NSNotification *)notification;
- (void)gameServerOutputError:(NSNotification *)notification;

@end
