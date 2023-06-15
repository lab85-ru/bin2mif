// Конвертер BIN2MIF
//
// Конвертация двоичных файлов в форматы для загрузки в FPGA:
// Altera/Intel - MIF 
// Xilinx/AMD   - COE
// ModelSim     - MEM
// GoWin        - MI
// Lattice      - HEX MEM
//
// разрядность данных: 8, 16, 32 бит
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
int generate_output_file_mi( char *name, unsigned char *buf, size_t buf_size, const int data_width );  // Gowin
int generate_output_file_mif( char *name, unsigned char *buf, size_t buf_size, const int data_width ); // Altera/Intel
int generate_output_file_coe( char *name, unsigned char *buf, size_t buf_size, const int data_width ); // Xilinx
int generate_output_file_mem( char *name, unsigned char *buf, size_t buf_size, const int data_width ); // ModelSim
int generate_output_file_lattice_mem( char *name, unsigned char *buf, size_t buf_size, const int data_width ); // Lattice hex mem
int read_file(FILE * fi, uint8_t * buf, size_t *size);
int write_file(FILE * fi, uint8_t * buf, size_t *size);

//-----------------------------------------------------------------------------
// Calc: size 2*x >= data_size
//-----------------------------------------------------------------------------
unsigned int calc_data_size_2_mul_x(const unsigned int data_size)
{
    unsigned int i = 0;
    unsigned int a = 0;

	while (a < data_size) {
	    a = 1 << i;
		i++;
	}
    return a;
}

const char PRINT_HELP[] = {
" MIF: data width = 8,16,32 bit, adr width = 16 bit\n"
" COE: data width = 8,16,32 bit, adr width = .. bit\n"
" Run & Parameters:\n"
" bin2mif -mi  -datawidth <x> -i <file-input-bin> -o <file-output.MI>\n"
" bin2mif -mif -datawidth <x> -i <file-input-bin> -o <file-output.MIF>\n"
" bin2mif -coe -datawidth <x> -i <file-input-bin> -o <file-output.COE>\n"
" bin2mif -mem -datawidth <x> -i <file-input-bin> -o <file-output.MEM>\n"
" bin2mif -lattice_mem -datawidth <x> -i <file-input-bin> -o <file-output.MEM>\n"
" -i             - <file-input>\n"
" -o             - <file-output>\n"
" -mi            - output file to MI  format (GoWin)\n"
" -mif           - output file to MIF format (Altera/Intel)\n"
" -coe           - output file to COE format (Xilinx)\n"
" -mem           - output file to MEM format (ModelSim)\n"
" -lattice_mem   - output file to HEX MEM format (Lattice)\n"
" -datawidth <x> - data width (8,16,32) bits to output file\n"
"--------------------------------------------------------------------------------\n"
};

