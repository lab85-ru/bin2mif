#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "get_opt.h"
#include "conv_str_to_uint32.h"


//------------------------------------------------------------------------------
// Poisk parametra v spiske i zapolnenie parametra
// return = X - kolihestvo naydenih parametrov
// retunr -1 - error
//------------------------------------------------------------------------------
int get_opt(const int argc, char** argv, param_opt_st *param_opt)
{
	int res = 0; //result get_opt
	int cn = 0;
	int i = 0;
	
	//printf("argc=%d\n\r", argc);

	// argc - 1 -> dolhno bit chetno ! t.k. "-param data"
	if (argc == 1) return 0;

	// start from i=1, t.k. =0 sama programma
    i = 1;
    while(i != argc){
		
		//printf("string = %s\n\r", argv[i]);
		//printf("     i = %d\n\r", i);

		if (strcmp("-hv", argv[i]) == 0){
			if (i<argc){
				i++;
				//printf("%s\n\r", argv[i]);
				param_opt->hversion = 0;
				cn = conv_to_uint32((const char*)argv[i], (uint32_t*)&param_opt->hversion);
				//printf("cn = %d\n", cn);
				//if (cn <= 0) return -1;
				i++;
				res++;
			}else return -1;
			continue;
		}
		
		if (strcmp("-v1", argv[i]) == 0){
			if (i<argc){
				i++;
				//printf("%s\n\r", argv[i]);
				param_opt->v1 = 0;
				cn = conv_to_uint32((const char*)argv[i], (uint32_t*)&param_opt->v1);
				//printf("cn = %d\n", cn);
				if (cn <= 0) return -1;
				i++;
				res++;
			}else return -1;
			continue;
		}
		
		if (strcmp("-v2", argv[i]) == 0){
			if (i<argc){
				i++;
				//printf("%s\n\r", argv[i]);
				param_opt->v2 = 0;
				cn = conv_to_uint32((const char*)argv[i], (uint32_t*)&param_opt->v2);
				//printf("cn = %d\n", cn);
				if (cn <= 0) return -1;
				i++;
				res++;
			}else return -1;
			continue;
		}
		
		if (strcmp("-v3", argv[i]) == 0){
			if (i<argc){
				i++;
				//printf("%s\n\r", argv[i]);
				param_opt->v3 = 0;
				cn = conv_to_uint32((const char*)argv[i], (uint32_t*)&param_opt->v3);
				//printf("cn = %d\n", cn);
				if (cn <= 0) return -1;
				i++;
				res++;
			}else return -1;
			continue;
		}
		
		if (strcmp("-i", argv[i]) == 0){
			//printf("Find: --read = ");
			if (i<argc){
				i++;
				//printf("%s\n\r", argv[i]);
                param_opt->i_file_name = argv[i];
				i++;
				res++;
			}else return -1;
			continue;
		}
		
		if (strcmp("-o", argv[i]) == 0){
			//printf("Find: --read = ");
			if (i<argc){
				i++;
				//printf("%s\n\r", argv[i]);
                param_opt->o_file_name = argv[i];
				i++;
				res++;
			}else return -1;
			continue;
		}

		return -1;

	}//for (int i=0; i<argc; i++){

    return res;
}

//=============================================================================
// conver string -> uint32
//
// return:
// -1 error(format string)
// > 0 len in string
//=============================================================================
int conv_to_uint32(const char * st, uint32_t *ui32)
{
	uint32_t ui = 0;

	switch ( get_type_string(st) ){
	case GTS_HEX:{
		return conv_str_to_uint32( (uint8_t*)st, ui32);
	}

	case GTS_DEC:{
        ui = atol(st);
		*ui32 = (uint32_t)ui;
		return strlen(st);
	}

	default:
		return -1;
	}//switch ( get_type_string(st) ){

}


//=============================================================================
// type string DEC or HEX
//
// return:
// -1 error(format string)
// > 0 len in string
//=============================================================================
type_string_e get_type_string(const char * st)
{
	int len = strlen(st);
	int index = 0;
	char c;
	type_string_e tse;

    if (len == 0)
		return GTS_ERROR;

	// scan
	while(index != len){
		c = *(st + index);
		switch(index){
			case 0:{
                //if (c == '0') tse = GTS_DEC;
                //else 
				if (c >= '0' && c <= '9') tse = GTS_DEC;
				else return GTS_ERROR;
				break;
			}

			case 1:{
				if (c >= '0' && c <= '9') tse = GTS_DEC;
				else if (c == 'x' || c == 'X') tse = GTS_HEX;
				else return GTS_ERROR;
				break;
		    }

			default:{
				if (tse == GTS_DEC && (c >= '0' && c <= '9')){ break;}
				else if (tse == GTS_HEX && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))){ break;}
				else return GTS_ERROR;
				break;
			}

		}//switch(index){
		index++;
	}//while(index != len){

	if (tse == GTS_DEC && len > LEN_STRING_DEC_MAX) return GTS_ERROR;
	if (tse == GTS_HEX && len > LEN_STRING_HEX_MAX) return GTS_ERROR;

	return tse;
}
