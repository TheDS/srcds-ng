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

#import "ServerWindowController.h"

//
// Private property and method declarations
//
@interface ServerWindowController ()

@property (retain, nonatomic) NSString *currentPlayerCount;

// Starts the server
- (void)runServer;

// Request variable values from the server
- (void)refreshVariables:(NSTimer *)timer;

// Updates variable field with the specfied value
- (void)updateVariable:(NSString *)variable withValue:(NSString *)value;

@end


@implementation ServerWindowController

@synthesize currentPlayerCount = currentPlayerCount_;

- (id)init
{
    self = [super initWithWindowNibName:@"ServerWindow"];

    if (self)
    {
        NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
        
        // Register for warning notification
        [defaultCenter addObserver:self
                          selector:@selector(gameServerOutputWarning:)
                              name:SDGameServerWarningNotification
                            object:nil];
        
        // Register for error notification
        [defaultCenter addObserver:self
                          selector:@selector(gameServerOutputError:)
                              name:SDGameServerErrorNotification
                            object:nil];
        
        uptimeReceived_ = 0;
        uptimeDisplayed_ = 0;
        lastUptimeReceiveTime_ = 0.0f;
        canUpdateUptime_ = NO;
        
        gameAPI_ = [SDGameAPIProxy sharedProxy];
        shouldQuitApp_ = NO;
        
        // Set the console font to monospace Menlo at size 11
        NSFont *consoleFont = [NSFont fontWithName:@"Menlo Regular" size:11.0];
        consoleTextAttr_ = [NSDictionary dictionaryWithObject:consoleFont
                                                      forKey:NSFontAttributeName];
        [consoleTextAttr_ retain];
    }

    return self;
}

- (void)dealloc
{
    [currentPlayerCount_ release];
    [variableTableContents_ release];
    [consoleTextAttr_ release];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Initialize dictionaries for the variable table view
    NSMutableDictionary *hostDict = [[NSMutableDictionary alloc]
                                     initWithObjectsAndKeys:@"Server Name", @"Name",
                                                            @"hostname", @"Variable",
                                                            @"", @"Value", nil];
    NSMutableDictionary *mapDict = [[NSMutableDictionary alloc]
                                     initWithObjectsAndKeys:@"Map", @"Name",
                                                            @"map", @"Variable",
                                                            @"", @"Value", nil];
    NSMutableDictionary *netDict = [[NSMutableDictionary alloc]
                                    initWithObjectsAndKeys:@"Network", @"Name",
                                                           @"sv_lan", @"Variable",
                                                           @"", @"Value", nil];
    
    // Autorelease the dictionaries. The variableTableContents_ array will hold on to them.
    [hostDict autorelease];
    [mapDict autorelease];
    [netDict autorelease];
    
    // Add dictionaries to the array holding variable table contents
    variableTableContents_ = [[NSArray alloc] initWithObjects:hostDict, mapDict, netDict, nil];
    
    // Update table with newly added contents
    [variableTable_ reloadData];
    
    // Run server a bit later so the window actually receives focus first
    [self performSelector:@selector(runServer) withObject:nil afterDelay:0.1];
    
    // Refresh variable data every minute
    refreshTimer_ = [NSTimer scheduledTimerWithTimeInterval:60.0
                                                     target:self
                                                   selector:@selector(refreshVariables:)
                                                   userInfo:nil
                                                    repeats:YES];
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Set game field to indicate that the server is loading
    [self updateVariable:@"gamedescription" withValue:@"Loading..."];
    
    // Set player count field with initial value
    NSInteger maxPlayers = [defaults integerForKey:@"maxplayers"] + 1;
    [self updateVariable:@"playercount" withValue:@"0"];
    [self updateVariable:@"maxplayers" withValue:[NSString stringWithFormat:@"%d", maxPlayers]];
    
    // Set other fields with initial values
    [self updateVariable:@"ipaddress" withValue:@"0.0.0.0:0"];
    [self updateVariable:@"uptime" withValue:@"0:00:00:00"];
    [self updateVariable:@"hostname" withValue: [defaults stringForKey:@"hostname"]];
    [self updateVariable:@"map" withValue: [defaults stringForKey:@"map"]];
    [self updateVariable:@"sv_lan" withValue:[defaults stringForKey:@"sv_lan"]];
}

// NSWindowDelegate implementation called when window is about to close
- (BOOL)windowShouldClose:(id)sender
{
    // Begin stopping the server
    [self startServerShutdown];
    
    // Window will close when the server has actually stopped
    return NO;
}

- (IBAction)selectMainTab:(id)sender
{
    [tabView_ selectTabViewItemAtIndex:0];
}

- (IBAction)selectConsoleTab:(id)sender
{
    [tabView_ selectTabViewItemAtIndex:1];
}

