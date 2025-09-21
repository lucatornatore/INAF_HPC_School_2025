

/* ────────────────────────────────────────────────────────────────────────── *
 │                                                                            │
 │ This file is part of the exercises for the Lectures on                     │
 │   "Foundations of High Performance Computing"                              │
 │ given at                                                                   │
 │   Master in HPC and                                                        │
 │   Master in Data Science and Scientific Computing                          │
 │ @ SISSA, ICTP and University of Trieste                                    │
 │                                                                            │
 │ contact: luca.tornatore@inaf.it                                            │
 │                                                                            │
 │     This is free software; you can redistribute it and/or modify           │
 │     it under the terms of the GNU General Public License as published by   │
 │     the Free Software Foundation; either version 3 of the License, or      │
 │     (at your option) any later version.                                    │
 │     This code is distributed in the hope that it will be useful,           │
 │     but WITHOUT ANY WARRANTY; without even the implied warranty of         │
 │     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          │
 │     GNU General Public License for more details.                           │
 │                                                                            │
 │     You should have received a copy of the GNU General Public License      │
 │     along with this program.  If not, see <http://www.gnu.org/licenses/>   │
 │                                                                            │
 * ────────────────────────────────────────────────────────────────────────── */


#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 201112L)    // c11
#    define _XOPEN_SOURCE 700
#  elif (__STDC_VERSION__ >= 199901L)  // c99
#    define _XOPEN_SOURCE 600
#  else
#    define _XOPEN_SOURCE 500          // c90
#  endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if _XOPEN_SOURCE >= 600
#  include <strings.h>
#endif
#include <sys/resource.h>
#include <sys/times.h>
#include <time.h>
#include <math.h>

#define STRIDED_WRITE 0
#define STRIDED_READ  1

#define NRUNS    5               // NOTE: AT LEAST THIS MUST BE AT LEAST 2
#if (NRUNS < 2)
#error "NRUNS must be larger than 1"
#endif


#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ),	\
                                          (double)ts.tv_sec +           \
                                          (double)ts.tv_nsec * 1e-9;})


double **allocate_matrix( int N )
{
  double **matrix = (double**)calloc( N, sizeof(double*) );
  if ( matrix == NULL )
    return NULL;
  for ( int row = 0; row < N; row++ ) {
    matrix[row]  = (double*)calloc( N, sizeof(double) );
    if( matrix[row] == NULL )
      return NULL; }
  return matrix;
}


void transpose_strided_write( double **matrix, double **tmatrix, int N )
{
  for (int r = 0; r < N; r++)
    for ( int c = 0; c < N; c++ )
      tmatrix[c][r] = matrix[r][c];
  return;
}


void transpose_contiguous_write( double **matrix, double **tmatrix, int N )
{
  for (int r = 0; r < N; r++)
    for ( int c = 0; c < N; c++ )
      tmatrix[r][c] = matrix[c][r];	
  return;
}

int main(int argc, char **argv)
{
  int     N             = (argc > 1 ? atoi(*(argv+1) : 10000 );
  int     mode          = (argc > 2 ? atoi(*(argv+2)) : STRIDED_WRITE );

  double **matrix  = allocate_matrix( N );
  double **tmatrix = allocate_matrix( N );

  if ( (matrix == NULL) ||
       (tmatrix == NULL) )
    {
      printf("allocation has not succeeded. Try a smaller N\n");
      exit(1);
    }

  /* load the data in the highest cache level that fits */
  for (int i = 0, r = 0; r < N; r++)
    for ( int c = 0; c < N; c++, i++ ) {
      matrix[r][c]  = (double)i;
      tmatrix[r][c] = 0; }
  
  double timing;
            
  if ( mode == STRIDED_WRITE ) {
    transpose_strided_write( matrix, tmatrix, N );
    timing = CPU_TIME;
    transpose_strided_write( matrix, tmatrix, N ); }
  else {  // STRIDED_READ
    transpose_contiguous_write( matrix, tmatrix, N );
    timing = CPU_TIME;
    transpose_contiguous_write( matrix, tmatrix, N ); }

  timing = CPU_TIME - timing;
  printf("timing: %g bw: %g\n", timing, N*N*sizeof(double)/timing/(1024.0*1024.0) );
  
  
  FILE *out = fopen("donotoptimizeout.dat", "w");
  for ( int r = 0; r < N; r++ )
    fwrite( tmatrix[r], N,  sizeof(double), out);
  fclose(out);
  system("rm -f donotoptimizeout.dat");


  for ( int r = 0; r < N; r++ )
    {
      free( (void*)matrix[r] );
      free( (void*)tmatrix[r] );
    }
  free( (void*)matrix );
  free( (void*)tmatrix );

  return 0;
}
