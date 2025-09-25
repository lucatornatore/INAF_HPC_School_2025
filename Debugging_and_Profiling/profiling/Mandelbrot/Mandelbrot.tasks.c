/* ··········································································
 *
 * @file mandelbrot_omp_tasks.c
 * @brief Calculates the Mandelbrot set using a recursive, task-based OpenMP approach.
 *
 * This program demonstrates a task-based parallelization strategy.
 * The image is initially tiled in patches, dispatched to threads.
 * Every thread evaluate the Mandelbrot kernel on the border of the patch:
 * - If the border is entirely within the set, the whole patch is colored as such (value 0)
 * - If the border is entirely outside the set, the patch is colored with the
 * average iteration count of the border points.
 * - If the border is mixed, the patch is split into four sub-patches,
 * and new OpenMP tasks are generated to handle them.
 *
 * This approach minimizes computation in the large, uniform areas of the set.
 * The core calculation loop tries to be friendly to compiler auto-vectorization.
 *
 *
 * Recommended compilation for performance:
 * gcc -O3 -fopenmp -march=native -mtune -ftree-vectorize -o mandelbrot_tasks mandelbrot_omp_tasks.c -lm
 *
 * COMMAND-LINE arguments:
 *
 * ./mandelbrot_tasks x_size y_size max_iteration initial_patch_size
 *
 * The coed will save the Mandelbrot set in an image file named "mandelbrot.ppm".
 ·················································································
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <stdbool.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
// --- default values
//
#define XSIZE_DFLT 4096
#define YSIZE_DFLT 4096
#define MAX_ITER 1000
#define PATCH_SIDE_DEFAULT (XSIZE_DFLT / 32 ) // Default side length for the initial patches


// --- Mandelbrot Set Boundaries in the Complex Plane ---
// 
const double X_MIN = -2.0;
const double X_MAX = 1.0;
const double Y_MIN = -1.7;
const double Y_MAX = 1.3;

/**
 * @brief The core kernel. Determines if a point c = (cx, cy) is in the set.
 * @return The number of iterations to escape, or 0 if it's in the set.
 * This function is a prime candidate for SIMD/vectorization.
 */
static inline int mandelbrot_point(double cx, double cy)
{
  double zx = 0.0, zy = 0.0;
  double zx_sq = 0.0, zy_sq = 0.0;
  int iter = 0;

    while (iter < MAX_ITER && zx_sq + zy_sq <= 4.0)
      {
        zy = 2.0 * zx * zy + cy;
        zx = zx_sq - zy_sq + cx;
        zx_sq = zx * zx;
        zy_sq = zy * zy;
        iter++;
      }

    return (iter == MAX_ITER) ? 0 : iter;
}

// Forward declaration for the recursive function
void compute_patch(int* image, int x_start, int y_start, int xsize, int ysize, int size);

/**
 * @brief Main recursive function to compute a patch of the Mandelbrot set.
 */
void compute_patch(int* image, int x_start, int y_start, int xsize, int ysize, int size)
{
    bool all_in = true;
    bool all_out = true;
    long total_border_iter = 0;
    int border_points = 0;

    // 1. Check the border of the patch
    for (int i = 0; i < size; i++)
      {
	
        // Top and Bottom borders
        int p_top = mandelbrot_point(X_MIN + (x_start + i) * (X_MAX - X_MIN) / xsize,
				     Y_MIN + y_start * (Y_MAX - Y_MIN) / ysize);
	
        int p_bot = mandelbrot_point(X_MIN + (x_start + i) * (X_MAX - X_MIN) / xsize,
				     Y_MIN + (y_start + size - 1) * (Y_MAX - Y_MIN) / ysize);
	
	
        // Left and Right borders (excluding corners already checked)
        int p_left = (i > 0 && i < size - 1) ?
	  mandelbrot_point(X_MIN + x_start * (X_MAX - X_MIN) / xsize,
			   Y_MIN + (y_start + i) * (Y_MAX - Y_MIN) / ysize) : -1;
	
        int p_right = (i > 0 && i < size - 1) ?
	  mandelbrot_point(X_MIN + (x_start + size - 1) * (X_MAX - X_MIN) / xsize,
			   Y_MIN + (y_start + i) * (Y_MAX - Y_MIN) / ysize) : -1;

        int border_values[] = {p_top, p_bot, p_left, p_right};
	
        for(int j = 0; j < 4; ++j)
	  {
            if (border_values[j] != -1)
	      {
                if (border_values[j] == 0) all_out = false;
                else all_in = false;
                total_border_iter += border_values[j];
                border_points++;
	      }
	  }
      }

    // 2. evaluate the border
    if (all_in)
      {
        // Fill the entire patch with 0 (inside the set)
        for (int y = y_start; y < y_start + size; y++)
	  {
            for (int x = x_start; x < x_start + size; x++)
	      image[y * xsize + x] = 0;
	  }
      } else if (all_out)
      {
        // Fill the entire patch with the average border iteration
        int avg_iter = border_points > 0 ? (int)(total_border_iter / border_points) : 1;
        if (avg_iter == 0) avg_iter = 1; // Ensure it's not colored as "in the set"
        for (int y = y_start; y < y_start + size; y++)
	  {
            for (int x = x_start; x < x_start + size; x++)
                image[y * xsize + x] = avg_iter;
	  }
      } else
      {
        // 3. Mixed border: subdivide or compute directly
        if (size <= 8)
	  { //If patch is small, compute all points
            for (int y = y_start; y < y_start + size; y++)
	      for (int x = x_start; x < x_start + size; x++)
		image[y * xsize + x] = mandelbrot_point(X_MIN + x * (X_MAX - X_MIN) / xsize,
							Y_MIN + y * (Y_MAX - Y_MIN) / ysize );
	  }
	else
	  {
            // Recursive step: split into 4 sub-patches and create tasks
	    int new_size = size / 2;
	   #pragma omp task
            compute_patch(image, x_start, y_start, xsize, ysize, new_size);
	   #pragma omp task
            compute_patch(image, x_start + new_size, y_start, xsize, ysize, new_size);
	   #pragma omp task
            compute_patch(image, x_start, y_start + new_size, xsize, ysize, new_size);
	   #pragma omp task
            compute_patch(image, x_start + new_size, y_start + new_size, xsize, ysize, new_size);
	  }
      }

    return;
}