- (IBAction)submitCommand:(id)sender
{
    NSString *command = [commandField_ stringValue];
    
    // Trim whitespace from command text
    command = [command stringByTrimmingCharactersInSet:
               [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    
    if ([command length] > 0)
    {
        // Echo command text
        NSString *commandEcho = [NSString stringWithFormat:@"] %@\n", command];
        [self onConsoleOutput:commandEcho];
        
        // Execute command
        [gameAPI_ execCommand:command];
    }
    
    // Clear command field
    [commandField_ setStringValue:@""];
}

- (BOOL)startServerShutdown
{
    if (shouldQuitApp_ == NO)
        [gameAPI_ stopServer];
    
    return shouldQuitApp_;
}

- (void)runServer
{
    id defaults = [[NSUserDefaultsController sharedUserDefaultsController] values];
    
    // Set game directory for the server
    [gameAPI_ setGame:[defaults valueForKey:@"game"]];
    
    // Start the server
    [gameAPI_ runServerWithDelegate:self];
    
    // The app should be able to quit since the server should have stopped after the above call
    shouldQuitApp_ = YES;
    [self close];
}

- (void)refreshVariables:(NSTimer *)timer
{
    // Request values from the server for each of the window's fields
    [gameAPI_ requestValueForVariable:@"playercount"];
    [gameAPI_ requestValueForVariable:@"maxplayers"];
    [gameAPI_ requestValueForVariable:@"gamedescription"];
    [gameAPI_ requestValueForVariable:@"uptime"];
    [gameAPI_ requestValueForVariable:@"ipaddress"];
    [gameAPI_ requestValueForVariable:@"hostname"];
    [gameAPI_ requestValueForVariable:@"map"];
    [gameAPI_ requestValueForVariable:@"sv_lan"];
}

- (void)updateVariable:(NSString *)variable withValue:(NSString *)value
{
    if ([variable isEqualToString:@"playercount"])
    {
        // Store player count. The field will be updated when the maxplayers value is received.
        [self setCurrentPlayerCount:value];
    }
    else if ([variable isEqualToString:@"maxplayers"])
    {
        // Update player count field with current players and max players
        NSString *pc = [NSString stringWithFormat:@"%@ / %@", currentPlayerCount_, value];
        [playerCountField_ setStringValue:pc];
    }
    else if ([variable isEqualToString:@"gamedescription"])
    {
        // Update game field
        [gameDescriptionField_ setStringValue:value];
    }
    else if ([variable isEqualToString:@"uptime"])
    {
        // Update uptime field
        [uptimeField_ setStringValue:value];
    }
    else if ([variable isEqualToString:@"ipaddress"])
    {
        // Update IP address field
        [ipAddressField_ setStringValue:value];
    }
    else if ([variable isEqualToString:@"hostname"])
    {
        NSMutableDictionary *dict = [variableTableContents_ objectAtIndex:0];
        
        // Update server name field in the variable table
        [dict setObject:value forKey:@"Value"];
        [variableTable_ reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:0]
                                  columnIndexes:[NSIndexSet indexSetWithIndex:1]];
        
        // Update server name in the window's title bar
        [[self window] setTitle:value];
    }
    else if ([variable isEqualToString:@"map"])
    {
        NSMutableDictionary *dict = [variableTableContents_ objectAtIndex:1];
        
        // Update map field in the variable table
        [dict setObject:value forKey:@"Value"];
        [variableTable_ reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:1]
                                  columnIndexes:[NSIndexSet indexSetWithIndex:1]];
    }
    else if ([variable isEqualToString:@"sv_lan"])
    {
        NSMutableDictionary *dict = [variableTableContents_ objectAtIndex:2];
        
        // Update network field in the variable table
        if ([value isEqualToString:@"0"])
            [dict setObject:@"Internet" forKey:@"Value"];
        else
            [dict setObject:@"LAN" forKey:@"Value"];
        
        [variableTable_ reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:2]
                                  columnIndexes:[NSIndexSet indexSetWithIndex:1]];
    }
}

// NSTableViewDelegate implementation for counting rows in variable table
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return [variableTableContents_ count];
}

// NSTableViewDelegate implementation for retrieving object for row and column of variable table
- (id)tableView:(NSTableView *)tableView
      objectValueForTableColumn:(NSTableColumn *)tableColumn
            row:(NSInteger)row
{
    NSDictionary *dictionary = [variableTableContents_ objectAtIndex:row];
    id item = [dictionary objectForKey:[tableColumn identifier]];

    return item;
}

