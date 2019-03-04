#include "file_system.hpp"
#include <stb_image.h>

// by default is emptyr
global_var const char *virtual_file_path = "";
global_var Free_List_Allocator file_proc_allocator;

void
set_virtual_file_path(const char *path)
{
    virtual_file_path = path;
}

internal char *
make_file_path(const char *file_path)
{
    u32 virtual_path_size = strlen(virtual_file_path);
    u32 file_path_size = strlen(file_path);
    u32 total_path_size = virtual_path_size + file_path_size;
    char *path_buffer = (char *)allocate_stack(total_path_size * sizeof(char));
    memcpy(path_buffer, virtual_file_path, virtual_path_size);
    memcpy(path_buffer + virtual_path_size, file_path, file_path_size);
    return(path_buffer);
}

internal File_Data
read_binary(const File &file_description)
{
    auto path = make_file_path(file_description.file_path);
    
    FILE *file = fopen(path, "rb");
    if (file == nullptr)
    {
	OUTPUT_DEBUG_LOG("error - couldnt load file \"%s\"\n", path);
	assert(false);
    }
    fseek(file, 0, SEEK_END);
    u32 size = ftell(file);
    rewind(file);

    void *buffer = allocate_stack(size
				  , 1
				  , file_description.file_path);
    fread(buffer, 1, size, file);
    
    fclose(file);

    return(File_Data{buffer, size});
}    

internal File_Data
read_image(const File &file_description)
{
    s32 w, h, channels;
    stbi_uc *pixels = stbi_load(file_description.file_path, &w, &h, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
	OUTPUT_DEBUG_LOG("error - couldnt load image \"%s\"\n", file_description.file_path);
	assert(false);
    }
    
    File_Data data;
    data.data = pixels;
    data.size = w * h * channels;
    data.extras[File_Data::Extra_Data::WIDTH] = w;
    data.extras[File_Data::Extra_Data::HEIGHT] = h;
    data.extras[File_Data::Extra_Data::CHANNELS] = channels;

    return(data);
}

File_Data
read_file_data(const File &file, Read_Flags flags)
{
    switch(file.format)
    {
    case File_Format::BINARY: return(read_binary(file));
    case File_Format::PNG: case File_Format::JPG: return(read_image(file));
    };
}
