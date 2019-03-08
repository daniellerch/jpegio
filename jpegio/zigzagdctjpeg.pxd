
from libc.stdio cimport FILE
cimport numpy as np

from read cimport *
from .decompressedjpeg cimport DecompressedJpeg


"""
cdef extern from "dctblockarraysize.h":
    cdef struct DctBlockArraySize:
        JDIMENSION nrows
        JDIMENSION ncols
                
cdef extern from "read.h":
    cdef struct my_error_mgr:
        pass    

    ctypedef my_error_mgr* my_error_ptr

#    unsigned char* _read_jpeg_decompress_struct(FILE* infile,
#                                                j_decompress_ptr cinfo,
#                                                my_error_ptr jerr)

    void _read_coef_array(JCOEF* arr,
                          j_decompress_ptr cinfo,
                          jvirt_barray_ptr coef_array,
                          DctBlockArraySize blkarr_size)

    void _get_size_dct_block(DctBlockArraySize* blkarr_size,
                             const j_decompress_ptr cinfo,
                             int ci)
    
    void _read_coef_array_zigzag_dct_1d(JCOEF* arr,
                                        j_decompress_ptr cinfo,
                                        jvirt_barray_ptr coef_array,
                                        DctBlockArraySize blkarr_size)
"""

cdef class ZigzagDctJpeg(DecompressedJpeg):
#    cdef public list comp_info
#    cdef public np.ndarray quant_tables
#    cdef public list coef_arrays
#    
#    cdef unsigned char* _mem_buff
#    cdef FILE* _infile
#    cdef j_decompress_ptr _cinfo
#    cdef my_error_ptr _jerr    
#        
#    cdef _get_comp_info(self)
#    cdef _get_quant_tables(self)
    cdef _get_dct_coefficients(self)

#    cpdef read(self, fname)
#    cpdef get_coef_block(self, c, i, j)
#    cpdef get_coef_block_array_shape(self, c)
    
