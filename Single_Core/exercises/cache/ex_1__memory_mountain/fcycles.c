
// [   ] ------------------------------------------------
// set the ...
// _XOPEN_SOURCE=700  implies  _POSIX_C_SOURCE=200809L


#define _XOPEN_SOURCE 700

#if !defined(_GNU_SOURCE) && defined(__linux__)
#define _GNU_SOURCE
#endif

// [···] ------------------------------------------------


#include <unistd.h>
#include <stdint.h>

#include <sched.h>
#include <x86intrin.h> 
#include <cpuid.h>

#include "fcycles.h"


static int    timing_type = TIMING_CLOCK;
static double _frequency_ = 1.0e9;


void set_timing_type ( int type )
{
  timing_type = type % (TIMING_CYCLES+1);
  return;
} 

int get_timing_type ( void )
{
  return timing_type;
} 



inline int get_core ( unsigned int * node )
{
 #if defined (__linux__)
  if ( node != NULL )
    {
      unsigned int cpu;
      getcpu( &cpu, node );
      return cpu;
    }
  else
    return sched_getcpu();
 #else

  return -1;
 #endif
}


inline int pin_to_core( unsigned int coreid )
{
 #if defined(__linux__)
  cpu_set_t cpuset;
  CPU_ZERO ( &cpuset );
  CPU_SET  ( coreid, &cpuset );

  // remind: this call fails if the affinity mask in the kernel is
  // larger than 1024 bytes
  // In the most general case, we should discover which is the required
  // mask size by iteratively calling sched_getaffinity with with
  // increasing mask sizes, until it fails (see man page)
  //
  return sched_setaffinity ( 0, sizeof(cpu_set_t), &cpuset );
 #else
  return -1;
 #endif
}


inline int has_invariant_tsc ( void )
// this function return 1 if the invariant cycles counter is present
// and 0 otherwise (very old cpus, indeed)
//
{
 #if defined(__linux__)
  unsigned max = __get_cpuid_max(0x80000000, NULL);
  if (max >= 0x80000007)
    {
      unsigned a,b,c,d;
      __cpuid(0x80000007, a,b,c,d);
      return (d & (1u << 8)) != 0; // Invariant TSC
    }
 #else
  #warning "Note:: get_cpuid defined only for linux systems. Alternative not provided yet."
 #endif
  return 0;

}

#if !defined(__linux__) && defined(TSC_V1)
#warning "TSC reader v1 is defined only on linux systems. Fallback to v2."
#undef TSC_V1
#define TSC_V2
#endif

#if !defined(TSC_V1) && !defined(TSC_V2)
#define TSC_V2
#endif

#if defined (TSC_V1)

inline uint64_t tsc_start ( void )
{
  unsigned a, b, c, d;
  __get_cpuid(0, &a, &b, &c, &d);   // serialize earlier instructions
  return __rdtsc();                 // read TSC (low-overhead)
}

inline uint64_t tsc_end ( unsigned int *coreid )
{
  unsigned coreid_local;
  uint64_t t = __rdtscp(&coreid_local);      // read TSC and get TSC_AUX (CPU id)
  unsigned a, b, c, d;
  __get_cpuid(0, &a, &b, &c, &d);   // serialize later instructions
  if ( coreid != NULL )
    *coreid = aux;
  return t;
}


#elif defined( TSC_V2 )

inline uint64_t tsc_start ( void )
{
  _mm_lfence(); 
  uint64_t start_cycles = __rdtsc();
  return start_cycles;
}

inline uint64_t tsc_end ( unsigned int *coreid )
{
  unsigned int aux;
  
  uint64_t end_cycles = __rdtscp(&aux);
  //_mm_lfence();
  
  if ( coreid != NULL )
    *coreid = aux;
  
  return end_cycles;
}

#endif


#if defined( USE_FREQ_FROM_CPU ) && !defined( _GNU_SOURCE )
#undef USE_FREQ_FROM_CPU
#endif

#if defined( _GNU_SOURCE ) && defined( USE_FREQ_FROM_CPU )

static double tsc_hz_cached = 0.0;

