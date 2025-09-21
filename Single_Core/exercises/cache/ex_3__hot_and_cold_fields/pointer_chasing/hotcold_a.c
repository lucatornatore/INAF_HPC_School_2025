
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
  struct node_t *next;
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

  double *keys  = (double*)calloc( N, sizeof(double));
  node   *last  = NULL;
  node   *first = NULL;

  printf("creating and initializing %d nodes\n", N ); fflush(stdout);
  srand48( time(NULL) );

  for( int nn = 0; nn < N; nn++ )
    {
      node *new = (node*)calloc( 1, sizeof(node) );
      if ( last != NULL )
	last->next = new;
      else
	first = new;
      new ->key  = drand48();
      keys[nn] = new->key;
      new ->next = NULL;
      new->data[DATASIZE/2] = (double)nn;
      last = new;
    }


  int NSHOTS    = N/5;
  double sum    = 0;

  printf("now let's search for %d of them\n", NSHOTS); fflush(stdout);
  
  double tstart = CPU_TIME;
  
  for( int ii = 0; ii < NSHOTS; ii++ )
    {      
      double key = keys[lrand48() % N];
      node *target = first;

      while ( (target != NULL) && (target->key != key) )
	target = target->next;
      sum += ( (target != NULL) ? (target->data)[DATASIZE/2] : 0 );

    }

  double et = CPU_TIME - tstart;

  // we need to print the sum, otherwise the compiler would
  // prune the previous loop entirely
  //
  printf("sum result is: %g, timing for %d shots: %g\n", sum, NSHOTS, et );

  node *target = first;
  while( target->next != NULL )
    {
      node *tmp = target->next;
      free(target);
      target = tmp;
    }
  
  return 0;
}





