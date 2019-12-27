#include <fstream>
#include <iostream>
#include <string>
#include <ios>
#include <vector>
#include <cstdlib>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
} Image;

Image *readbmp(const char *filename)
{
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->data = new RGB[w * h]{};
    for (int i = 0; i < ret->size; i++)
    {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    fout.write((char *)header, 54);
    fout.write((char *)img->data, img->size * 4);
    fout.close();
}

cl_program load_program(cl_context context, const char* filename)
{
    std::ifstream input(filename, std::ios_base::binary);

    // get length
    input.seekg(0, std::ios_base::end);
    size_t length = input.tellg();
    input.seekg(0, std::ios_base::beg);

    // read data
    std::vector<char> data(length + 1);
    input.read(&data[0], length);
    data[length] = 0;

    // build
    const char* source = &data[0];
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
    clBuildProgram(program, 0, 0, 0, 0, 0);
    return program;
}


int main(int argc, char *argv[])
{
    char *filename;
    if (argc >= 2){
        int many_img = argc - 1;
        for (int i = 0; i < many_img; i++){
            filename = argv[i + 1];
            cl_uint num;
            clGetPlatformIDs(0, 0, &num);

	        std::vector<cl_platform_id> platforms(num);
	        clGetPlatformIDs(num, &platforms[0], &num);

	        cl_context_properties prop[] = 
                {CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platforms[0]), 0};
	        cl_context context = clCreateContextFromType(prop, CL_DEVICE_TYPE_GPU, NULL, NULL, NULL);

	        size_t cb;
	        clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
	        std::vector<cl_device_id> devices(cb / sizeof(cl_device_id));
	        clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, &devices[0], 0);

	        clGetDeviceInfo(devices[0], CL_DEVICE_NAME, 0, NULL, &cb);
	        std::string devname;
	        devname.resize(cb);
	        clGetDeviceInfo(devices[0], CL_DEVICE_NAME, cb, &devname[0], 0);

	        cl_command_queue queue = clCreateCommandQueue(context, devices[0], 0, 0);
            Image *img = readbmp(filename);

            std::cout << img->weight << ":" << img->height << "\n";
	        int DATA_SIZE = img->weight * img->height;
	        unsigned char Rin[DATA_SIZE];
	        unsigned char Gin[DATA_SIZE];
	        unsigned char Bin[DATA_SIZE];
	        for(int i=0; i<DATA_SIZE; i++){
		        Rin[i] = img->data[i].R;
		        Gin[i] = img->data[i].G;
		        Bin[i] = img->data[i].B;
	        }
            unsigned int R[256];
            unsigned int G[256];
            unsigned int B[256];
	        size_t size = DATA_SIZE*sizeof(unsigned char);
	        cl_mem cl_rin = 
                clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, &Rin[0], NULL);  
	        cl_mem cl_gin = 
                clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, &Gin[0], NULL);  
	        cl_mem cl_bin = 
                clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, &Bin[0], NULL);

	        cl_mem cl_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 256, NULL, NULL);
	        cl_mem cl_g = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 256, NULL, NULL);
	        cl_mem cl_b = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * 256, NULL, NULL);

	        cl_program program = load_program(context, "histogram.cl");

	        cl_kernel histogram = clCreateKernel(program, "histogram", 0);

	        clSetKernelArg(histogram, 0, sizeof(cl_mem), &cl_rin);
	        clSetKernelArg(histogram, 1, sizeof(cl_mem), &cl_gin);
            clSetKernelArg(histogram, 2, sizeof(cl_mem), &cl_bin);
            clSetKernelArg(histogram, 3, sizeof(cl_mem), &cl_r);
            clSetKernelArg(histogram, 4, sizeof(cl_mem), &cl_g);
            clSetKernelArg(histogram, 5, sizeof(cl_mem), &cl_b);
            clSetKernelArg(histogram, 6, sizeof(cl_int), (void*)&DATA_SIZE);
            size_t work_size = 256;

            clEnqueueNDRangeKernel(queue, histogram, 1, 0, &work_size, 0, 0, 0, 0);
            clEnqueueReadBuffer(queue, cl_r, CL_TRUE, 0, sizeof(unsigned int) * 256, &R[0], 0, 0, 0);
            clEnqueueReadBuffer(queue, cl_g, CL_TRUE, 0, sizeof(unsigned int) * 256, &G[0], 0, 0, 0);
            clEnqueueReadBuffer(queue, cl_b, CL_TRUE, 0, sizeof(unsigned int) * 256, &B[0], 0, 0, 0);

	        //histogram(img,R,G,B);
    	    clReleaseKernel(histogram);
    	    clReleaseProgram(program);
    	    clReleaseMemObject(cl_rin);
    	    clReleaseMemObject(cl_gin);
    	    clReleaseMemObject(cl_bin);
    	    clReleaseMemObject(cl_r);
    	    clReleaseMemObject(cl_g);
    	    clReleaseMemObject(cl_b);
    	    clReleaseCommandQueue(queue);
    	    clReleaseContext(context);

            int max = 0;
            for(int i=0;i<256;i++){
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }
            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];
            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){
                    if(R[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
		    else 
			ret->data[256*i+j].R = 0;
                    if(G[j]*256/max > i)
                        ret->data[256*i+j].G = 255;
		    else 
			ret->data[256*i+j].G = 0;
                    if(B[j]*256/max > i)
                        ret->data[256*i+j].B = 255;
		    else 
			ret->data[256*i+j].B = 0;
                }
            }

            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);
        }
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}

