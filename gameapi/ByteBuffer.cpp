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

#include "ByteBuffer.h"
#include <string.h>

//
// ByteBufferReader
//

ByteBufferReader::ByteBufferReader(const char *buffer, size_t bytes)
    : base_(buffer), current_(buffer), size_(bytes)
{
    
}

ByteBufferReader::~ByteBufferReader()
{

}

void ByteBufferReader::Reset() const
{
    current_ = base_;
}

bool ByteBufferReader::Seek(size_t pos) const
{
    if (pos > size_ - 1)
        return false;
    
    current_ = base_ + pos;

    return true;
}

bool ByteBufferReader::CanRead(size_t bytes) const
{
    return ((current_ - base_) + bytes) <= size_;
}

const char *ByteBufferReader::GetBase() const
{
    return base_;
}

const char *ByteBufferReader::GetCurrent() const
{
    return current_;
}

char ByteBufferReader::ReadByte() const
{
    if (!CanRead( sizeof(char) ))
        return 0;
    
    char value = *(char *)current_;

    current_ += sizeof(char);

    return value;
}

int ByteBufferReader::ReadInt() const
{
    if (!CanRead( sizeof(int) ))
        return 0;
    
    int value = *(int *)current_;
    
    current_ += sizeof(int);
    
    return value;
}

const char *ByteBufferReader::ReadString() const
{
    const char *value = current_;
    size_t bytesLeft = size_ - (current_ - base_);
    size_t len = strnlen(current_, bytesLeft);
    
    if (len >= bytesLeft)
        return nullptr;
    
    current_ += len + 1;
    
    return value;
}

//
// ByteBufferWriter
//

#define BUFFER_INITIAL_SIZE 512

ByteBufferWriter::ByteBufferWriter() : size_(0)
{
    base_ = (char *)malloc(BUFFER_INITIAL_SIZE);
    capacity_ = BUFFER_INITIAL_SIZE;
    current_ = base_;
}

ByteBufferWriter::ByteBufferWriter(size_t capacity) : size_(0), capacity_(capacity)
{
    base_ = (char *)malloc(capacity_);
    current_ = base_;
}

ByteBufferWriter::~ByteBufferWriter()
{
    free(base_);
}

void ByteBufferWriter::Reset()
{
    current_ = base_;
    size_ = 0;
}

bool ByteBufferWriter::Seek(size_t pos)
{
    if (pos > size_ - 1)
        return false;
    
    current_ = base_ + pos;
    
    return true;
}

const char *ByteBufferWriter::GetBase() const
{
    return base_;
}

size_t ByteBufferWriter::GetBytesWritten() const
{
    return size_;
}

void ByteBufferWriter::WriteByte(char value)
{
    Grow(sizeof(char));
    
    *(char *)current_ = value;
    current_ += sizeof(char);

    size_ += sizeof(char);
}

void ByteBufferWriter::WriteInt(int value)
{
    Grow(sizeof(int));
    
    *(int *)current_ = value;
    current_ += sizeof(int);
    
    size_ += sizeof(int);
}

void ByteBufferWriter::WriteString(const char *value)
{
    size_t len = strlen(value);
    size_t strSize = len + 1;
    
    Grow(strSize);
    
    memcpy(current_, value, len);
    current_[len] = '\0';
    current_ += strSize;
    
    size_ += strSize;
}

void ByteBufferWriter::Write(const void *memory, size_t size)
{
    Grow(size);

    memcpy(current_, memory, size);
    current_ += size;
    
    size_ += size;
}

void ByteBufferWriter::Grow(size_t needed)
{
    if (size_ + needed <= capacity_)
        return;
    
    size_t pos = current_ - base_;
    
    do {
        capacity_ *= 2;
        base_ = (char *)realloc(base_, capacity_);
        current_ = base_ + pos;
    } while (size_ + needed > capacity_);
}
