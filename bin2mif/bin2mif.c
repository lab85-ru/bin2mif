// Конвертер BIN2MIF
//
//
// Версия v0.1 ----------------------------------------------------------------
// разрядность данных: 8 бит
// разрядность адреса: 16 бит
//-----------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "get_opt.h"
#include "git_commit.h"
#include "version.h"

#define RET_ERROR     (1)
#define RET_OK        (0)



// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int generate_output_file_mif( char *name, unsigned char *buf, size_t buf_size, const int data_width );
int generate_output_file_coe( char *name, unsigned char *buf, size_t buf_size, const int data_width );
int read_file(FILE * fi, uint8_t * buf, size_t *size);
int write_file(FILE * fi, uint8_t * buf, size_t *size);

const char PRINT_HELP[] = {
" MIF: data width = 8,16,32 bit, adr width = 16 bit\n\r"
" COE: data width = 8,16,32 bit, adr width = .. bit\n\r"
" Run & Parameters:\n\r"
" bin2mif -mif -datawidth <x> -i <file-input-bin> -o <file-output.MIF>\n\r"
" bin2mif -coe -datawidth <x> -i <file-input-bin> -o <file-output.COE>\n\r"
" -i             - <file-input>\n\r"
" -o             - <file-output>\n\r"
" -mif           - output file to MIF format\n\r"
" -coe           - output file to COE format\n\r"
" -datawidth <x> - data width (8,16,32) bits to output file\n\r"
"--------------------------------------------------------------------------------\n\r"
};

const char PRINT_TIRE[] = {"================================================================================\n\r"};
const char PRINT_PROG_NAME[] = {" BIN2MIF Converter (c) Sviridov Georgy 2019, www.lab85.ru sgot@inbox.ru\n\r"};

#define FILE_RW_BLOCK_SIZE (1024)
#define FILE_SIZE_MAX      (1*1024*1024*1024)  // максимальный размер файла который можем обрабатывать

//int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char* argv[])
{
	size_t file_size_in = 0;
    size_t result = 0;

	struct stat stat_buf;
	FILE * fi = 0;
	int res = 0;
	uint8_t *mem = NULL;   // ukazatel na buf-mem dla READ_WRITE SPI mem.

	param_opt_st param_opt;        // clean structure
	param_opt.i_file_name = NULL;
	param_opt.o_file_name = NULL;
	param_opt.output_type_file_e = NONE;
	param_opt.data_width = 8;


	printf( PRINT_TIRE );
    printf( PRINT_PROG_NAME );
	printf( " Ver %d.%d.%d\n\r", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );
    printf( " GIT = %s\n\r", git_commit_str );
	printf( PRINT_TIRE );

	if (argc == 1){
	    printf( PRINT_HELP );
		printf("Maximum input file size: %d bytes\n\r", FILE_SIZE_MAX);
		return 0;
	}

	// razbor perametrov
	res = get_opt(argc, argv, &param_opt);
	if (res == -1){
		printf("\n\rERROR: input parameters.\n\r");
		return 1;
	}

	if (param_opt.data_width != 8 && param_opt.data_width != 16 && param_opt.data_width != 32) {
	    printf("\n\rERROR: input parameters Data Width (8, 16, 32 bit).\n\r");
	    return 1;
	}

	{// file load -------------------------------------------------------------
		printf(PRINT_TIRE);

		if (stat(param_opt.i_file_name, &stat_buf) == -1){
			printf("ERROR: fstat return error !\n\r");
			return 1;
		}
		file_size_in = stat_buf.st_size;

    	// proverka diapazona offset+size <= density
		if (file_size_in > FILE_SIZE_MAX ){
			printf("ERROR: file is BIG !!! file_size( %lld ) > chip_size(+offset %lld )\n\r", (long long)stat_buf.st_size, (long long)(FILE_SIZE_MAX));
			return 1;
		}

    	mem = (uint8_t*)malloc( file_size_in );
	    if (mem == NULL){
		    printf("ERROR: malloc( size[  %d  ])\n\r", file_size_in);
		    return 1;
	    }

		printf("File size = %lld bytes.\n\r", (long long)stat_buf.st_size);

		if (file_size_in == 0){
			printf("ERROR: file is empty.\n\r");
			return 1;
		}

		printf("Read file(%s) : ", param_opt.i_file_name);
        // read data from file
		
		fi = fopen(param_opt.i_file_name, "rb");
		if ( fi == NULL ){
			printf("ERROR: open file.\n\r");
			return 1;
		}

		read_file(fi, mem, &result);

        fclose(fi);

		if (result != file_size_in){
    		printf("ERROR: file size = %lld, read size = %lld\n", file_size_in, result);
	    	return 1;
	    }

		//-------------------------------------------------------------------------

		printf("- OK\n\r");
	}// file load -------------------------------------------------------------


	// ========================================================================================

	printf("Write file(%s) : ", param_opt.o_file_name);

	switch (param_opt.output_type_file_e){
    case NONE:// write MIF to file
    case MIF:
	{
		res = generate_output_file_mif( param_opt.o_file_name, mem, file_size_in, param_opt.data_width );
		break;
	}// MIF ------------------------------

    case COE:// write COE to file
	{
        res = generate_output_file_coe( param_opt.o_file_name, mem, file_size_in, param_opt.data_width );
		break;
	}// COE ------------------------------
	}// switch ---------------------------

	if (res == RET_OK){
	    printf("- OK\n\r");
	} else {
	    printf(" - ERROR\n\r");
	}

	free(mem);

	return 0;
}

