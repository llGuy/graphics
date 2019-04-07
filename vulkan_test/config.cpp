#include "core.hpp"

// parser has 2 sections : the static section, and dynamic
// static : all things that are built-in to the language
// dynamic : the procedure each attribute trigures is up to the user

enum class Token_Type { KEYWORD
			, CONSTANT
			, ENTRY_ID
			, OPERATOR
			, NOT_DETERMINED_YET};

enum class Constant_Type { NAME 
			   , NUMBER /* start with number */ };

enum Operator_Type :s8 { COLON=':'
			 , EQUAL='='
			 , COMMA=','
			 , OPEN_SQUARE='['
			 , CLOSE_SQUARE=']'
			 , DOLLAR='$'
			 , OPEN_BRACK='('
			 , CLOSE_BRACK=')' };

struct Token
{
    const char *tk_bgn; // beginning
    s32 tk_len;
    Token_Type type = Token_Type::NOT_DETERMINED_YET;
};

struct Attribute
{
    Token *id;
    Memory_Buffer_View<Token *> data;
};

struct Entry
{
    Token *id;
    Memory_Buffer_View<Attribute> atrbs;
};

struct Config_Attribute_Proc
{
    void *proc;
    const char *str;
    s32 str_len;

    void *usr_data;
};

struct MAC_Definition
{
    Token *MAC_id;
    Memory_Buffer_View<Token *> params;
};

struct Config_Parser
{
    const char *cfgs;

    Hash_Table_Inline<MAC_Definition, 7, 3, 3> *mac_lkup_tbl;
    
    Memory_Buffer_View<Token> tks;
};

void
begin_proc_init(void)
{
    
}

void
push_proc(void)
{
    
}

void
end_proc_init(void)
{
    
}

void
open_config_file(const char *filename)
{
    FILE *fp = fopen("r", filename);
}