double get_tsc_freq ( void )
{
  // check whether we have already found the frequency
  // in case, return it
  
  if (tsc_hz_cached > 0) return tsc_hz_cached;

  unsigned eax, ebx, ecx, edx;
  unsigned max_basic = __get_cpuid_max(0, NULL);

  // Try CPUID 0x15
  if (max_basic >= 0x15)
    {
      __cpuid_count(0x15, 0, eax, ebx, ecx, edx);
      if (eax != 0 && ebx != 0)
	{
	  // If ECX=0, the the "crystal freq" isn’t avaialble;
	  // the code will fall-back to the leaf 0x16 below
	  if (ecx != 0) {
	    tsc_hz_cached = (double)ecx * (double)ebx / (double)eax;
	    return tsc_hz_cached;
	  }
	}
    }

  // get the CPUID 0x16 base MHz
  if (max_basic >= 0x16)
    {
      __cpuid(0x16, eax, ebx, ecx, edx);
      if (eax != 0) {
	tsc_hz_cached = (double)eax * 1e6;  // base MHz -> Hz
	return tsc_hz_cached;
      }
    }

  // last resort:
  // calibrate with CLOCK_MONOTONIC_RAW
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
  
  uint64_t s = tsc_start();
  struct timespec wait_time = {0, 100 * 1000 * 1000}; // 100 ms
  nanosleep(&wait_time, NULL);
  uint64_t e = tsc_end();
  
  clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
  
  double deltat = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
  tsc_hz_cached = (double)(e - s) / deltat;
  
  return tsc_hz_cached;
}



#else


//
// ===========================================================================
//


double get_tsc_freq( void )
{
    struct timespec start_time, end_time;
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    uint64_t start_cycles = __rdtsc(); // rdtsc is fine here, we just need a delta
    
    // Sleep for a known duration, e.g., 100ms
    struct timespec wait_time = {0, 100 * 1000 * 1000}; // 100 ms
    nanosleep(&wait_time, NULL);

    unsigned int aux;
    uint64_t end_cycles = __rdtscp(&aux);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

    uint64_t cycles_elapsed = end_cycles - start_cycles;
    uint64_t ns_elapsed = (end_time.tv_sec - start_time.tv_sec) * 1e9 + (end_time.tv_nsec - start_time.tv_nsec);
    
    // Frequency (cycles/sec) = total cycles / total seconds
    _frequency_ = (double)cycles_elapsed / ((double)ns_elapsed / 1e9);
    return _frequency_;
}


#endif



double calibrate_cpu_frequency( int coreid, int mode )
{

  // [ 1 ] pin to core if on linux system
  
 #if defined(__linux__)
  if ( coreid >= 0 )
    pin_to_core( coreid );
 #else
  #warning "Note:: not on a linux system: remember to manually pin the execution to a core"
 #endif

  if ( !mode ) {
    unsigned long long acc = 0;
    for ( int i = 0; i < 10000000; i++ )
      acc += i; }
  _frequency_ = get_tsc_freq( );
  return _frequency_;
  
}


mytimer_t get_time_start( void )
{
  mytimer_t timer;
  switch ( timing_type )
    {
    case TIMING_CLOCK: timer.seconds = CPU_TIME; break;
    case TIMING_CYCLES: timer.cycles = tsc_start(); break;
    default: timer.cycles = tsc_start(); break;
    }

  return timer;
}


mytimer_t get_time_end( unsigned int *core )
{
  mytimer_t timer;
  switch ( timing_type )
    {
    case TIMING_CLOCK: timer.seconds = CPU_TIME; break;
    case TIMING_CYCLES: timer.cycles = tsc_end( core ); break;
    default: timer.cycles = tsc_end( core ); break;
    }

  return timer;
}


double get_timing ( mytimer_t BEGIN, mytimer_t END, double freq )
{
  double result = 0;
    switch ( timing_type )
    {
    case TIMING_CLOCK: result = END.seconds - BEGIN.seconds; break;
    case TIMING_CYCLES: {
      double myfreq = ( freq == 0? _frequency_ : freq );
      result = (double)(END.cycles - BEGIN.cycles) / myfreq;
    } break;
    }
    return result;
}


void do_not_optimize_away(void* p) {
  asm volatile("" : : "r,m"(p) : "memory"); }


double get_timing_overhead ( )
{

  if ( get_timing_type() == TIMING_CYCLES )
    calibrate_cpu_frequency( -1, 1 );

  volatile mytimer_t timer_a;
  volatile mytimer_t timer_b;

  double volatile acc = 1.0;
  for ( int i = 0; i < 100000; i++ )
    acc +=acc;
  
  for ( int i = 0; i < 2; i++ )
    {
      timer_a = get_time_start ( );
      timer_b = get_time_end ( NULL );
    }

  double overhead = get_timing ( timer_a, timer_b, _frequency_ );

  return overhead;
}
