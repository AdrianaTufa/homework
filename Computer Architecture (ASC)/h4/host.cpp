#include <stdio.h>
#include <CL/cl.h>
#include <iostream>
#include <string>
#include <vector>
#include <math.h>

#include "helper.hpp"

using namespace std;

static int kmrange;

/* Structure holding information about a city */
typedef struct {
	int id; /* city id */
	float latitude, longitude; /* coordinates */
	int population;
	int acc_pop;
} city;

/* cities read from file */
vector<city> cities;


/**
* Retrieve GPU device
*/
void gpu_find(cl_device_id &device)
{
	cl_platform_id platform;
	cl_uint platform_num = 0;
	cl_platform_id* platform_list = NULL;

	cl_uint device_num = 0;
	cl_device_id* device_list = NULL;

	size_t attr_size = 0;
	cl_char* attr_data = NULL;

	/* get num of available OpenCL platforms */
	CL_ERR( clGetPlatformIDs(0, NULL, &platform_num));
	platform_list = new cl_platform_id[platform_num];
	DIE(platform_list == NULL, "alloc platform_list");

	/* get all available OpenCL platforms */
	CL_ERR( clGetPlatformIDs(platform_num, platform_list, NULL));

	/* list all platforms and VENDOR/VERSION properties */
	for(uint i=0; i<platform_num; i++)
	{
		/* get attribute CL_PLATFORM_VENDOR */
		CL_ERR( clGetPlatformInfo(platform_list[i],
				CL_PLATFORM_VENDOR, 0, NULL, &attr_size));
		attr_data = new cl_char[attr_size];
		DIE(attr_data == NULL, "alloc attr_data");

		/* get data CL_PLATFORM_VENDOR */
		CL_ERR( clGetPlatformInfo(platform_list[i],
				CL_PLATFORM_VENDOR, attr_size, attr_data, NULL));

		/* select platform based on CL_PLATFORM_VENDOR */
		if(string((const char*)attr_data).find("NVIDIA", 0) != string::npos)
			platform = platform_list[i]; /* select NVIDIA platform */
		delete[] attr_data;
	}

	/* no platform found */
	DIE(platform == 0, "platform selection");

	/* get num of available OpenCL devices type GPU on the selected platform */
	CL_ERR( clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &device_num));
	device_list = new cl_device_id[device_num];
	DIE(device_list == NULL, "alloc devices");

	/* get all available OpenCL devices type GPU on the selected platform */
	CL_ERR( clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU,
		  device_num, device_list, NULL));

	/* select first available GPU */
	if(device_num > 0)
		device = device_list[0];
	else
		device = 0;

	delete[] platform_list;
	delete[] device_list;
}


/* Put information from cities in simple arrays */
void copy_info(cl_float *v_lat, cl_float *v_long, cl_int *v_pop)
{
	int n = cities.size();

	for (int i = 0; i < n; i++) {
		v_lat[i] = cities[i].latitude;
		v_long[i] = cities[i].longitude;
		v_pop[i] = cities[i].population;
	}
}


