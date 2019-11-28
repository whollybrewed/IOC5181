#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#ifndef W
#define TAG 0
#define W 20                                      // Width
#endif
int main(int argc, char **argv) 
{
	int L = atoi(argv[1]);                        // Length
	int iteration = atoi(argv[2]);                // Iteration
	srand(atoi(argv[3]));                         // Seed
	float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity
	int *temp = malloc(L * W * sizeof(int));      // Current temperature
	int *next = malloc(L * W * sizeof(int));      // Next time step
	int rank, num_prc;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &num_prc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int local_L = L / num_prc;
	int local_start = rank * local_L;
	int local_end = rank * local_L + local_L - 1;
	int global_min, global_balance = 1;
	
	for (int i = 0; i < L; i++) {
		for (int j = 0; j < W; j++) {
			temp[i * W + j] = random() >> 3;
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	int count = 0, balance = 0;
	while (iteration--) {     // Compute with up, left, right, down points
		balance = 1;
		count++;

		if (rank == 0){
			MPI_Sendrecv(&temp[local_end * W], W, MPI_INT, 1, TAG,
						 &temp[local_end * W + W], W, MPI_INT, 1, TAG,  
						 MPI_COMM_WORLD, &status);
		}
		
		else if (rank == num_prc - 1){
			MPI_Sendrecv(&temp[local_start * W], W, MPI_INT, num_prc - 2, TAG,
						 &temp[local_start * W - W], W, MPI_INT, num_prc - 2, TAG,  
					 	 MPI_COMM_WORLD, &status);
		}
		else{
			MPI_Sendrecv(&temp[local_end * W], W, MPI_INT, rank + 1, TAG,
						 &temp[local_end * W + W], W, MPI_INT, rank + 1, TAG,  
						 MPI_COMM_WORLD, &status);
			MPI_Sendrecv(&temp[local_start * W], W, MPI_INT, rank - 1, TAG,
						 &temp[local_start * W - W], W, MPI_INT, rank - 1, TAG,  
						 MPI_COMM_WORLD, &status);
		}
	
				
		MPI_Barrier(MPI_COMM_WORLD);
		for (int i = local_start; i < local_end + 1; i++) {
			for (int j = 0; j < W; j++) {
				float t = temp[i * W + j] / d;
				t += temp[i * W + j] * -4;
				t += temp[(i - 1 <  0 ? 0 : i - 1) * W + j];
				t += temp[(i + 1 >= L ? i : i + 1) * W + j];
				t += temp[i * W + (j - 1 <  0 ? 0 : j - 1)];
				t += temp[i * W + (j + 1 >= W ? j : j + 1)];
				t *= d;
				next[i * W + j] = t ;
				if (next[i * W + j] != temp[i * W + j]) {
					balance = 0;
				}
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Allreduce(&balance, &global_balance, 1, MPI_INT, MPI_PROD, MPI_COMM_WORLD);		
		if (global_balance) {
			break;
		}
		int *tmp = temp;
		temp = next;
		next = tmp;
	}
	int min = temp[local_start * W];
	for (int i = local_start; i < local_end + 1; i++) {
		for (int j = 0; j < W; j++) {
			if (temp[i * W + j] < min) {
				min = temp[i * W + j];
			}
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Reduce(&min, &global_min, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
	if (rank == 0)
		printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, global_min);
	MPI_Finalize();
	return 0;
}
