

/* mountain.c - Generate the memory mountain. */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "fcycles.h" /* measurement routines */
#include "type_defs.h"
#include "ftime.h"


#define MINBYTES (1 << 14)  /* First working set size */
#define MAXBYTES (1 << 27)  /* Last working set size */
#define MAXSTRIDE 15        /* Stride x8 bytes */
#define MAXELEMS MAXBYTES/sizeof(data_t) 



void       init_data ( data_t *, int );
test_funct test;


int main( int argc, char **argv )
 {

   
   // [ 0 ] ----------------------------------------------------------
   //       set up timing type and pinning
   //
  int timing_type;
  int pinning;
  int correct_overhead;
  timing_type      = ( argc > 1 ? atoi(*(argv+1)) != 0 : TIMING_CLOCK );
  correct_overhead = ( argc > 2 ? atoi(*(argv+2)) : 0 );
  pinning          = ( argc > 3 ? atoi(*(argv+3)) : -1 );

  printf ( "timing using: %s\n",
	   (timing_type == TIMING_CLOCK ? "clock" :
	    "cpu cycles") );
  printf ( "running over array of %lu elements %lu-bytes long\n",
	   MAXELEMS, sizeof(data_t) );

  
  if ( pinning >= 0 )
    pin_to_core( pinning );

  pinning = get_core( NULL );
  if ( pinning >= 0 )
    printf("running on core %d\n", pinning);
  
  
  // ---------------------------------------------------------- [ 0 ]

  

  // [ 1 ] ----------------------------------------------------------
  //       allocate memory and initialize data
  //
  
  int    max_count   = 5;
  int    max_samples = 500;
  double epsilon  = 0.01; 
    
  data_t *data   = (data_t*)aligned_alloc( 64, MAXBYTES );
  
  init_data(data, MAXELEMS); /* Initialize each element in data */

  // --------------------------------------------------------- [ 1 ]

  

  // [ 2 ] ----------------------------------------------------------
  //       run the experiment
  //

  FILE *outfile = fopen( "mountain.dat", "w" );

  if ( outfile == NULL )
    printf ( "It was impossible to write data in file «mountain.dat»\n"
	    "Only screen output will be produced\n" );
  
  
  printf ( "# Memory mountain (MB/sec)\n"
	   "--\t" );
  for (int stride = 1; stride <= MAXSTRIDE; stride++)
    printf("%d\t", stride);
  printf("\n");

  if ( outfile != NULL ) {
    fprintf ( outfile, "# Memory mountain (MB/sec)\n"
	     "--\t" );
    for (int stride = 1; stride <= MAXSTRIDE; stride++)
      fprintf ( outfile, "%d\t", stride );
    fprintf ( outfile, "\n" );
  }


  //
  // run mountain
  //
  for (int size = MAXBYTES; size >= MINBYTES; size >>= 1)
    {
      int log2size_kb = (int)(log2((double)size / 1024.0));
      printf ( "%d\t", log2size_kb );
      if ( outfile != NULL ) fprintf ( outfile, "%d\t", log2size_kb );
      
      int n_elements = size / sizeof(data_t);
      
      for (int stride = 1; stride <= MAXSTRIDE; stride++)
	{

	  double ovrhd;
	  double timing    = ftime( test, data, n_elements, stride, max_count, max_samples, epsilon, correct_overhead, &ovrhd);
	  double bandwidth = (size / (1024.0*1024.0) / stride) / timing;
	  printf("%.0f\t", bandwidth );
	  if ( outfile != NULL ) fprintf ( outfile, "%.0f\t", bandwidth );
	}
      
      printf("\n");
      if ( outfile != NULL ) fprintf ( outfile, "\n" );
    }

  if ( outfile != NULL ) fclose( outfile );
  
  // --------------------------------------------------------- [ 2 ]



  // [ 3 ] ----------------------------------------------------------
  //       release the memory
  //

  free ( data );

  // --------------------------------------------------------- [ 3 ]
  
  return 0;
}




/* init_data - initializes thex array */
void init_data(data_t *data, int N)
{
    for (int i = 0; i < N; i++)
      data[i] = (data_t)i;
}

/* $begin mountainfuns */
/* test - Iterate over first "elems" elements of array "data" with
 *        stride of "stride", using 4x4 loop unrolling.
 */


data_t test(data_t *data, int elems, int stride)

{

  int    sx2 = stride*2, sx3 = stride*3, sx4 = stride*4;
  int    i, length = elems;
  int    limit = length - sx4;  
  data_t acc0 = 0, acc1 = 0, acc2 = 0, acc3 = 0;
  
    /* Combine 4 elements at a time */
    for ( i = 0; i < limit; i += sx4) {
      acc0 = acc0 + data[i];     
      acc1 = acc1 + data[i+stride];
      acc2 = acc2 + data[i+sx2]; 
      acc3 = acc3 + data[i+sx3];
    }

    /* Finish any remaining elements */
    for ( ; i < length; i++)
	acc0 = acc0 + data[i];
    
    return ((acc0 + acc1) + (acc2 + acc3));
}

/*
int test(int *data, int elems, int stride)

{
	// PUT HERE YOUR CODE
	// THAT RETURNS THE SUM
	// OF THE ARRAY data[0:elems] with stride "stride"

  int sum = 0;
  for ( int ii = 0; ii < elems; ii += stride )
    sum += data[ii];
  
  return sum;
}
*/


