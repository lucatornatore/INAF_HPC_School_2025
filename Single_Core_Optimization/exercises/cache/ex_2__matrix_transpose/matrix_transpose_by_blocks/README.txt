
command-line arguments:
BLOCK_SIZE NROWS NCOLS CHECK

The difference between v0, v1 and v2 is in the way the memory is allocated

==========================================
v0.c
------------------------------------------
matrices is represented as array of pointers
data_t **matrix;


==========================================
v1.c
------------------------------------------
matrices are represented as 1D array
data_t *matrix


==========================================
v2.c
------------------------------------------
a smarter way, unique allocation but addressable as [][]
data_t (* matrix)[ncols]  = calloc( nrows*ncols, sizeof( *matrix  ) );
