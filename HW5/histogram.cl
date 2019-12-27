__kernel void histogram(__global unsigned char *imgR,
                        __global unsigned char *imgG,
                        __global unsigned char *imgB,
                        __global unsigned int R[256],
                        __global unsigned int G[256],
                        __global unsigned int B[256], 
                        unsigned int size)
{   
    const int idx = get_global_id(0);
    for(int i = idx; i < size; i += size){
	    atomic_inc(&R[imgR[idx]]);
	    atomic_inc(&G[imgG[idx]]);
	    atomic_inc(&B[imgB[idx]]);
    }
}
