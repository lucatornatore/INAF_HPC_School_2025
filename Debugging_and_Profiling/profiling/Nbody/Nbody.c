/**
 * @file nbody_simulation.c
 * @brief An N-body gravitational simulation for demonstrating HPC profiling.
 *
 * This program simulates the interaction of N bodies under gravity.
 * It's designed to highlight performance differences based on data layout.
 *
 * Two data layouts are implemented, switchable at compile time:
 * 1. Array of Structs (AoS): The default, intuitive, but cache-unfriendly layout.
 * 2. Struct of Arrays (SoA): A more performant, cache-friendly layout.
 *
 * --- How to Compile & Run ---
 *
 * For the slow, cache-unfriendly version (AoS):
 * gcc -g -O3 -o nbody_aos nbody_simulation.c -lm
 *
 * For the fast, cache-friendly version (SoA):
 * gcc -g -O3 -DUSE_SOA -o nbody_soa nbody_simulation.c -lm
 *
 * The -g flag includes debug symbols for profiling.
 * The -O3 flag is important to enable vectorization and other optimizations,
 * which makes the performance gap between AoS and SoA even more significant.
 * The -lm flag links the math library for sqrt().
 */


#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <math.h>

#define CPU_TIME ({struct  timespec ts; clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ), \
                                          (double)ts.tv_sec +           \
                                          (double)ts.tv_nsec * 1e-9;})

#define NP_dflt 2048
#define NSTEPS_dflt 100
#define G 6.67430e-11 // Gravitational constant
#define DT 0.01       // Time step

// Epsilon to avoid division by zero when bodies are at the same position.
const double epsilon_sq = 1e-9;

#ifdef USE_SOA
// --- Struct of Arrays (SoA) Layout ---
// Cache-friendly for this problem
typedef struct {
    double* x, * y, * z;
    double* vx, * vy, * vz;
    double* mass;
    double* fx, * fy, * fz; // Forces
} particle_t;

#else
// --- Array of Structs (AoS) Layout ---
// Cache-unfriendly for this problem
typedef struct {
    double x, y, z;
    double vx, vy, vz;
    double mass;
    double fx, fy, fz; // Forces
} particle_t;
#endif


#ifdef USE_SOA
void initialize_particles ( particle_t *, const size_t N );
#else
void initialize_particles ( particle_t **, const size_t N );
#endif
void compute_forces       ( particle_t * restrict, const size_t );
void update_particles     ( particle_t *, size_t, double );





/**
 * @brief Initializes the bodies with random positions and masses.
 */

#ifdef USE_SOA
void initialize_particles ( particle_t * P, const size_t N )
#else
void initialize_particles ( particle_t ** _P, const size_t N )
#endif
{
 #ifdef USE_SOA
  P->x = (double*)malloc(N * 10 * sizeof(double));
  memset ( P->x, 0, N * 10 * sizeof(double) );

  P->y    = P->x + N;
  P->z    = P->x + 2*N;
  P->vx   = P->x + 3*N;
  P->vy   = P->x + 4*N;
  P->vz   = P->x + 5*N;
  P->mass = P->x + 6*N;
  P->fx   = P->x + 7*N;
  P->fy   = P->x + 8*N;
  P->fz   = P->x + 9*N;
  
 #else

  particle_t *P = (particle_t*)malloc( N * sizeof(particle_t) );
  *_P = P;  
  memset ( P, 0, N * sizeof(particle_t) );
  
 #endif
  
  // Use a fixed seed for reproducibility
  
  for ( size_t i = 0; i < N; i++)
    {
      double a = (double)drand48();
      double b = (double)drand48();
      double c = (double)drand48();
      double d = (double)drand48();

     #ifdef USE_SOA
      
      P->x[i] = a;
      P->y[i] = b;
      P->z[i] = c;
      P->mass[i] = d * 1e12 + 1e11;
      
     #else

      P[i].x = a;
      P[i].y = b;
      P[i].z = c;
      P[i].mass = d * 1e12 + 1e11;
      
     #endif

    }
}


/**
 * @brief The core computational kernel. Calculates the gravitational forces.
 * This is where the O(N^2) complexity lies and where the performance
 * difference between AoS and SoA is most apparent.
 */

