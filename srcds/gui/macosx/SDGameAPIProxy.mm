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

#import "SDGameAPIProxy.h"
#import "SDGame.h"

#include <crt_externs.h>

#include "IGameAPI.h"
#include "am-linkedlist.h"
#include "am-string.h"

NSString * const SDGameServerWarningNotification = @"GameServerWarningNotification";
NSString * const SDGameServerErrorNotification = @"GameServerErrorNotification";

class CocoaListener : public IGameListener
{
private:
    SDGameAPIProxy *owner_;

public:
    CocoaListener(SDGameAPIProxy *owner) : owner_(owner)
    {
    }
    
    void OnServerLoaded()
    {
        [[owner_ delegate] onServerLoaded];
    }
    
    void OnServerStarted()
    {
        [[owner_ delegate] onServerStarted];
    }
    
    void OnServerStopped()
    {
        [[owner_ delegate] onServerStopped];
    }
    
    void OnGameFrame()
    {
        [[owner_ delegate] onGameFrame];
    }
    
    void OnValueReceived(const char *variable, const char *value)
    {
        [[owner_ delegate] onValueReceived:@(value) forVariable:@(variable)];
    }
    
    void OnDataUpdated(const char *data)
    {
        [[owner_ delegate] onDataUpdated:@(data)];
    }
    
    void OnConsoleOutput(const char *msg)
    {
        [[owner_ delegate] onConsoleOutput:@(msg)];
    }
    
    void OnWarning(const char *msg)
    {
        NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
        NSDictionary *info = [NSDictionary dictionaryWithObject:@(msg) forKey:@"message"];
        
        [defaultCenter postNotificationName:SDGameServerErrorNotification
                                     object:owner_
                                   userInfo:info];
    }
    
    void OnError(const char *msg)
    {
        NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
        NSDictionary *info = [NSDictionary dictionaryWithObject:@(msg) forKey:@"message"];
        
        [defaultCenter postNotificationName:SDGameServerErrorNotification
                                     object:owner_
                                   userInfo:info];
    }
};

@implementation SDGameAPIProxy

@synthesize delegate = _delegate;

+ (instancetype)sharedProxy
{
    static SDGameAPIProxy *proxy = nil;
    
    @synchronized(self)
    {
        if (proxy == nil)
            proxy = [[super allocWithZone:NULL] init];
    }

    return proxy;
}

+ (id)allocWithZone:(struct _NSZone *)zone
{
    return [[self sharedProxy] retain];
}

+ (id)copyWithZone:(struct _NSZone *)zone
{
    return self;
}

- (id)retain
{
    return self;
}

- (unsigned)retainCount
{
    // Denotes that the object cannot be released
    return NSUIntegerMax;
}

- (oneway void)release
{
    // Since this object is shared, never release
}

- (id)autorelease
{
    return self;
}

- (id)init
{
    self = [super init];
    if (self)
    {
        _api = GetGameAPI();
        _listener = new CocoaListener(self);

        _api->SetListener(_listener);
    }
    return self;
}

- (void)dealloc
{
    delete _listener;
    
    _delegate = nil;
    
    [super dealloc];
}

- (void)setGame:(NSString *)name
{
    _api->SetGame([name UTF8String]);
}

- (void)addParam:(NSString *)param withValue:(NSString *)value
{
    _api->AddParam([param UTF8String], [value UTF8String]);
}

- (NSArray *)buildGameList
{
    NSMutableArray *games;
    LinkedList<game_t> gameList;
    
    _api->BuildGameList(gameList);
    
    games = [[NSMutableArray alloc] init];
    
    for (game_t i : gameList)
    {
        SDGame *game = [[[SDGame alloc] init] autorelease];
        [game setGameDescription:@(i.gameDescription.chars())];
        [game setGameDirectory:@(i.gameDirectory.chars())];

        [games addObject:game];
    }
    
    return [games autorelease];
}

- (NSArray *)buildMapListForGame:(NSString *)gameDir
{
    NSMutableArray *maps;
    LinkedList<AString> mapList;
    
    _api->BuildMapListForGame([gameDir UTF8String], mapList);
    
    maps = [[NSMutableArray alloc] init];
    
    for (AString i : mapList)
        [maps addObject:@(i.chars())];
         
    return [maps autorelease];
}

- (void)runServerWithDelegate:(id<SDGameDelegate>)delegate
{
    [self setDelegate:delegate];

    _api->RunServer(UIMode_GUI, *_NSGetArgc(), *_NSGetArgv());
}

- (void)stopServer
{
    _api->StopServer();
}

- (void)execCommand:(NSString *)cmd;
{
    _api->ExecCommand([cmd UTF8String]);
}

- (void)setVariable:(NSString *)variable withValue:(NSString *)value;
{
    _api->SetValue([variable UTF8String], [value UTF8String]);
}

- (void)requestValueForVariable:(NSString *)variable;
{
    _api->RequestValue([variable UTF8String]);
}

- (float)getFrameTime
{
    return _api->GetFrameTime();
}

@end
