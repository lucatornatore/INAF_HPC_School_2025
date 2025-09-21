

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


void transpose_strided_write( double *matrix, double *tmatrix, int N )
{
  for (int i = 0; i < N; i++) {
    int offset = i*N;
    for(int j = 0, acc = 0; j < N; j++, acc+=N)
      tmatrix[acc + i] = matrix[offset + j]; }
  return;
}


void transpose_contiguous_write( double *matrix, double *tmatrix, int N )
{
  for (int i = 0; i < N; i++) {
    int offset = i*N;
    for(int j = 0, acc = 0; j < N; j++, acc+=N)
      tmatrix[offset + j] = matrix[acc + i]; }
  return;
}


int main(int argc, char **argv)
{
  
  int     N             = (argc > 1 ? atoi(*(argv+1) : 10000 );
  int     mode          = (argc > 2 ? atoi(*(argv+2)) : STRIDED_WRITE );
  double *array, *array_swap;

  array          = (double*) calloc(N*N, sizeof(double));
  array_swap     = (double*) calloc(N*N, sizeof(double));

  if ( (array == NULL) ||
       (array_swap == NULL ) )
    {
      printf("allocation has not succeeded. Try a smaller N\n");
      exit(1);
    }
      
  /* load the data in the highest cache level that fits */
  for (int n = 0; n < N*N; n++) {
    array[n] = (double)n; array_swap[n] = 0.0; }


  double timing;
  
  if ( mode == STRIDED_WRITE )
    {
      transpose_strided_write( array, array_swap, N );
      timing = CPU_TIME;
      for ( int i = 0; i < NRUNS; i++ )
	transpose_strided_write( array, array_swap, N );
      timing = (CPU_TIME - timing)/NRUNS;
    }
  else  // STRIDED_READ
    {
      transpose_contiguous_write( array, array_swap, N );
      timing = CPU_TIME;
      for ( int i = 0; i < NRUNS; i++ )
	transpose_contiguous_write( array, array_swap, N );
      timing = (CPU_TIME - timing)/NRUNS;
    }
  
  printf("timing: %g bw: %g\n", timing, N*N*sizeof(double)/timing/(1024.0*1024.0) );
  
  FILE *out = fopen("donotoptimizeout.dat", "w");
  fwrite( array_swap, N * N,  sizeof(double), out);
  fclose(out);
  system("rm -f donotoptimizeout.dat");
  
  free(array);

  return 0;
}
