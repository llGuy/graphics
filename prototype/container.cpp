#include "container.h"

constexpr uint32
CompileHash(const char *String
	    , uint32 Size)
{
    return ((Size ? CompileHash(String, Size - 1) : 2166136261u) ^ String[Size]) * 16777619u;
};

constexpr string_view
operator""_hash(const char *String
		, uint32 Count)
{
    return string_view{ String, Count
	    , ((Count ? CompileHash(String, Count - 1) : 2166136261u) ^ String[Count]) * 16777619u };
};
