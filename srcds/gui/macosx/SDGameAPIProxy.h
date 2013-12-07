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

#import <Foundation/Foundation.h>
#import "SDGameDelegate.h"

//
// Objective-C singleton wrapper for IGameAPI C++ interface
//

// Forward declarations
struct IGameAPI;
struct CocoaListener;

@interface SDGameAPIProxy : NSObject
{
@private
    id<SDGameDelegate> _delegate;

    // C++ objects
    struct IGameAPI *_api;
    struct CocoaListener *_listener;
}

@property (assign, nonatomic) id<SDGameDelegate> delegate;

// Returns the shared Game API proxy object
+ (instancetype)sharedProxy;

// Set the game directory name for the server
- (void)setGame:(NSString *)name;

// Adds a command line parameter
- (void)addParam:(NSString *)param withValue:(NSString *)value;

// Returns an array of SDGame objects
- (NSArray *)buildGameList;

// Returns an array of maps for the specified game directory
- (NSArray *)buildMapListForGame:(NSString *)gameDir;

// Starts up the game server
- (void)runServerWithDelegate:(id<SDGameDelegate>)delegate;

// Shuts down the game server
- (void)stopServer;

// Executes a command on the game server
- (void)execCommand:(NSString *)cmd;

// Sets the value of a console variable
- (void)setVariable:(NSString *)variable withValue:(NSString *)value;

// Requests the value of a console variable.
// The value will be returned through onValueReceived:forVariable: from SDGameDelegate.
- (void)requestValueForVariable:(NSString *)variable;

// Returns the current server frame time
- (float)getFrameTime;

@end

// Notifications posted for server warnings and errors
extern NSString * const SDGameServerWarningNotification;
extern NSString * const SDGameServerErrorNotification;
