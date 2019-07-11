// ��������� BIN2MIF
//
//
// ������ v0.1 ----------------------------------------------------------------
// ����������� ������: 8 ���
// ����������� ������: 16 ���
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
int generate_output_file_mif( char *name, unsigned char *buf, size_t buf_size );
int read_file(FILE * fi, uint8_t * buf, size_t *size);
int write_file(FILE * fi, uint8_t * buf, size_t *size);

const char PRINT_HELP[] = {
" Run & Parameters:\n\r"
" bin2mif -i <file-input-bin> -o <file-output.MIF>\n\r"
" -i <file-input>\n\r"
" -o <file-output>\n\r"
"--------------------------------------------------------------------------------\n\r"
};

const char PRINT_TIRE[] = {"================================================================================\n\r"};
const char PRINT_PROG_NAME[] = {" BIN2MIF Converter (c) Sviridov Georgy 2019, www.lab85.ru sgot@inbox.ru\n\r"};

#define FILE_RW_BLOCK_SIZE (1024)
#define FILE_SIZE_MAX      (1*1024*1024*1024)  // ������������ ������ ����� ������� ����� ������������

//int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char* argv[])
{
	size_t file_size_in = 0;
    size_t result = 0;
	uint32_t crc32_mem = 0;
	size_t firmware_header_i_size = 0; // ������ ��������� �������� �����
	size_t firmware_header_o_size = 0; // ������ ��������� ��������� �����
	uint32_t f_hlen  = 0;
	uint32_t f_hcrc32 = 0;

	struct stat stat_buf;
	FILE * fi = 0;
	int res = 0;
	uint8_t *mem = NULL;   // ukazatel na buf-mem dla READ_WRITE SPI mem.
	uint8_t *mem_cmp = NULL;   // ukazatel na buf-mem dla COMPARE FLASH - FILE.

	uint8_t * f_head_p = 0;       // ��������� �� ���������
	int header_present = 0;         // =0 header not present, =1 header present !
	int header_input_size = 0;      // ������ ��������� �������� �����

	param_opt_st param_opt;        // clean structure
	param_opt.i_file_name = NULL;
	param_opt.o_file_name = NULL;


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

// write MIF to file
	printf("Write file(%s) : ", param_opt.o_file_name);
	if (generate_output_file_mif( param_opt.o_file_name, mem, file_size_in ) == RET_OK){
	    printf("- OK\n\r");
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
int generate_output_file_mif( char *name, unsigned char *buf, size_t buf_size )
{
    FILE *fo;
    int res = 0;
	uint32_t adr_count = 0;
	uint32_t data = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_mif: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "-- BIN2MIF converter (c) Svridov Georgy 2019, www.lab85.ru sgot@inbox.ru\n");
	res |= fprintf(fo, "-- Ver %d.%d.%d\n\r", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

    res |= fprintf(fo, "DEPTH = %d;\n", buf_size);
    res |= fprintf(fo, "WIDTH = %d;\n", 8);

    res |= fprintf(fo, "ADDRESS_RADIX = HEX;\n");
    res |= fprintf(fo, "DATA_RADIX = HEX;\n");

    res |= fprintf(fo, "CONTENT\n");
    res |= fprintf(fo, "BEGIN\n");

    for(adr_count=0; adr_count<buf_size; adr_count += 1){
        if (res < 0){
            printf("FATAL ERROR: file list write error.\n");
            fclose(fo);
            return RET_ERROR;
        }

        //res |= fprintf(fo, "%04X : %04X;\n", adr_count, *(buf + adr_count) );
		res |= fprintf(fo, "%04X : %02X;\n", adr_count, *(buf + adr_count) );
        
    } // for --------------------------------------------------------------------

    res = fprintf(fo,"END;\n");

    fclose(fo);
    
    return RET_OK;
}