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

#import "StartWindowController.h"
#import "SDGameAPIProxy.h"
#import "SDGame.h"

//
// Private property and method declarations
//
@interface StartWindowController ()

@property (retain, nonatomic) NSArray *gameList;
@property (retain, nonatomic) NSArray *mapList;
@property (retain, nonatomic) NSArray *maxPlayersList;
@property (copy, nonatomic) NSString *lastGameDesc;

// Load games into game drop-down list
- (void)loadGameList;

// Selects the first item from the game drop-down list if necessary
- (void)selectFirstGame;

// Selects the first item from the map drop-down list if necessary
- (void)selectFirstMap;

@end


@implementation StartWindowController

@synthesize gameList = gameList_;
@synthesize mapList = mapList_;
@synthesize maxPlayersList = maxPlayersList_;
@synthesize lastGameDesc = lastGameDesc_;

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self)
    {
        NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
        
        // Register for warning notification
        [defaultCenter addObserver:self
                          selector:@selector(gameServerOutputWarningOrError:)
                              name:SDGameServerWarningNotification
                            object:nil];
        
        // Register for error notification
        [defaultCenter addObserver:self
                          selector:@selector(gameServerOutputWarningOrError:)
                              name:SDGameServerErrorNotification
                            object:nil];

        // Register default values for some of the window's fields
        [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"sv_lan": @0,
                                                                  @"maxplayers": @7,
                                                                  @"port": @27015,
                                                                  @"rcon_password": @"",
                                                                  @"secure": @YES}];
    }
    return self;
}

- (void)dealloc
{
    [mapList_ release];
    [lastGameDesc_ release];
    [gameList_ release];
    [maxPlayersList_ release];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}


- (void)awakeFromNib
{
    NSMutableArray *maxPlayers = [[[NSMutableArray alloc] init] autorelease];
    
    // Initialize max players list with the numbers 1 through 32
    for (int i = 1; i <= 32; i++)
        [maxPlayers addObject:[NSString stringWithFormat:@"%d", i]];
    
    // Assign data source for max players field
    [self setMaxPlayersList:maxPlayers];
    
    // Load game field with game list
    [self loadGameList];
}

- (void)controlTextDidChange:(NSNotification *)notification
{
    // Start button is only enabled if all text fields are filled in
    BOOL startEnabled = [[serverNameField_ stringValue] length] > 0 &&
                                  [mapListField_ numberOfItems] > 0 &&
                              [[portField_ stringValue] length] > 0 &&
                      [[rconPasswordField_ stringValue] length] > 0;
    
    [startButton_ setEnabled:startEnabled];
}

- (void)selectFirstMap
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *map = [defaults stringForKey:@"map"];
    
    // Select the first map if one hasn't been saved or doesn't exist for the current game
    if (map == nil || [[self mapList] containsObject:map] == NO)
    {
        map = [[self mapList] objectAtIndex:0];
        [defaults setObject:map forKey:@"map"];
    }
}

- (void)selectFirstGame
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *gameDir = [defaults stringForKey:@"game"];
    BOOL foundSavedGame = NO;
    
    // Determine if the saved game name actually exists
    if (gameDir != nil)
    {
        for (SDGame *game in [self gameList])
        {
            if ([[game gameDirectory] isEqualToString:gameDir])
            {
                foundSavedGame = YES;
                break;
            }
        }
    }
    
    // Select the first game if one hasn't been previously set or doesn't exist
    if (foundSavedGame == NO)
    {
        gameDir = [[[self gameList] objectAtIndex:0] gameDirectory];
        [defaults setObject:gameDir forKey:@"game"];
    }
}

- (void)loadGameList
{
    // Assign data source for game list field
    [self setGameList:[[SDGameAPIProxy sharedProxy] buildGameList]];
    
    if ([[self gameList] count] > 0)
    {
        // Select the first game if necessary
        [self selectFirstGame];
        
        // Fire game selection change to load map list
        [self gameSelectionChanged:self];
    }
    else
    {
        // Show error message because no games have been detected
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:@"No games have been detected."];
        [alert setInformativeText:@"Please make sure that the app is in the main folder of a " \
                                  @"Source engine based game."];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert runModal];
        
        // Terminate app since there are no detected games
        [NSApp terminate:nil];
    }
}

- (IBAction)gameSelectionChanged:(id)sender
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    NSString *gameDir = [defaults stringForKey:@"game"];
    NSString *savedServerName = [defaults stringForKey:@"hostname"];
    NSString *lastServerName = [NSString stringWithFormat:@"%@ Server", [self lastGameDesc]];
    NSString *currentGame = [[gameListField_ selectedItem] title];
    
    // Update default host name if it hasn't been altered by the user
    if ([savedServerName length] == 0 ||
        [[serverNameField_ stringValue] isEqualToString:lastServerName])
    {
        NSString *newHostname = [NSString stringWithFormat:@"%@ Server", currentGame];
        
        [defaults setObject:newHostname forKey:@"hostname"];
    }
    
    // Set last selected game
    [self setLastGameDesc:currentGame];
    
    // Assign data source for map list field
    [self setMapList:[[SDGameAPIProxy sharedProxy] buildMapListForGame:gameDir]];
    
    if ([[self mapList] count] > 0)
    {
        // Select the first map if necessary
        [self selectFirstMap];
        
        // Fire control text change to possibly enable Start Server button
        [self controlTextDidChange:nil];
    }
    else
    {
        // Remove any items in the map list and disable it
        [mapListField_ removeAllItems];
        [mapListField_ setEnabled:NO];

        // Show error message because no maps have been detected
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:@"The selected game does not have any maps."];
        [alert setInformativeText:@"Please select a different game."];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert performSelector:@selector(runModal) withObject:nil afterDelay:0.1];
    }
}

- (void)gameServerOutputWarningOrError:(NSNotification *)notification
{
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    NSString *msg = [[notification userInfo] objectForKey:@"message"];

    // Show warning and error messages using alert window
    [alert setMessageText:msg];
    [alert runModal];
}
@end
