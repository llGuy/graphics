#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#include "int.h"

struct string_view
{
    const char *Data;
    uint32 Size;
    uint32 Hash;

    bool
    operator==(const string_view &Other) const
    {
	return Other.Hash == this->Hash;
    }
};

extern constexpr uint32
CompileHash(const char *String
	    , uint32 Size);

extern constexpr string_view
operator""_hash(const char *String
		, uint32 Count);

template<typename T> class string_hash;

template<> class string_hash <string_view>
{
public:
    uint32
    operator()(const string_view &String) const
    {
	return String.Hash;
    };
};

#endif
