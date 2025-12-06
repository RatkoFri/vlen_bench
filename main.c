#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <math.h>
#include <stdio.h>


// Usage:
// Pass -DUSE_DOUBLE to the compiler to use double precision.
// Otherwise, single precision (float) will be used.

#ifdef USE_DOUBLE
typedef double fp_t;
#define FP_TYPE "double"
#define FP_EPSILON 1e-12
#define FP_FMT "%.12lf"
#define FP_SCANF_FMT "%lf"
#define fp_sqrt sqrt
#define fp_abs fabs
#define fp_pow pow
#define fp_exp exp
#define fp_log log
#define fp_sin sin
#define fp_cos cos
#define fp_tan tan
#define index_t uint64_t
#define INDEX_FMT "%zu"
#define shift 3 
#define vtype "e64"
#ifdef USE_RVV0_7
#define vload "vle.v"
#define vstore "vse.v"
#define vlind "vlxe.v"
#define tail_mask " "
#else
#define vload "vle64.v"
#define vstore "vse64.v"
#define vlind "vluxei64.v"
#define tail_mask ", ta, ma"
#endif
#else
typedef float fp_t;
#define FP_TYPE "float"
#define FP_EPSILON 1e-6f
#define FP_FMT "%.6f"
#define FP_SCANF_FMT "%f"
#define fp_sqrt sqrtf
#define fp_abs fabsf
#define fp_pow powf
#define fp_exp expf
#define fp_log logf
#define fp_sin sinf
#define fp_cos cosf
#define fp_tan tanf
#define index_t uint32_t
#define INDEX_FMT "%u"
#define vtype "e32"
#define shift 2 
#ifdef USE_RVV0_7
#define vload "vle.v"
#define vstore "vse.v"
#define vlind "vlxe.v"
#define tail_mask " "
#else
#define vload "vle32.v"
#define vstore "vse32.v"
#define vlind "vluxei32.v"
#define tail_mask ", ta, ma"

#endif

#endif

// Macro for ceil function that returns integer
#define CEIL_INT(x) ((int)((x) == (int)(x) ? (x) : (int)(x) + 1))

#define NUM_ITERATIONS 10

// Scalar array addition
void scalar_add(fp_t *a, fp_t *b, fp_t *result, int size) {
    for (int i = 0; i < size; i++) {
        result[i] = a[i] + b[i];
    }
}

// RVV vector array addition using inline assembly
void rvv_add(fp_t *a, fp_t *b, fp_t *result, int size) {
    // RVV assembly implementation
    // This uses RISC-V Vector extension instructions
    int avlen, req_vlen, max_vlen; // Requested vector length in bits 
    req_vlen = -1;
    asm volatile("vsetvli %0, %1, "vtype", m1 "tail_mask" " : "=r"(max_vlen) : "r"(req_vlen));
    req_vlen = size; // size in bits
    int lmul = CEIL_INT((float)req_vlen / (float)max_vlen);
    if (lmul <= 1)
    {   

        asm volatile("vsetvli %0, %1, "vtype" , m1 "tail_mask" " : "=r"(avlen) : "r"(req_vlen));
    }
    else if (lmul <= 2)
    {
        asm volatile("vsetvli %0, %1, "vtype" , m2 "tail_mask" " : "=r"(avlen) : "r"(req_vlen));
    }
    else if (lmul <= 4)
    {
        asm volatile("vsetvli %0, %1, "vtype" , m4 "tail_mask" " : "=r"(avlen) : "r"(req_vlen));
    }
    else
    {
        printf("Unsupported LMUL value: %d\n", lmul);
        return; // return the output vector without any changes
    }
    for (int i = 0; i<100; i++){ // Dummy loop to avoid compiler optimizing out the asm code
        asm volatile(" "vload" v8, (%0)" : : "r"(a));
        asm volatile(" "vload" v16, (%0)" : : "r"(b));
        asm volatile("vfadd.vv v8, v8, v16");
        asm volatile(" "vstore" v8, (%0)" : : "r"(result));  
    }
}

// RISC-V cycle counter not working
unsigned long read_cycles(void)
{
    unsigned long cycles;
    cycles = clock();
    return cycles;
}

double timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0 ;
}

double calculate_median(double *times, int n) {
    double *sorted = (double *)malloc(n * sizeof(double));
    memcpy(sorted, times, n * sizeof(double));
    // Simple bubble sort
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (sorted[j] > sorted[j+1]) {
                double temp = sorted[j];
                sorted[j] = sorted[j+1];
                sorted[j+1] = temp;
            }
        }
    }
    double median;
    if (n % 2 == 0) {
        median = (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    } else {
        median = sorted[n/2];
    }
    free(sorted);
    return median;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <array_size>\n", argv[0]);
        return 1;
    }

    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Array size must be positive\n");
        return 1;
    }

    // Allocate arrays
    fp_t *a = (fp_t *)malloc(size * sizeof(fp_t));
    fp_t *b = (fp_t *)malloc(size * sizeof(fp_t));
    fp_t *result_scalar = (fp_t *)malloc(size * sizeof(fp_t));
    fp_t *result_rvv = (fp_t *)malloc(size * sizeof(fp_t));

    if (!a || !b || !result_scalar || !result_rvv) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Initialize arrays with sample data
    for (int i = 0; i < size; i++) {
        a[i] = (fp_t)((i+1) * 1.5f);
        b[i] = (fp_t)((i+1) * 2.3f);
        result_scalar[i] = (fp_t)0;
        result_rvv[i] = (fp_t)0;
    }


    double scalar_times[NUM_ITERATIONS];
    double total_scalar_time = 0.0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        double start = timestamp();
        scalar_add(a, b, result_scalar, size);
        double end = timestamp();
        
        scalar_times[i] = (end - start); // Convert to milliseconds
        total_scalar_time += scalar_times[i];
    }

    // double mean_scalar_time = total_scalar_time / NUM_ITERATIONS;


    double rvv_times[NUM_ITERATIONS];
    double total_rvv_time = 0.0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        double start = timestamp();
        rvv_add(a, b, result_rvv, size);
        double end = timestamp();

        rvv_times[i] = (end - start); // Convert to milliseconds
        //printf("Iteration %d: %.6f ms\n", i + 1, rvv_times[i]);
    }

    //double mean_rvv_time = total_rvv_time / NUM_ITERATIONS;

    // print times 
    /*
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        printf("Iteration %d: Scalar Time: %.6f ms, RVV Time: %.6f ms, array size: %d\n", 
               i + 1, scalar_times[i], rvv_times[i], size);
    }
    */
    double median_rvv_time = calculate_median(rvv_times, NUM_ITERATIONS);
    printf("Median RVV Time: %.6f, array size: %d\n", median_rvv_time, size);
    // Verify results match (optional)
    int mismatch = 0;
    for (int i = 0; i < size; i++) {
        if (result_scalar[i] != result_rvv[i]) {
            mismatch++;
        }
    }
    
    if (mismatch > 0) {
        printf("\nWarning: Results differ in %d elements!\n", mismatch);
        printf("\nSize: %d\n", size);
    } else {
        
    }

    // Cleanup
    free(a);
    free(b);
    free(result_scalar);
    free(result_rvv);

    return 0;
}
