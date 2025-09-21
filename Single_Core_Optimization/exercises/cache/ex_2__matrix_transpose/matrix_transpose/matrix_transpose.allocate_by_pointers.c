

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
#include "mypapi.h"


extern int TESTS_QUIET;

#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ),	\
                                          (double)ts.tv_sec +           \
                                          (double)ts.tv_nsec * 1e-9;})

#define STRIDED_WRITE 0
#define STRIDED_READ  1

char *mode_labels[2] = {"strided write", "contiguous write"};

#define NRUNS    5
#define L1WORDS (32 * 1024 / 8)  // double capacity of L1
#define L2WORDS (256 * 1024 / 8) // double capacity of L2

#define dprintf( L, ... ) { if (verbose_level >= (L) ) printf( __VA_ARGS__ ); }


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
  if ( (argc>1) &&
       ( (strncmp(*(argv+1), "-h", 2) == 0) ||
	 (strncmp(*(argv+1), "--help", 6) == 0) ) ) {    
    printf("Expected arguments: "
	   " [ run mode <0|1> ] [ avoid powers of 2 <0|1> ] [ verbose level ]\n"
	   "  run mode is STRIDED_WRITE (=0, default) or CONTIGUOUS_WRITE (=1)\n"
	   "  avoid powers of 2 (=1 default) to skip cache resonance\n"
	   
	   "  verbose level is >=0 (default =0, minimize the output)\n"
	   "\n"
	   "I need at least the size of the matrix\n"); exit(1); }

  int     mode          = (argc > 1 ? atoi(*(argv+1)) : STRIDED_WRITE );
  int     avoid_pwr2    = (argc > 2 ? (atoi(*(argv+2))>0) : 1 );
  int     verbose_level = (argc > 3 ? atoi(*(argv+3)) : 0 );
  int     N, Nmax;
  double  ** matrix;
  double  ** tmatrix;

  if ( mode > 1 )
    {
      printf("run mode can only have value in the range [0:1]\n");
      exit(1);
    }

  PAPI_INIT;
    
  /* declare an array that is more than twice the L2 cache size */
  Nmax           = L2WORDS/2;

  matrix  = allocate_matrix( Nmax );
  tmatrix = allocate_matrix( Nmax );

  if ( (matrix == NULL) ||
       (tmatrix == NULL) )
    {
      printf("allocation has not succeeded. Try a smaller N\n");
      exit(1);
    }

  
  int Nmin  = 8;
  int Nstep = L1WORDS/4;
  int nruns = NRUNS;
  
  dprintf(0, "max N will be %d, Nsteps will be %d, run mode: %s\n",
	  Nmax, Nstep, mode_labels[mode]);

  
  for (N = Nmin; N <= Nmax; )
    {

      int _N_ = N - avoid_pwr2;
      dprintf(1, "Run: data set size=%d x %d\n", _N_, _N_);
      
     #if defined(USE_PAPI)
      unsigned long long values[PAPI_EVENTS_NUM] = {0};
     #endif
       
      /* warm-up the cache */
      for (int i = 0, r = 0; r < _N_; r++)
	for ( int c = 0; c < _N_; c++, i++ )
	  matrix[r][c] = (double)i, tmatrix[r][c] = 0.0;
      
      unsigned int N2 = _N_*_N_;
      double size     = nruns * (double)N2 * sizeof(double) / (1024*1024);
      
      double timing = 0;
      /* run the experiment */
	  		  
      if ( mode == STRIDED_WRITE )
	for (int cc = 0; cc < nruns; cc++) {
	  PAPI_FLUSH;
	  PAPI_START_CNTR;
	  
	  double tstart = CPU_TIME;
	  transpose_strided_write( matrix, tmatrix, _N_ ); 
	  timing += CPU_TIME - tstart;
	  
	  PAPI_STOP_CNTR;
	  if( (nruns==1) || (cc > 0) )
	    PAPI_ACC_CNTR(values);	    
	}
      

      else  // STRIDED_READ
	for (int cc = 0; cc < nruns; cc++) {
	  PAPI_FLUSH;
	  PAPI_START_CNTR;
	  
	  double tstart = CPU_TIME;	 
	  transpose_contiguous_write( matrix, tmatrix, _N_ );	  
	  timing += CPU_TIME - tstart;
	  
	  PAPI_STOP_CNTR; 
	  if( (nruns==1) || (cc > 0) )
	    PAPI_ACC_CNTR(values);
	}

      dprintf(0, "size: %d [ %2d nruns] time: %5.3g s [total: %5.3g s] bw: %g MB/s ",
	      _N_, nruns, timing/nruns, timing, size / timing);
      
     #if defined(USE_PAPI)
      int nruns_1 = nruns - (nruns>1);
      dprintf(0, "cycles: %lld "
	      "cycles_per_loc: %9.5f "
	      "L1miss: %lld L1miss_frac: %9.5f "
	      "L2miss: %lld L2miss_frac: %9.5f\n",
	      values[0], values[0]/(2.*nruns_1*N2),
	      values[1], (double)values[1]/(2.* nruns_1*N2),
	      values[2], (double)values[2]/(2.* nruns_1*N2));
     #else
      dprintf(0, "\n");
     #endif
      
      
      if (N <= L1WORDS)
	N*=2;
      else
	N += Nstep;
      
      nruns -= (int)timing / 2;
      if ( nruns < 1 )
	nruns = 1;

  }

  
  for ( int r = 0; r < Nmax; r++ )
    {
      free( (void*)matrix[r] );
      free( (void*)tmatrix[r] );
    }
  free( (void*)matrix );
  free( (void*)tmatrix );


  return 0;
}