/**
* Kernel execution using the selected device
*/
void gpu_exec_kernel(cl_device_id device)
{
	cl_int ret;
	cl_context context;
	cl_command_queue cmd_queue;
	cl_program program;
	cl_kernel kernel;
	cl_event event;

	string kernel_src;
	int no_cities = cities.size();

	/* create a context for the device */
	context = clCreateContext(0, 1, &device, NULL, NULL, &ret);
	CL_ERR( ret );

	/* create a command queue for the device in the context */
	cmd_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE,
									&ret);
	CL_ERR( ret );

	/* alloc buffers on CPU */
	cl_float *host_lat = new cl_float[no_cities]; /* array of latitudes */
	DIE ( host_lat == NULL, "alloc host_lat" );
	cl_float *host_long = new cl_float[no_cities]; /* array of longitudes */
	DIE ( host_long == NULL, "alloc host_long" );
	cl_int *host_pop = new cl_int[no_cities]; /* array of population */
	DIE ( host_pop == NULL, "alloc host_pop" );
	cl_int *host_acc_pop = new cl_int[no_cities]; /* array of acc population */
	DIE ( host_acc_pop == NULL, "alloc host_pop" );

	/* populate arrays with data */
	copy_info(host_lat, host_long, host_pop);


	/* alloc memory on GPU - the corresponding buffers for host_.. arrays*/	
	cl_mem dev_lat = clCreateBuffer(context, CL_MEM_READ_WRITE,
				  sizeof(cl_float) * no_cities, NULL, &ret);
	CL_ERR( ret );
	cl_mem dev_long = clCreateBuffer(context, CL_MEM_READ_WRITE,
				  sizeof(cl_float) * no_cities, NULL, &ret);
	CL_ERR( ret );
	cl_mem dev_pop = clCreateBuffer(context, CL_MEM_READ_WRITE,
				  sizeof(cl_int) * no_cities, NULL, &ret);
	CL_ERR( ret );
	cl_mem dev_acc_pop = clCreateBuffer(context, CL_MEM_READ_WRITE,
				  sizeof(cl_int) * no_cities, NULL, &ret);
	CL_ERR( ret );

	/* read the program to execute on kernel */
	read_kernel("kernel_acc_pop.cl", kernel_src);
	const char* kernel_c_str = kernel_src.c_str();

	program = clCreateProgramWithSource(context, 1,
		  &kernel_c_str, NULL, &ret);
	CL_ERR( ret );

	/* build the program */
	ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	CL_COMPILE_ERR( ret, program, device );

	kernel = clCreateKernel(program, "kernel_acc_pop", &ret);
	CL_ERR( ret );

	/* set the arguments for kernel function */
	CL_ERR( clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&dev_lat) );
	CL_ERR( clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&dev_long) );
	CL_ERR( clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&dev_pop) );
	CL_ERR( clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&dev_acc_pop) );
	CL_ERR( clSetKernelArg(kernel, 4, sizeof(int), &no_cities) );
	CL_ERR( clSetKernelArg(kernel, 5, sizeof(int), &kmrange) );

	size_t globalSize[2] = {no_cities, 0};

	/* write the values from host to device's arrays */
	CL_ERR( clEnqueueWriteBuffer(cmd_queue, dev_lat, CL_TRUE, 0,
		  sizeof(cl_float) * no_cities, host_lat, 0, NULL, NULL));
	CL_ERR( clEnqueueWriteBuffer(cmd_queue, dev_long, CL_TRUE, 0,
		  sizeof(cl_float) * no_cities, host_long, 0, NULL, NULL));
	CL_ERR( clEnqueueWriteBuffer(cmd_queue, dev_pop, CL_TRUE, 0,
		  sizeof(cl_int) * no_cities, host_pop, 0, NULL, NULL));

	ret = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL,
		  globalSize, NULL, 0, NULL, &event);
	CL_ERR( ret );

	clWaitForEvents(1, &event);

	/* bring the results from device to host */
	CL_ERR( clEnqueueReadBuffer(cmd_queue, dev_acc_pop, CL_TRUE, 0,
		  sizeof(cl_int) * no_cities, host_acc_pop, 0, NULL, NULL));

	CL_ERR( clFinish(cmd_queue) );

	/* print results to stdout */
	for (int i = 0; i < no_cities; i++) {
		printf("%d\n", host_acc_pop[i]);
	}

	CL_ERR( clReleaseMemObject(dev_lat) );
	CL_ERR( clReleaseMemObject(dev_long) );
	CL_ERR( clReleaseMemObject(dev_pop) );
	CL_ERR( clReleaseMemObject(dev_acc_pop) );

	delete[] host_long;
	delete[] host_lat;
	delete[] host_pop;
}


/* read the cities from file */
void read_cities(char *filename);


/**
* MAIN function (CPU/HOST)
*/
int main(int argc, char** argv)
{
	cl_device_id device;

	if (argc != 3) {
		printf("Usage ./accpop_gpu <krange> <filename>\n");
		return 0;
	}

	kmrange = atoi(argv[1]);

	read_cities(argv[2]);

	/* retrieve platform and device (GPU NVIDIA TESLA) */
	gpu_find(device);

	/* perform kernel function using selected device (GPU NVIDIA TESLA) */
	gpu_exec_kernel(device);

	return 0;
}


void read_cities(char *filename)
{

	FILE *fptr = fopen(filename, "r");

	int id, pop;
	char name[20], code[20], aux[20];
	float lat, longi;

	/* read and ignore the first line */
	fscanf(fptr, "%s %s %s %s %s\n", aux, aux, aux, aux, aux);

	while (fscanf(fptr, "%d %s %f,%f %s %d\n", &id, name, &lat, &longi,
												code, &pop)) {
		/* check correctness */
		if (lat > 90 || lat < -90 || longi > 180 || longi < -180) {
			if (feof(fptr)) break;
			continue;
		}

		/* add a new city to the vector */
		city *new_city = new city();
    	new_city->id = id;
    	new_city->latitude = lat;
    	new_city->longitude = longi;
    	new_city->population = pop;
    	new_city->acc_pop = pop;

    	cities.push_back(*new_city);

		if (feof(fptr)) break;
	}

	fclose(fptr);
}