/**
 * @brief Saves the image data to a PNG file using stb_image_write.
 * @param image Pointer to the raw iteration data.
 * @param filename The name of the output file.
 */
void save_to_png(int* image, int xsize, int ysize, const char* filename)
{
    // PNG needs a buffer of unsigned char in RGB format (3 bytes per pixel).
  int num_pixels = xsize * ysize;
  unsigned char* pixel_data = (unsigned char*)malloc(num_pixels * 3 * sizeof(unsigned char));
  if (!pixel_data) {
    perror("Failed to allocate pixel buffer for PNG");
    return;
  }
  
  // Convert iteration counts to colors, mapping each pixel to 3 bytes (R, G, B)
 #pragma omp parallel for
  for (int i = 0; i < num_pixels; i++)
    {
      int iter = image[i];
      if (iter == 0)
	{
	  pixel_data[i * 3 + 0] = 0;   // R
	  pixel_data[i * 3 + 1] = 0;   // G
	  pixel_data[i * 3 + 2] = 0;   // B (Black)
	}
      else
	{
	  // Simple coloring scheme
	  pixel_data[i * 3 + 0] = (iter % 256);         // R
	  pixel_data[i * 3 + 1] = (iter * 2 % 256);     // G
	  pixel_data[i * 3 + 2] = (iter * 5 % 256);     // B
	}
    }
  
  // Write the PNG file.
  // stbi_write_png arguments: filename, width, height, channels, data, stride.
  // Stride is the number of bytes per row (width * channels).
  if (!stbi_write_png(filename, xsize, ysize, 3, pixel_data, XSIZE_DFLT * 3)) {
    fprintf(stderr, "ERROR: could not write PNG file %s\n", filename);
  }
  
  free(pixel_data);
}

/**
 * @brief Saves the image data to a PPM file.
 */
void save_to_ppm(int* image, int xsize, int ysize, const char* filename)
{
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Failed to open file");
        return; }

    fprintf(f, "P3\n%d %d\n255\n", xsize, ysize);
    for (int i = 0; i < xsize * ysize; i++) {
        int iter = image[i];
        if (iter == 0) {
            fprintf(f, "0 0 0\n"); // Black for points in the set
        } else {
            // Simple coloring scheme
            int r = (iter % 256);
            int g = (iter * 2 % 256);
            int b = (iter * 5 % 256);
            fprintf(f, "%d %d %d\n", r, g, b);
        }
    }
    fclose(f);
}

int main ( int argc, char **argv)

{
  
  unsigned int img_size[2] = { XSIZE_DFLT, YSIZE_DFLT };
  unsigned int max_iter = MAX_ITER;
  int init_patch = PATCH_SIDE_DEFAULT;
  
  if ( argc > 2 )
    {
      img_size[0] = (unsigned int)atoi(*(argv+1));
      img_size[1] = (unsigned int)atoi(*(argv+2));
      if ( argc > 3 )
	max_iter = (unsigned int)atoi(*(argv+3));

      if ( argc > 4 )
	init_patch = (int)atoi(*(argv+4));
      
    }

  
    unsigned int* image = (unsigned int*)malloc( img_size[0]*img_size[1] * sizeof(int));
    if (!image) {
        perror("Failed to allocate image memory");
        return 1; }

    printf("Calculating Mandelbrot set (%dx%d) with max %d iterations...\n", img_size[0], img_size[1], max_iter );
    printf("Using patch size: %d\n", init_patch );

    double start_time = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            printf("Running with %d OpenMP threads.\n", omp_get_num_threads());
	    
            for (int y = 0; y < img_size[1]; y += init_patch) {
                for (int x = 0; x < img_size[0]; x += init_patch) {
                    #pragma omp task
                    compute_patch(image, x, y, img_size[0], img_size[1], init_patch);
                }
            }
        }
    } // Implicit barrier here ensures all tasks are complete

    double end_time = omp_get_wtime();
    printf("Calculation finished in %.4f seconds.\n", end_time - start_time);

    printf("Saving image to mandelbrot.ppm...\n");
    //save_to_ppm(image, img_size[0], img_size[1], "mandelbrot.ppm");
    save_to_png(image, img_size[0], img_size[1], "mandelbrot.png");
    printf("Done.\n");

    free(image);
    return 0;
}
