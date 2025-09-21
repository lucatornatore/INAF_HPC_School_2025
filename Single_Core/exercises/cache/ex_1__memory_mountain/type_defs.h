
#pragma once

// define the data type
typedef long data_t;


// Function to be tested takes two integer arguments 
typedef data_t (test_funct)(data_t *, int, int);
			   
typedef data_t (*test_funct_ptr)(data_t *, int, int); 