const char PRINT_TIRE[] = {"================================================================================\n"};
const char PRINT_PROG_NAME[] = {" BIN2MIF Converter (c) Sviridov Georgy 2019, www.lab85.ru sgot@inbox.ru\n"};

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
	printf( " Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );
    printf( " GIT = %s\n", git_commit_str );
	printf( PRINT_TIRE );

	if (argc == 1){
	    printf( PRINT_HELP );
		printf("Maximum input file size: %d bytes\n", FILE_SIZE_MAX);
		return 0;
	}

	// razbor perametrov
	res = get_opt(argc, argv, &param_opt);
	if (res == -1){
		printf("\nERROR: input parameters.\n");
		return 1;
	}

	if (param_opt.data_width != 8 && param_opt.data_width != 16 && param_opt.data_width != 32) {
	    printf("\nERROR: input parameters Data Width (8, 16, 32 bit).\n");
	    return 1;
	}

	{// file load -------------------------------------------------------------
		printf(PRINT_TIRE);

		if (stat(param_opt.i_file_name, &stat_buf) == -1){
			printf("ERROR: fstat return error !\n");
			return 1;
		}
		file_size_in = stat_buf.st_size;

    	// proverka diapazona offset+size <= density
		if (file_size_in > FILE_SIZE_MAX ){
			printf("ERROR: file is BIG !!! file_size( %lld ) > chip_size(+offset %lld )\n", (long long)stat_buf.st_size, (long long)(FILE_SIZE_MAX));
			return 1;
		}

    	mem = (uint8_t*)malloc( file_size_in );
	    if (mem == NULL){
		    printf("ERROR: malloc( size[  %d  ])\n", file_size_in);
		    return 1;
	    }

		printf("File size = %lld bytes.\n", (long long)stat_buf.st_size);

		if (file_size_in == 0){
			printf("ERROR: file is empty.\n");
			return 1;
		}

		printf("Read file(%s) : ", param_opt.i_file_name);
        // read data from file
		
		fi = fopen(param_opt.i_file_name, "rb");
		if ( fi == NULL ){
			printf("ERROR: open file.\n");
			return 1;
		}

		read_file(fi, mem, &result);

        fclose(fi);

		if (result != file_size_in){
    		printf("ERROR: file size = %lld, read size = %lld\n", file_size_in, result);
	    	return 1;
	    }

		//-------------------------------------------------------------------------

		printf("- OK\n");
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

	case MEM:// write MEM to file
	{
        res = generate_output_file_mem( param_opt.o_file_name, mem, file_size_in, param_opt.data_width );
		break;
	}// MEM ------------------------------

	case LATTICE_MEM:// write LATTICE_MEM to file
	{
        res = generate_output_file_lattice_mem( param_opt.o_file_name, mem, file_size_in, param_opt.data_width );
		break;
	}// MEM ------------------------------

	case MI:// write MI to file
	{
        res = generate_output_file_mi( param_opt.o_file_name, mem, file_size_in, param_opt.data_width );
		break;
	}// MEM ------------------------------

	}// switch ---------------------------

	if (res == RET_OK){
	    printf("- OK\n");
	} else {
	    printf(" - ERROR\n");
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
	size_t in_data_length = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_mif: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "-- %s", PRINT_PROG_NAME);
	res |= fprintf(fo, "-- Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

	switch (data_width) {
		case 8:
			in_data_length = buf_size;
			break;
	
		case 16:
			in_data_length = buf_size / 2;
			break;
	
		case 32:
			in_data_length = buf_size / 4;
			break;
	
	} // switch -------------------------

	switch (data_width) {
		case 8:
			res |= fprintf(fo, "DEPTH = %d;\n", in_data_length);
			res |= fprintf(fo, "WIDTH = %d;\n", 8);
			break;

		case 16:
			res |= fprintf(fo, "DEPTH = %d;\n", in_data_length);
			res |= fprintf(fo, "WIDTH = %d;\n", 16);
			break;

		case 32:
			res |= fprintf(fo, "DEPTH = %d;\n", in_data_length);
			res |= fprintf(fo, "WIDTH = %d;\n", 32);
			break;

    } // switch -------------------------------------

    res |= fprintf(fo, "ADDRESS_RADIX = HEX;\n");
    res |= fprintf(fo, "DATA_RADIX = HEX;\n");

    res |= fprintf(fo, "CONTENT\n");
    res |= fprintf(fo, "BEGIN\n");

	adr_count = 0;
    while (adr_count < in_data_length) {
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
	size_t in_data_length = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_coe: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "; %s", PRINT_PROG_NAME);
	res |= fprintf(fo, "; Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

    res |= fprintf(fo, "memory_initialization_radix=16;\n");
    res |= fprintf(fo, "memory_initialization_vector=\n");

	switch (data_width) {
		case 8:
			in_data_length = buf_size;
			break;
	
		case 16:
			in_data_length = buf_size / 2;
			break;
	
		case 32:
			in_data_length = buf_size / 4;
			break;
	
	} // switch -------------------------

	adr_count = 0;
    while(adr_count < in_data_length){
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

		} // switch --------------------------------

		if (adr_count < in_data_length){
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

//==============================================================================
// generate output file MEM (For ModelSim format)
//==============================================================================
int generate_output_file_mem( char *name, unsigned char *buf, size_t buf_size, const int data_width )
{
    FILE *fo;
    int res = 0;
	uint32_t adr_count = 0;
	uint32_t data = 0;
	uint16_t d8 = 0;
	uint16_t d16 = 0;
	uint32_t d32 = 0;
	size_t in_data_length = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_mem: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "// %s", PRINT_PROG_NAME);
	res |= fprintf(fo, "// Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

    res |= fprintf(fo, "@00000000\n");

	switch (data_width) {
		case 8:
			in_data_length = buf_size;
			break;
	
		case 16:
			in_data_length = buf_size / 2;
			break;
	
		case 32:
			in_data_length = buf_size / 4;
			break;
	
	} // switch -------------------------

	adr_count = 0;
    while(adr_count < in_data_length){
        if (res < 0){
            printf("FATAL ERROR: file list write error.\n");
            fclose(fo);
            return RET_ERROR;
        }

		switch (data_width) {
			case 8:
				d8 = *(buf + adr_count);
				res |= fprintf(fo, "%02x\n", d8 );
				adr_count += 1;
				break;

			case 16:
				d16 = *((uint16_t*)buf + adr_count);
				res |= fprintf(fo, "%04x\n", d16 );
				adr_count += 1;
				break;

			case 32:
				d32 = *((uint32_t*)buf + adr_count);
				res |= fprintf(fo, "%08x\n", d32 );
				adr_count += 1;
				break;

		} // switch --------------------------------

    } // for --------------------------------------------------------------------

	if (res < 0){
		printf("ERROR: Write to file...\n");
		fclose(fo);
		return RET_ERROR;
	}

	fclose(fo);
   
    return RET_OK;
}

//==============================================================================
// generate output file LATTICE MEM HEX (For Lattice MEM HEX format)
//==============================================================================
int generate_output_file_lattice_mem( char *name, unsigned char *buf, size_t buf_size, const int data_width )
{
    FILE *fo;
    int res = 0;
	uint32_t adr_count = 0;
	uint32_t data = 0;
	uint16_t d8 = 0;
	uint16_t d16 = 0;
	uint32_t d32 = 0;
	size_t in_data_length = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_mem: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "// %s", PRINT_PROG_NAME);
	res |= fprintf(fo, "// Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

	res |= fprintf(fo, "#Format=Hex\n");

	switch (data_width) {
		case 8:
			in_data_length = buf_size;
            res |= fprintf(fo, "#Depth=%d\n", calc_data_size_2_mul_x(in_data_length) );
            res |= fprintf(fo, "#Width=8\n");
			break;
	
		case 16:
			in_data_length = buf_size / 2;
            res |= fprintf(fo, "#Depth=%d\n", calc_data_size_2_mul_x(in_data_length) );
            res |= fprintf(fo, "#Width=16\n");
			break;
	
		case 32:
			in_data_length = buf_size / 4;
            res |= fprintf(fo, "#Depth=%d\n", calc_data_size_2_mul_x(in_data_length) );
            res |= fprintf(fo, "#Width=32\n");
			break;
	
	} // switch -------------------------

	// radix:
	//Binary: 0
	//Octal: 1
	//Decimal: 2
	//Hexadecimal: 3
	res |= fprintf(fo, "#AddrRadix=3\n");
	res |= fprintf(fo, "#DataRadix=3\n");
	res |= fprintf(fo, "#Data\n");

	adr_count = 0;
    while(adr_count < in_data_length){
        if (res < 0){
            printf("FATAL ERROR: file list write error.\n");
            fclose(fo);
            return RET_ERROR;
        }

		switch (data_width) {
			case 8:
				d8 = *(buf + adr_count);
				res |= fprintf(fo, "%02x\n", d8 );
				adr_count += 1;
				break;

			case 16:
				d16 = *((uint16_t*)buf + adr_count);
				res |= fprintf(fo, "%04x\n", d16 );
				adr_count += 1;
				break;

			case 32:
				d32 = *((uint32_t*)buf + adr_count);
				res |= fprintf(fo, "%08x\n", d32 );
				adr_count += 1;
				break;

		} // switch --------------------------------

    } // for --------------------------------------------------------------------

	if (res < 0){
		printf("ERROR: Write to file...\n");
		fclose(fo);
		return RET_ERROR;
	}

	fclose(fo);
   
    return RET_OK;
}

//==============================================================================
// generate output file MI (For GoWin format)
//==============================================================================
int generate_output_file_mi( char *name, unsigned char *buf, size_t buf_size, const int data_width )
{
    FILE *fo;
    int res = 0;
	uint32_t adr_count = 0;
	uint32_t data = 0;
	uint16_t d8 = 0;
	uint16_t d16 = 0;
	uint32_t d32 = 0;
	size_t in_data_length = 0;

    if (name == NULL){
        printf("FATAL ERROR: generate_output_file_mi: name = NULL.\n");
        return RET_ERROR;
    }

    fo = fopen(name, "wt");

    if (fo == NULL){
        printf("FATAL ERROR: generate_output_file fopen return error.\n");
        return RET_ERROR;
    }

	res |= fprintf(fo, "# %s", PRINT_PROG_NAME);
	res |= fprintf(fo, "# Ver %d.%d.%d\n", SOFT_VER_H, SOFT_VER_L, SOFT_VER_Y );

	res |= fprintf(fo, "#File_format=Hex\n");

	switch (data_width) {
		case 8:
			in_data_length = buf_size;
            res |= fprintf(fo, "#Address_depth=%d\n", in_data_length);
			res |= fprintf(fo, "#Data_width=8\n");
			break;
	
		case 16:
			in_data_length = buf_size / 2;
            res |= fprintf(fo, "#Address_depth=%d\n", in_data_length);
			res |= fprintf(fo, "#Data_width=16\n");
			break;
	
		case 32:
			in_data_length = buf_size / 4;
            res |= fprintf(fo, "#Address_depth=%d\n", in_data_length);
			res |= fprintf(fo, "#Data_width=32\n");
			break;
	
	} // switch -------------------------

	adr_count = 0;
    while(adr_count < in_data_length){
        if (res < 0){
            printf("FATAL ERROR: file list write error.\n");
            fclose(fo);
            return RET_ERROR;
        }

		switch (data_width) {
			case 8:
				d8 = *(buf + adr_count);
				res |= fprintf(fo, "%02x\n", d8 );
				adr_count += 1;
				break;

			case 16:
				d16 = *((uint16_t*)buf + adr_count);
				res |= fprintf(fo, "%04x\n", d16 );
				adr_count += 1;
				break;

			case 32:
				d32 = *((uint32_t*)buf + adr_count);
				res |= fprintf(fo, "%08x\n", d32 );
				adr_count += 1;
				break;

		} // switch --------------------------------

    } // for --------------------------------------------------------------------

	if (res < 0){
		printf("ERROR: Write to file...\n");
		fclose(fo);
		return RET_ERROR;
	}

	fclose(fo);
   
    return RET_OK;
}
