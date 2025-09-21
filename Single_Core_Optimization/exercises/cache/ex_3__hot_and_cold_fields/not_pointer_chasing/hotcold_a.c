
/*
 * This file is part of the exercises for the Lectures on 
 *   "Foundations of High Performance Computing"
 * given at 
 *   Master in HPC and 
 *   Master in Data Science and Scientific Computing
 * @ SISSA, ICTP and University of Trieste
 *  2019
 *
 *     This is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 3 of the License, or
 *     (at your option) any later version.
 *     This code is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License 
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#define _XOPEN_SOURCE 700  // ensures we're using c11 standard
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>


#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ),	\
                                          (double)ts.tv_sec +           \
                                          (double)ts.tv_nsec * 1e-9;})


#ifndef DATASIZE
#define DATASIZE 100
#endif

typedef struct node_t {
  double         key;
  double         data[DATASIZE];
} node;




#define N_default 100000

int main( int argc, char **argv )
{

  // -------------------------------------
  // startup
  
  int N    = N_default;
  
  if ( argc > 1 )
    N = atoi( *(argv+1) );


  // -------------------------------------
  // setup

  printf("creating and initializing %d nodes\n", N ); fflush(stdout);
  srand48( time(NULL) );

  node * nodes = (node*)calloc( N, sizeof(node) );
  
  for( int nn = 0; nn < N; nn++ )
    {
      nodes[nn].key = drand48();
      nodes[nn].data[DATASIZE/2] = (double)nn;
    }
  
  int NSHOTS    = N / 5;
  double sum    = 0;

  printf("now let's search for %d keys\n", NSHOTS); fflush(stdout);
  
  double tstart = CPU_TIME;
  
  for( int ii = 0; ii < NSHOTS; ii++ )
    {      
      double key = nodes[lrand48() % N].key;

      int nn = 0;
      while ( (nn < N) && (nodes[nn].key != key) )
	nn++;
      sum += ( nn < N ? nodes[nn].data[DATASIZE/2] : 0 );
    }

  double et = CPU_TIME - tstart;

  // we need to print the sum, otherwise the compiler would
  // prune the previous loop entirely
  //
  printf("sum result is: %g, timing for %d shots: %g\n", sum, NSHOTS, et );

  free ( nodes );
  
  return 0;
}





