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

#ifndef _INCLUDE_SRCDS_IGAMESERVERDATA_H_
#define _INCLUDE_SRCDS_IGAMESERVERDATA_H_

//
// The following declarations were discovered through reverse-engineering Source game libraries,
// some of which contained symbols and DWARF debug information.
//

struct netadr_t;

typedef unsigned int ra_listener_id;
const ra_listener_id INVALID_LISTENER_ID = 0xffffffff;

// Interface name to request from engine's factory (CreateInterface)
#define GAMESERVERDATA_INTERFACE_VERSION "GameServerData001"

// Used with WriteDataRequest below
enum ServerDataRequestType_t
{
	SERVERDATA_REQUESTVALUE,
	SERVERDATA_SETVALUE,
	SERVERDATA_EXECCOMMAND,
	SERVERDATA_AUTH,
	SERVERDATA_VPROF,
	SERVERDATA_REMOVE_VPROF,
	SERVERDATA_TAKE_SCREENSHOT,
	SERVERDATA_SEND_CONSOLE_LOG,
};

// Used with ReadDataResponse below
enum ServerDataResponseType_t
{
	SERVERDATA_RESPONSE_VALUE,
	SERVERDATA_UPDATE,
	SERVERDATA_AUTH_RESPONSE,
	SERVERDATA_VPROF_DATA,
	SERVERDATA_VPROF_GROUPS,
	SERVERDATA_SCREENSHOT_RESPONSE,
	SERVERDATA_CONSOLE_LOG_RESPONSE,
	SERVERDATA_RESPONSE_STRING,
};

class IBaseInterface
{
public:
    virtual ~IBaseInterface() {}
};

// Abstract class for dealing with a variety of server data
class IGameServerData : public IBaseInterface
{
public:

	virtual void WriteDataRequest(ra_listener_id listener, const void *buffer, int bufferSize) = 0;
	virtual int ReadDataResponse(ra_listener_id listener, void *buffer, int bufferSize) = 0;
	virtual ra_listener_id GetNextListenerID(bool authConnection = true, const netadr_t *adr = nullptr) = 0;
	virtual void RegisterAdminUIID(ra_listener_id listener) = 0;
};

#endif
