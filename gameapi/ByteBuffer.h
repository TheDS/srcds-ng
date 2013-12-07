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

#ifndef _INCLUDE_SRCDS_BYTEBUFFER_H_
#define _INCLUDE_SRCDS_BYTEBUFFER_H_

#include <stdlib.h>

// Reads bytes of various sizes from a buffer
class ByteBufferReader
{
public:
    ByteBufferReader(const char *buffer, size_t bytes);
    ~ByteBufferReader();
    
    void Reset() const;
    bool Seek(size_t pos) const;
    bool CanRead(size_t bytes) const;
    
    const char *GetBase() const;
    const char *GetCurrent() const;
    
    char ReadByte() const;
    int ReadInt() const;
    const char *ReadString() const;
private:
    const char *base_;
    mutable const char *current_;
    size_t size_;
};

// Writes bytes of various sizes to a buffer
class ByteBufferWriter
{
public:
    ByteBufferWriter();
    explicit ByteBufferWriter(size_t capacity);
    ~ByteBufferWriter();
    
    void Reset();
    bool Seek(size_t pos);
    
    const char *GetBase() const;
    size_t GetBytesWritten() const;
    
    void WriteByte(char value);
    void WriteInt(int value);
    void WriteString(const char *value);
    void Write(const void *memory, size_t size);
private:
    void Grow(size_t needed);
private:
    char *base_;
    char *current_;
    size_t size_;
    size_t capacity_;
};

#endif // _INCLUDE_SRCDS_BYTEBUFFER_H_
