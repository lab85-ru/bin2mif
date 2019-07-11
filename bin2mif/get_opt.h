#ifndef GET_OPT_H_
#define GET_OPT_H_

#include "stdint.h"

#define LEN_STRING_DEC_MAX   (10)
#define LEN_STRING_HEX_MAX   (10)


typedef enum {GTS_ERROR, GTS_DEC, GTS_HEX} type_string_e;
typedef enum { OP_NONE = 0, OP_RD, OP_WR, OP_CMP, OP_ER } operation_e;

typedef struct
{
    int hversion;         // номер версии Звголовка
    int v1;               // номер версии софта v1
    int v2;               // номер версии софта v2
    int v3;               // номер версии софта v3
	char * i_file_name;   // file name for I
	char * o_file_name;   // file name for O
}param_opt_st;

int get_opt(const int argc, char** argv, param_opt_st *param_opt);
type_string_e get_type_string(const char * st);
int conv_to_uint32(const char * st, uint32_t *ui32);


#endif