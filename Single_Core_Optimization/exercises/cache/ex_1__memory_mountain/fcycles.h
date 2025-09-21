
#pragma once

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
#include <time.h>

#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ),	\
                                          (double)ts.tv_sec +           \
                                          (double)ts.tv_nsec * 1e-9;})


#define TIMING_CLOCK  0
#define TIMING_CYCLES 1


typedef struct { union {
  unsigned long long int cycles;
  double seconds; }; } mytimer_t;


extern void set_timing_type     ( int );
extern int  get_timing_type     ( void );

extern int  get_core            ( unsigned int * );
extern int  pin_to_core         ( unsigned int );
extern int  has_invariant_tsc   ( void );

extern uint64_t  tsc_start           ( void );
extern uint64_t  tsc_end             ( unsigned int * );

double  calibrate_cpu_frequency ( int, int );
double  get_timing_overhead     ( void );
double  get_timing              ( mytimer_t, mytimer_t, double );

mytimer_t get_time_start ( void );
mytimer_t get_time_end   ( unsigned int * );