//=============================================================================
// Read data from file to buff
//=============================================================================
int read_file(FILE * fi, uint8_t * buf, size_t *size)
{
	size_t pos    = 0;
	size_t result = 0;
	size_t r_size = FILE_RW_BLOCK_SIZE;

	pos = 0;
	while( (result = fread( &buf[pos], 1, r_size, fi)) > 0){
		pos = pos + result;
    }

	*size = pos;

	return 0;
}

//=============================================================================
// Write buf to file
//=============================================================================
int write_file(FILE * fi, uint8_t * buf, size_t *size)
{
	size_t pos    = 0;
	size_t result = 0;
	size_t r_size = FILE_RW_BLOCK_SIZE;

	pos = 0;
	while(r_size != 0){
		if (*size - pos > FILE_RW_BLOCK_SIZE){
		    r_size = FILE_RW_BLOCK_SIZE;
		} else {
		    r_size = *size - pos;
		}

		result = fwrite( &buf[pos], 1, r_size, fi);
       
		pos = pos + result;
		if (pos == *size) break;
    }

	*size = pos;

	return 0;
}


//==============================================================================
// generate output file MIF
//==============================================================================
int generate_output_file_mif( char *name, unsigned char *buf, size_t buf_size, const int data_width )
{
    FILE *fo;
    int res = 0;
	uint32_t adr_count = 0;
	uint32_t data = 0;
	uint8_t  d8  = 0;
	uint16_t d16 = 0;
	uint32_t d32 = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_mif: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "-- BIN2MIF converter (c) Sviridov Georgy 2019, www.lab85.ru sgot@inbox.ru\n");
	res |= fprintf(fo, "-- Ver %d.%d.%d\n\r", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

	switch (data_width) {
		case 8:
			res |= fprintf(fo, "DEPTH = %d;\n", buf_size);
			res |= fprintf(fo, "WIDTH = %d;\n", 8);
			break;

		case 16:
			res |= fprintf(fo, "DEPTH = %d;\n", buf_size / 2);
			res |= fprintf(fo, "WIDTH = %d;\n", 16);
			break;

		case 32:
			res |= fprintf(fo, "DEPTH = %d;\n", buf_size / 4);
			res |= fprintf(fo, "WIDTH = %d;\n", 32);
			break;

    } // switch -------------------------------------

    res |= fprintf(fo, "ADDRESS_RADIX = HEX;\n");
    res |= fprintf(fo, "DATA_RADIX = HEX;\n");

    res |= fprintf(fo, "CONTENT\n");
    res |= fprintf(fo, "BEGIN\n");

	adr_count = 0;
    while (adr_count < buf_size) {
        if (res < 0){
            printf("FATAL ERROR: file list write error.\n");
            fclose(fo);
            return RET_ERROR;
        }

		switch (data_width) {
			case 8:
				d8 = *(buf + adr_count);
				res |= fprintf(fo, "%04X : %02X;\n", adr_count, d8 );
				adr_count += 1;
				break;

			case 16:
				d16 = *((uint16_t*)buf + adr_count);
				res |= fprintf(fo, "%04X : %04X;\n", adr_count, d16 );
				adr_count += 1;
				break;

			case 32:
				d32 = *((uint32_t*)buf + adr_count);
				res |= fprintf(fo, "%04X : %08X;\n", adr_count, d32 );
				adr_count += 1;
				break;
		} // switch --------------------------------------------------

		
    } // for --------------------------------------------------------------------

    res = fprintf(fo,"END;\n");

    fclose(fo);
    
    return RET_OK;
}

//==============================================================================
// generate output file COE
//==============================================================================
int generate_output_file_coe( char *name, unsigned char *buf, size_t buf_size, const int data_width )
{
    FILE *fo;
    int res = 0;
	uint32_t adr_count = 0;
	uint32_t data = 0;
	uint16_t d8 = 0;
	uint16_t d16 = 0;
	uint32_t d32 = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_coe: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "; BIN2MIF converter (c) Sviridov Georgy 2019, www.lab85.ru sgot@inbox.ru\n");
	res |= fprintf(fo, "; Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

    res |= fprintf(fo, "memory_initialization_radix=16;\n");
    res |= fprintf(fo, "memory_initialization_vector=\n");

	adr_count = 0;
    while(adr_count < buf_size){
        if (res < 0){
            printf("FATAL ERROR: file list write error.\n");
            fclose(fo);
            return RET_ERROR;
        }

		switch (data_width) {
			case 8:
				d8 = *(buf + adr_count);
				res |= fprintf(fo, "%02x", d8 );
				adr_count += 1;
				break;

			case 16:
				d16 = *((uint16_t*)buf + adr_count);
				res |= fprintf(fo, "%04x", d16 );
				adr_count += 1;
				break;

			case 32:
				d32 = *((uint32_t*)buf + adr_count);
				res |= fprintf(fo, "%08x", d32 );
				adr_count += 1;
				break;


		}

		if (adr_count <= buf_size - 2){
			res |= fprintf(fo, ",\n");
		}

    } // for --------------------------------------------------------------------

    res |= fprintf(fo, ";\n");

	if (res < 0){
		printf("ERROR: Write to file...\n");
		fclose(fo);
		return RET_ERROR;
	}

	fclose(fo);
   
    return RET_OK;
}
