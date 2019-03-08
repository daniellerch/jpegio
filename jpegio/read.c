

#include "read.h"

/* The default output_message routine causes a seg fault in Matlab,
 * at least on Windows.  Its generally used to emit warnings, since
 * fatal errors call the error_exit routine, so we emit a Matlab
 * warning instead.  If desired, warnings can be turned off by the
 * user with "warnings off".   -- PAS 10/03
*/
METHODDEF(void)
my_output_message(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    // Create the message
    (*cinfo->err->format_message) (cinfo, buffer);

    printf("[LIBJPEG MESSAGE] %s\n", buffer);
}


METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    // cinfo->err really points to a my_error_mgr struct, so coerce pointer 
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    // create the message
    (*cinfo->err->format_message) (cinfo, buffer);
    printf("[LIBJPEG ERROR]: %s\n", buffer);

    // return control to the setjmp point
    longjmp(myerr->setjmp_buffer, 1);
}


unsigned char* _read_jpeg_decompress_struct(
    FILE* infile,
    j_decompress_ptr cinfo,
    my_error_ptr jerr)
{
    unsigned char* mem_buffer = NULL;
    unsigned long mem_size = 0;

    // Set up the normal JPEG error routines, then override error_exit. 
    cinfo->err = jpeg_std_error(&jerr->pub);    
    jerr->pub.error_exit = my_error_exit;
    jerr->pub.output_message = my_output_message;
    
    
    // Establish the setjmp return context for error_exit to use. 
    if (setjmp(jerr->setjmp_buffer))
    {
        jpeg_destroy_decompress(cinfo);
        printf("[LIBJPEG] Error occurs during reading file.\n");
        return NULL;
    }
    
    // Get the size of file
    fseek(infile, 0, SEEK_END);
    mem_size = ftell(infile);
    rewind(infile);
    
    // Allocate memory buffer for the jpeg file.
    mem_buffer = (unsigned char*) malloc(mem_size + 100);

    int num_bytes = fread(mem_buffer, sizeof(unsigned char), mem_size, infile);

    // Initialize JPEG decompression cinfo object 
    jpeg_create_decompress(cinfo);
    
    // Replace jpeg_stdio_src(cinfo, infile) with jpeg_mem_src
    jpeg_mem_src(cinfo, mem_buffer, mem_size);

    // Save contents of markers 
    // jpeg_save_markers(cinfo, JPEG_COM, 0xFFFF);

    // Read header
    jpeg_read_header(cinfo, TRUE);

    // For some reason out_color_components isn't being set by
    // jpeg_read_header, so we will infer it from out_color_space: 
    switch (cinfo->out_color_space)
    {
    case JCS_GRAYSCALE:
        cinfo->out_color_components = 1;
        break;
    case JCS_RGB:
        cinfo->out_color_components = 3;
        break;
    case JCS_YCbCr:
        cinfo->out_color_components = 3;
        break;
    case JCS_CMYK:
        cinfo->out_color_components = 4;
        break;
    case JCS_YCCK:
        cinfo->out_color_components = 4;
        break;
    }
    
    return mem_buffer;
}


int _get_num_quant_tables(const j_decompress_ptr cinfo)
{
    int n;
    int num_tables = 0;
    
    // Count the number of tables.
    for (n = 0; n < NUM_QUANT_TBLS; n++)
    {
        if (cinfo->quant_tbl_ptrs[n] != NULL)
        {
            num_tables++;
        }
    }
    return num_tables;    
}

void _read_quant_tables(UINT16 tables[], const j_decompress_ptr cinfo)
{
    size_t n, i, j;
    JQUANT_TBL* quant_ptr;
    UINT16* vals;
    UINT16* table;

    
    for (n = 0; n < NUM_QUANT_TBLS; n++)
    {
        if (cinfo->quant_tbl_ptrs[n] != NULL)
        {
            table = &tables[n*(DCTSIZE*DCTSIZE)];
            quant_ptr = cinfo->quant_tbl_ptrs[n];
            for (i = 0; i < DCTSIZE; i++)
            {
                for (j = 0; j < DCTSIZE; j++)
                {
                    vals = &(quant_ptr->quantval);
                    table[i*DCTSIZE + j] = vals[i*DCTSIZE + j];
                }
            }
        }
    }
    
}

