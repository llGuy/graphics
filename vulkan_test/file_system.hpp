#pragma once

#include "core.hpp"

// create a file handling system : for example track certain files to see if they change and what to do if that is the case

enum File_Format
{
    BINARY
    , PNG
    , JPG
    , WAV
};

struct File
{
    const char *file_path;
    File_Format format;
};

struct File_Data
{
    void *data;
    u32 size;

    // all types of extra data
    enum Extra_Data { WIDTH, HEIGHT, CHANNELS, INVALID };
    u32 extras[Extra_Data::INVALID];
};

void
set_virtual_file_path(const char *path);

enum Read_Flags {RECORD_BIT = 1 << 0};

File_Data
read_from_file(const File &file, Read_Flags flags);

template <typename T> struct File_Proc_Wrapper
{
    T proc;

    FORCEINLINE void
    execute(void)
    {
	proc();
    }
};

template <typename T> void
attach_update_proc(T &&proc, const File &file)
{
    
}