void compute_forces ( particle_t * restrict P, const size_t N )
{
  
 #ifdef USE_SOA
  for ( size_t i = 0; i < N; i++)
    P->fx[i] = P->fy[i] = P->fz[i] = 0.0;
 #else
  for ( size_t i = 0; i < N; i++)
    P[i].fx = P[i].fy = P[i].fz = 0.0;
 #endif

  
  for ( size_t i = 0; i < N; i++ )
    {            
     #ifdef USE_SOA
      const double x = P->x[i];
      const double y = P->y[i];
      const double z = P->z[i];
      const double mG = P->mass[i]*G;
     #else
      const double x = P[i].x;
      const double y = P[i].y;
      const double z = P[i].z;
      const double mG = P[i].mass*G;
     #endif

      double fx = 0;
      double fy = 0;
      double fz = 0;
      
      
      for (size_t j = i + 1; j < N; j++)
	{
	 #ifdef USE_SOA
	  double dx = P->x[j] - x;
	  double dy = P->y[j] - y;
	  double dz = P->z[j] - z;
	 #else
	  double dx = P[j].x - x;
	  double dy = P[j].y - y;
	  double dz = P[j].z - z;
	 #endif
	  
	  double dist_sq = dx * dx + dy * dy + dz * dz + epsilon_sq;
	  double inv_dist = 1.0 / sqrt(dist_sq);
	  double inv_dist_cubed = inv_dist * inv_dist * inv_dist;

	 #ifdef USE_SOA
	  double force_mag = mG * P->mass[j] * inv_dist_cubed;
	 #else
	  double force_mag = mG * P[j].mass * inv_dist_cubed;
	 #endif
	  
	  double _fx = force_mag * dx;
	  double _fy = force_mag * dy;
	  double _fz = force_mag * dz;
	  
	  fx += _fx;
	  fy += _fy;
	  fz += _fz;
	  
	 #ifdef USE_SOA
	  P->fx[j] -= _fx;
	  P->fy[j] -= _fy;
	  P->fz[j] -= _fz;
	 #else
	  P[j].fx -= _fx;
	  P[j].fy -= _fy;
	  P[j].fz -= _fz;
	 #endif
	}
    }
}
  
/**
 * @brief Updates the velocities and positions of all bodies based on the
 * computed forces using a simple Euler integration step.
 */

void update_particles ( particle_t* P, size_t N, double dt )
  {
    for (int i = 0; i < N; i++)
      {
	
       #ifdef USE_SOA
	
	double mass_r = 1.0 / P->mass[i];
	
	// Update velocity
	P->vx[i] += P->fx[i] / mass_r * dt;
	P->vy[i] += P->fy[i] / mass_r * dt;
	P->vz[i] += P->fz[i] / mass_r * dt;
	
	// Update position
	P->x[i] += P->vx[i] * dt;
	P->y[i] += P->vy[i] * dt;
	P->z[i] += P->vz[i] * dt;

	#else

	double mass_r = 1.0 / P[i].mass;
	
        // Update velocity
        P[i].vx += P[i].fx / mass_r * dt;
        P[i].vy += P[i].fy / mass_r * dt;
        P[i].vz += P[i].fz / mass_r * dt;

        // Update position
        P[i].x += P[i].vx * dt;
        P[i].y += P[i].vy * dt;
        P[i].z += P[i].vz * dt;
	
       #endif
      }

    return;
}


int main ( int argc, char **argv )

{
  double dt = 0.1;
  size_t N      = (argc > 1 ? atoll(*(argv+1)) : NP_dflt );
  size_t Nsteps = (argc > 2 ? atoll(*(argv+2)) : NSTEPS_dflt );
  long int seed = (argc > 3 ? atol(*(argv+3)) : 0 );
  if ( seed == 0 )
    seed = time(NULL);

  srand ( seed );
  
  printf ( " »»» N-Body toy simulator\n"
	   " using %s\n"
	   " \t %llu particles\n",
	  #ifdef USE_SOA
	   "Structures of Arrays",
	  #else
	   "Arrays of structures",
	  #endif
	   (unsigned long long)N );

  double timing_init = CPU_TIME;

 #ifdef USE_SOA
  particle_t P;  
 #else  
  particle_t *P = NULL;
 #endif
  
  initialize_particles( &P, N );
  
  timing_init = CPU_TIME - timing_init;
  

  printf("Starting simulation for %llu bodies over %lu timesteps...\n",
	 (unsigned long long)N, Nsteps );

  double timing_evolution = CPU_TIME;
  
  for (int step = 0; step < Nsteps; step++ )
    {
     #ifdef USE_SOA
      compute_forces( &P, N );
      update_particles( &P, N, dt );
     #else
      compute_forces( P, N );
      update_particles( P, N, dt );
     #endif
    }

  timing_evolution = CPU_TIME - timing_evolution;
  
  printf("Simulation finished.\n");
  printf("Total execution time: %g s (init), %g s (evolution)\n",
	 timing_init, timing_evolution );

    // Print a checksum to prevent dead code elimination and verify correctness
 #ifdef USE_SOA
  printf("Checksum (Position of particle 0): (%f, %f, %f)\n", P.x[0], P.y[0], P.z[0]);
  free ( P.x );
 #else
  printf("Checksum (Position of particle 0): (%f, %f, %f)\n", P[0].x, P[0].y, P[0].z);
  free ( P );
 #endif


    return 0;
}
