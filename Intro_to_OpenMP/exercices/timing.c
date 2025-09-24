#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>


#define CPU_TIME_W ({ struct timespec ts; (clock_gettime( CLOCK_REALTIME, &ts ), \
                                           (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9); })
#define CPU_TIME_T ({ struct timespec myts; (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), \
                                             (double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })
#define CPU_TIME_P ({ struct timespec myts; (clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &myts ), \
                                             (double)myts.tv_sec + (double)myts.tv_nsec * 1e-9); })


int main ( int argc, char **argv )
{

  uint N       = (argc>1 ? (unsigned long long int)atoll(*(argv+1)) : 1000000 );
  int  nthreads;

  double process_timing       = CPU_TIME_P;
  double wallclock_timing     = CPU_TIME_W;
  double wallclock_timing_omp = omp_get_wtime();
  
  
  #pragma omp parallel
  {
    
    int myid = omp_get_thread_num();
   #pragma omp masked
    nthreads = omp_get_num_threads();
    
    uint limit = N / nthreads;
    unsigned long long int sum = 0;
    
    double mytiming = CPU_TIME_T;
    
    for ( uint i = 0; i < limit ; i++ )
	sum += i;

    mytiming = CPU_TIME_T - mytiming;

    printf("thread %03d has run for %9.8g sec\n", myid, mytiming );
    
  }

  wallclock_timing_omp = omp_get_wtime();
  wallclock_timing = CPU_TIME_W - wallclock_timing;
  process_timing = CPU_TIME_P - process_timing;
    
  printf("Wall-clock elapsed time is %g sec\n"
	 "Wall-clock elapsed time measured via wtime is %g sec\n"
	 "Process time is %g sec\n",
	 wallclock_timing,
	 wallclock_timing_omp,
	 process_timing);
  

  return 0;
}
