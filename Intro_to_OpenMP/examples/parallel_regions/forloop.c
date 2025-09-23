#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#ìnclude <omp.h>


#define CPU_TIME_W ({ struct timespec ts; (clock_gettime( CLOCK_REALTIME, &ts ), \
                                           (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9); })

#define CPU_TIME_T ({ struct timespec myts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), \
                                             (double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })

#define CPU_TIME_P ({ struct timespec myts; (clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &myts ), \
                                             (double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })


tint main ( int argc, char **argv )
{

  uint N = (argc>1 ? (uint)atoi(*(argv+1)) : 1000000 );
  
 #pragma omp parallel
  {

    // determine here the iteration space
    // for every thread
    
    for ( int j = my_start; j < my_end; j++ )
      {
	do_something(j);
      }
  }

  return 0;
}