void _get_size_dct_block(struct DctBlockArraySize* blkarr_size,
                         const j_decompress_ptr cinfo,
                         int ci)
{
    jpeg_component_info *compptr;

    compptr = cinfo->comp_info + ci;
    blkarr_size->nrows = compptr->height_in_blocks;
    blkarr_size->ncols = compptr->width_in_blocks;
}


void _read_coef_array(JCOEF* arr,
                      j_decompress_ptr cinfo,
                      jvirt_barray_ptr coef_array,
                      struct DctBlockArraySize blkarr_size)
{
    JBLOCKARRAY buffer;
    JCOEFPTR bufptr;
    JDIMENSION ir_blk, ic_blk;
    JDIMENSION ir_arr, ic_arr;
    int i, j;

    // Copy coefficients from virtual block arrays
    for (ir_blk = 0; ir_blk < blkarr_size.nrows; ir_blk++)
    {
        buffer = (cinfo->mem->access_virt_barray) ((j_common_ptr)cinfo, coef_array, ir_blk, 1, FALSE);
        for (ic_blk = 0; ic_blk < blkarr_size.ncols; ic_blk++)
        {
            bufptr = buffer[0][ic_blk];            
            ir_arr = DCTSIZE2*blkarr_size.ncols*ir_blk;
            
            // Read a single block of DCT coefficients
            for (i = 0; i < DCTSIZE; i++) // for each row in block
            {                
                ic_arr = DCTSIZE*ic_blk;
                for (j = 0; j < DCTSIZE; j++) // for each column in block
                {
                    *(arr + ir_arr + ic_arr) = bufptr[i*DCTSIZE + j];
                    ic_arr++;
                    
                }
                ir_arr += DCTSIZE*blkarr_size.ncols;
            }
        }
    }
}



void _read_coef_array_zigzag_dct_1d(JCOEF* arr,
                                    j_decompress_ptr cinfo,
                                    jvirt_barray_ptr coef_array,
                                    struct DctBlockArraySize blkarr_size)
{
    JBLOCKARRAY buffer;
    JCOEFPTR bufptr;
    JDIMENSION ir_blk, ic_blk;
    JDIMENSION ir_arr, ic_arr;
    int i, j;

    // Copy coefficients from virtual block arrays
    for (ir_blk = 0; ir_blk < blkarr_size.nrows; ir_blk++)
    {
        buffer = (cinfo->mem->access_virt_barray) ((j_common_ptr)cinfo, coef_array, ir_blk, 1, FALSE);
        for (ic_blk = 0; ic_blk < blkarr_size.ncols; ic_blk++)
        {
            bufptr = buffer[0][ic_blk];            
            ir_arr = DCTSIZE2*blkarr_size.ncols*ir_blk;
            
            // Read a single block of DCT coefficients
            for (i = 0; i < DCTSIZE; i++) // for each row in block
            {                
                ic_arr = DCTSIZE*ic_blk;
                for (j = 0; j < DCTSIZE; j++) // for each column in block
                {
                    *(arr + ir_arr + ic_arr) = bufptr[i*DCTSIZE + j];
                    ic_arr++;
                    
                }
                ir_arr += DCTSIZE*blkarr_size.ncols;
            }
        }
    }
}

void _dealloc_jpeg_decompress(j_decompress_ptr cinfo)
{
    jpeg_finish_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);
    
}

void _dealloc_memory_buffer(unsigned char* mem_buffer)
{
    if (mem_buffer != NULL)
    {
        free(mem_buffer);
    }
}