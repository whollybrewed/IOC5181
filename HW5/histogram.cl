__kernel void histogram(__global unsigned char *Rin, __global unsigned char *Gin, __global unsigned char *Bin, __global unsigned int R[256], __global unsigned int G[256], __global unsigned int B[256],int siz)
{
    const int ix = get_global_id(0);
    int j;
    R[ix] = 0;
    G[ix] = 0;
    B[ix] = 0;
    for(j=ix; j<siz; j=j+256){
    	unsigned char tempr = Rin[j];
    	unsigned char tempg = Gin[j];
    	unsigned char tempb = Bin[j];
        atomic_inc(&R[tempr]);
        atomic_inc(&G[tempg]);
        atomic_inc(&B[tempb]);
    }
}
