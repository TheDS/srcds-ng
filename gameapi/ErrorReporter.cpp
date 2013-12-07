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

#include <stdio.h>
#include <stdlib.h>
#include "stringutil.h"
#include "ErrorReporter.h"

class DefaultListener : public IGameListener
{
    void OnWarning(const char *msg)
    {
        printf("[WARNING] %s", msg);
    }
    
    void OnError(const char *msg)
    {
        printf("[ERROR] %s", msg);
    }
} s_DefaultListener;

ErrorReporter::ErrorReporter() : listener_(&s_DefaultListener)
{
    
}

void ErrorReporter::SetListener(IGameListener *listener)
{
    listener_ = listener;
}

void ErrorReporter::Warning(const char *format, ...)
{
    va_list ap;
    
    va_start(ap, format);
    strvformat(message_, sizeof(message_), format, ap);
    va_end(ap);

    listener_->OnWarning(message_);
}

void ErrorReporter::Error(const char *format, ...)
{
    va_list ap;
    
    va_start(ap, format);
    strvformat(message_, sizeof(message_), format, ap);
    va_end(ap);
    
    listener_->OnError(message_);
    exit(1);
}
