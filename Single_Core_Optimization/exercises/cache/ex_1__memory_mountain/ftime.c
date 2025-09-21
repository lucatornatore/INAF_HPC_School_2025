/* Compute time used by a function f that takes two integer args */
#include <stdlib.h>
#include <stdio.h>

#include "fcycles.h"
#include "ftime.h"


void     add_sample(double, int, int);
int      has_converged(int, double);

static double *values = NULL;


void init_values ( int max_count )
{
  if ( values != NULL )
    free ( values );
  values = calloc( max_count, sizeof(double) );
  return;
}


//
// Add new sample
//
void add_sample(double value, int current_count, int max_count)
{
  int pos = 0;
  
  if (current_count < max_count) {
    pos = current_count-1;
    values[pos] = value;
  } else if (value < values[max_count-1]) {
    pos = max_count - 1;
    values[pos] = value;
  }

  /* Insertion sort */
  while ( (pos > 1) &&
	  (values[pos-1] > values[pos]) )
    {
      double temp = values[pos-1];
      values[pos-1] = values[pos];
      values[pos] = temp;
      pos--;
    }

  return;
}


/* What is relative error for kth smallest sample */
double err(int k, int max_count)
{
  if (k < max_count)
    return 1.0;
  return (values[k-1] - values[0])/values[0];
}

/* Have k minimum measurements converged within epsilon? */
int has_converged(int max_count, double epsilon)
{
  return epsilon*values[0] >= (values[max_count-1] - values[0]);
}

static int sink;
void clear_cache( void )
{
 #define CACHELINE 64
  int stride = ( CACHELINE  / (sizeof(int)) );

 #define MEMSIZE (1<<20)
  int size = ( MEMSIZE / sizeof(int) );

  int *stuff = (int*)malloc( MEMSIZE );

  int x = sink; 
  for (int i = 0; i < size; i += stride)
    x += stuff[i];
  sink = x;
  
  free( stuff );
  return;
}


double ftime (test_funct f,          // the test function to be used       
	      data_t *param0,	     // the pointer to data                
	      int param1,	     // how many elements in the data array
	      int param2,	     // he stride to be used               
	      int max_count,	     // how many best timings              
	      int max_samples,	     // how many samples                   
	      double epsilon, 	     // the convergence threshold
	      int correct_overhead,
	      double *overhead
	      )

{
  init_values( max_count );
  
 #define WARM_CYCLES 2
  double  _overhead_  = get_timing_overhead( );
  int     nsamples  = 0;
  int     stop      = 0;

  do {
    
    nsamples++;
    clear_cache();

    volatile int res = f(param0, param1, param2);   // warm-up the cache
    
    int rep = 1;
    mytimer_t time_start = get_time_start();

    res = f(param0, param1, param2);

    mytimer_t time_end = get_time_end(NULL);
    double timing = get_timing(time_start, time_end, 0);
    
    while ( _overhead_ / timing > 0.05 )
      {
	rep += 2;
	time_start = get_time_start();

	for ( int r = 0; r < rep; r++ )
	  res = f(param0, param1, param2);
	
	time_end = get_time_end(NULL);
	timing = get_timing(time_start, time_end, 0);
      }
    timing /= rep;
    
    if ( correct_overhead && (timing > _overhead_ ) )
      {
	timing -= _overhead_;
	if (overhead != NULL )
	  *overhead = _overhead_;
      }
        
    add_sample(timing, nsamples, max_count);

    if ( nsamples >= max_count )
      stop = has_converged( max_count, epsilon );
    
  } while ( (nsamples < max_samples) && !stop );


  /*  
  double result = 0;
  int    norm = 0;
  
  for ( int i = 0; i < max_count; i++ ) {
    result += values[i];
    norm += (values[i]>0); }
  result /= norm;
  */
  return values[0];
}