// SDGameDelegate implementation
- (void)onServerLoaded
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Set game name
    [gameAPI_ addParam:@"-game" withValue:[defaults stringForKey:@"game"]];
    
    // Set initial map
    [gameAPI_ addParam:@"+map" withValue:[defaults stringForKey:@"map"]];
    
    // Set maxplayers + 1 since the stored value is an index that starts from 0
    NSInteger maxPlayers = [defaults integerForKey:@"maxplayers"] + 1;
    [gameAPI_ addParam:@"-maxplayers" withValue:[NSString stringWithFormat:@"%d", maxPlayers]];
    
    // Set server port number
    [gameAPI_ addParam:@"-port" withValue:[defaults stringForKey:@"port"]];
    
    // Set sv_lan variable (LAN or Internet)
    [gameAPI_ addParam:@"+sv_lan" withValue:[defaults stringForKey:@"sv_lan"]];
    
    // Set hostname with quotes around it since it could contain spaces
    NSString *hostname = [NSString stringWithFormat:@"\"%@\"",
                                                    [defaults stringForKey:@"hostname"]];
    [gameAPI_ addParam:@"+hostname" withValue:hostname];
    
    // Set rcon password with quotes around it since it could contain spaces
    NSString *rconPassword = [NSString stringWithFormat:@"\"%@\"",
                                                        [defaults stringForKey:@"rcon_password"]];
    [gameAPI_ addParam:@"+rcon_password" withValue:rconPassword];
    
    // Add -insecure if secure field was unchecked
    if ([defaults boolForKey:@"secure"] == NO)
        [gameAPI_ addParam:@"-insecure" withValue:@""];
}

// SDGameDelegate implementation
- (void)onServerStarted
{
    canUpdateUptime_ = YES;
    
    // Force variable refresh at server start
    [self refreshVariables:nil];
}

// SDGameDelegate implementation
- (void)onServerStopped
{
    // Stop printing to console text view
    consoleView_ = nil;

    // Stop periodic variable refresh
    [refreshTimer_ invalidate];
}

// SDGameDelegate implementation
- (void)onGameFrame
{
    // The uptime value is received from the server and only refreshed every minute. So on every
    // server frame, the real uptime is extrapolated from the last received uptime value plus the
    // amount of frame time that has passed since the last uptime update.
    int currentTime = uptimeReceived_ + (int)([gameAPI_ getFrameTime] - lastUptimeReceiveTime_);
    
    if (canUpdateUptime_ && currentTime != uptimeDisplayed_)
    {
        // Calculate days, hours, minutes, and seconds from current time value
        int days = currentTime / 3600 / 24;
        int hours = currentTime / 3600;
        int minutes = currentTime / 60 % 60;
        int seconds = currentTime % 60;

        // Format time string using calculated values
        NSString *timeString = [NSString stringWithFormat:@"%0.1d:%0.2d:%0.2d:%0.2d",
                                                          days,
                                                          hours,
                                                          minutes,
                                                          seconds];
        
        // Update displayed value
        [self updateVariable:@"uptime" withValue:timeString];
        uptimeDisplayed_ = currentTime;
    }
    
    
    // Dispatch Cocoa events to keep the GUI responsive as the server runs
    @autoreleasepool
    {
        for (;;)
        {
            NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            
            if (event == nil)
                break;
            
            [NSApp sendEvent:event];
        }
    }
}

// SDGameDelegate implementation
- (void)onValueReceived:(NSString *)value forVariable:(NSString *)variable
{
    if ([variable isEqualToString:@"uptime"])
    {
        // Record uptime and frame time for which it was received
        uptimeReceived_ = [value integerValue];
        lastUptimeReceiveTime_ = [gameAPI_ getFrameTime];
        
        // Let the field be updated with the formatted time value on each game frame
        return;
    }
    
    // Update variable field with new value
    [self updateVariable:variable withValue:value];
}

// SDGameDelegate implementation
- (void)onDataUpdated:(NSString *)data
{
    // If one of these has been updated, force a variable refresh
    if ([data isEqualToString:@"UpdatePlayers"] || [data isEqualToString:@"UpdateMap"])
        [self refreshVariables:nil];
}

// SDGameDelegate implementation
- (void)onConsoleOutput:(NSString *)msg
{
    if ([msg length] > 0)
    {
        NSAttributedString* str = [[NSAttributedString alloc] initWithString:msg
                                                                  attributes:consoleTextAttr_];
        NSTextStorage *storage = [consoleView_ textStorage];
        
        // Add message to console text
        [storage beginEditing];
        [storage appendAttributedString:str];
        [storage endEditing];

        [str autorelease];
        
        // FIXME: When there is a lot of text to output, scrolling after each call to
        //        OnConsoleOutput() slows the program down considerably. Scrolling should be
        //        delayed to some later point in time to fix this.
        [consoleView_ scrollRangeToVisible:NSMakeRange([[consoleView_ string] length], 0)];
    }
}

- (void)gameServerOutputWarning:(NSNotification *)notification
{
    NSString *msg = [[notification userInfo] objectForKey:@"message"];

    // Display warning in console
    [self onConsoleOutput:[NSString stringWithFormat:@"[WARNING] %@\n", msg]];
}

- (void)gameServerOutputError:(NSNotification *)notification
{
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    NSString *msg = [[notification userInfo] objectForKey:@"message"];
    
    // Show error message using alert window
    [alert setMessageText:msg];
    [alert runModal];
}

@end
