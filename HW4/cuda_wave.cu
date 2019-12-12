/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265

void check_param(void);
void printfinal (void);

int nsteps,                 	/* number of time steps */
    tpoints, 	     		/* total points along string */
    rcode;                  	/* generic return code */
float  values[MAXPOINTS+2]; 	/* values at time t */


/**********************************************************************
 *	Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n", 
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);

}



/**********************************************************************
 *     Update all values along line a specified number of times
 *********************************************************************/
__global__ void kernel_update(float *val, int tpoints, int nsteps, int start_point)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x + 1 + start_point;
    __shared__ float x, fac, d_oldval, d_newval, d_tmpval;
   	__shared__ float dtime, c, dx, tau, sqtau;

    //--------------------------------------------------------------
    // init line (moved into update())
    //--------------------------------------------------------------
    fac = 2.0 * PI;
	x = (idx - 1.0) / (float)(tpoints - 1);
	d_tmpval = sin(fac * x);	
	d_oldval = d_tmpval;

    //--------------------------------------------------------------
    // math paramerters setup (moved into update())
    //--------------------------------------------------------------
   	dtime = 0.3;
   	c = 1.0;
   	dx = 1.0;
   	tau = (c * dtime / dx);
   	sqtau = tau * tau;

    if ((idx == 1) || (idx == tpoints)){
       	d_newval = 0.0;
    }
    /* Update values for each time step */
    for (int i = 1; i<= nsteps; i++) {
       	d_newval = (2.0 * d_tmpval) - d_oldval + (sqtau * (-2.0) * d_tmpval);
        d_oldval = d_tmpval;
        d_tmpval = d_newval;
    }
	if (start_point != 0)
    	val[idx - start_point - 1] = d_tmpval;
	else
    	val[idx] = d_tmpval;
}

/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 1; i <= tpoints; i++) {
      printf("%6.4f ", values[i]);
      if (i%10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *	Main program
 *********************************************************************/
int main(int argc, char *argv[])
{   
	sscanf(argv[1],"%d",&tpoints);
	sscanf(argv[2],"%d",&nsteps);
	check_param();
	printf("Initializing points on the line...\n");
	//init_line();
	printf("Updating all points for all time steps...\n");

    //--------------------------------------------------------------
    // Launch Kernal
    //--------------------------------------------------------------
    float *d_val;
	float *d_remain;
    int tile_width = 1024;
	int remain = tpoints % tile_width;  
    dim3 dimBlock(tile_width);
    dim3 dimGrid(tpoints / tile_width);
    cudaMalloc((void**)&d_val, (tpoints + 1) * sizeof(float));
    cudaMalloc((void**)&d_remain, (remain + 1) * sizeof(float));
	// two streams so kernel function could overlap
	cudaStream_t stream1, stream2;
	cudaStreamCreate(&stream1);
	cudaStreamCreate(&stream2);
	kernel_update<<<dimGrid, dimBlock, 0, stream1>>>(d_val, tpoints, nsteps, 0);
	// second call is for the remainder
	kernel_update<<<1, remain, 0, stream2>>>(d_remain, tpoints, nsteps, tpoints - remain - 1);
    //--------------------------------------------------------------
    // Data transfer back in async. manner 
    //--------------------------------------------------------------
    cudaMemcpyAsync(values, d_val, (tpoints + 1)  * sizeof(float), cudaMemcpyDeviceToHost);
    cudaMemcpyAsync(values + tpoints - remain, d_remain, (remain + 1) * sizeof(float), cudaMemcpyDeviceToHost);
	cudaStreamDestroy(stream1);
	cudaStreamDestroy(stream2);
	printf("Printing final results...\n");
	printfinal();
	printf("\nDone.\n\n");
    cudaFree(d_val);
    cudaFree(d_remain);
	return 0;
}
