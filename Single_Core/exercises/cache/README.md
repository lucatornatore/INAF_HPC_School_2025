# Code Optimization : be cache-friendly

## **ex_0__array_traversal**

Testing the impact in terms of run-time of traversing a matrix $A[i][j]$ 

in row-major order, i.e. as

```c
for ( int i = 0; i < Nrows; i++ )
    for ( int j = 0; j < Ncols; j++ )
        // do something on A[i][j]
```

and in column-major order, i.e. as

```c
for ( int j = 0; j < Ncols; j++ )
	for ( int i = 0; i < Nrows; i++ )
        // do something on A[i][j]
```

***Note**: the column-major order would be cache-friendly in FORTRAN, since that language follows tha opposite convention than C/C++*



## ex_1__memory_mountain

We measure the run-time of traversing a 1D array varying the stride and the size of the array.
In this way, we’ll the impact of having an array that fits entirely in the L1 cache, traversed contiguosly, up to the very opposite of having an array that is much larger than the L3 traversed with a stride that leads you out of the cache line at every step.



## ex_2__matrix_transpose

We measure the impact of transposing a matrix

$A^t [j][i] = A[i][j]$

in 3 different ways.

1) “schoolbook”, reading contiguous locations and writing strided locations

   ```C
   for ( int i = 0; i < Nrows; i++ )
       for ( int j = 0; j < Ncols; j++ )
           At[j][i] = A[i][j]
   ```

2) swapping the pattern: strided reading and contiguous writing

   ```C
   for ( int i = 0; i < Nrows; i++ )
       for ( int j = 0; j < Ncols; j++ )
           At[i][j] = A[j][i]
   ```

3) transposing by tiles, which enhances cache locality



## ex_3__hot_and_cold_fields

This example is about the importance of data locality and size: 

1. keep close in memory data that are likely to be used close in time
2. keep their size as small as possible, possibly at the level of the cache-line size



## ex_4__AoS_vs_SoA

Which is the best pattern of data?
having then packed in structures and the structures collected in arrays?
or arrays of single data fields?
it depends..