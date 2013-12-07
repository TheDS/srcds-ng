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

//
// Objective-C adapatation of C++ IGameListener from IGameAPI.h
//

@protocol SDGameDelegate <NSObject>

@required

// Called after the dedicated server library has been loaded.
// This is where command line parameters should be added.
- (void)onServerLoaded;

// Called when the server has started it's first frame
- (void)onServerStarted;

// Called when the server has been stopped
- (void)onServerStopped;

// Called every server frame
- (void)onGameFrame;

// Called when a requested variable's value has been received
- (void)onValueReceived:(NSString *)value forVariable:(NSString *)variable;

// Called when some piece of server data has been updated - usually the map or player count
- (void)onDataUpdated:(NSString *)data;

// Called when server has sent a message to the console
- (void)onConsoleOutput:(NSString *)msg;

@end
