# Code Optimization: how to write decently efficient loops



Writing loops efficiently is a big chapter in HPC, since most of the time in scientific computing is normally spent in loop traversing big data fields.
Here we give some basic elements, that unify the cache-friendliness, avoiding the perils of branches *and* writing code that the compiler can translate so to exploit the super-scalarity of your cores (i.e. the fact that they have Instruction-Level Parallelism) 



## ex_1__matrix_multiplication

A great classic, which we can not miss.
We’ll explore three ways:

1. schoolbook
2. swapping the two inner loops, which give surprising results
3. by tiles



## ex_2__array_reduction

More efficient way to perform a reduction operation

```C
double result = initializer;
for ( int i = 0; i < N; i++ )
	result OP= array[i];   // OP may be +,-,*,/,^,...
```



## ex_3__multiply_arrays

What if you want to reduce the element-wise multiply two arrays $A[i]$ and $B[i]$ ?
Is

```c
for ( int i = 0; i < N; i++ )
    result += A[i]*B[i];
```

The only possible code?
(very similar to ex_2, though)