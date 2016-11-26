#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libspe2.h>
#include <pthread.h>
#include <libmisc.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>

#include "common.h"

extern spe_program_handle_t spu;

#define SIZE 80
#define MIN_SIZE 16 * 8 //min dimension for each spu
#define PATH_SIZE 40
#define KEY_SIZE 4 * sizeof(int)


/* Structure sent as argument to thread routine for SPU threads */
struct thread_spu_arg {
	spe_context_ptr_t ctx;
	struct arg_spe spe_arg;
};

/* Structure holding information of the current file */
struct file_op {
	char *in; /* input file */
	char *out; /* output file */
	char *keyfile; /* key file */
	char *buf; /* input file content */
	char *key; /* key file content */
	char *res; /* buffer after the operation */
	int size; /* size of buf/ res */
	char op; /* operator : encrypt/decrypt */
};

char mod_vect, mod_dma;
struct file_op file;
int spu_threads;

spe_context_ptr_t *ctxs;
struct arg_spe *spe_arg;

void _write_file(char* filepath, void* buf, int size);
void* _read_file(char *filepath, int* size);


/*
 * thread used to start the programs on the SPE's
 */
void *ppu_pthread_function(void *thread_arg) {
	unsigned int entry = SPE_DEFAULT_ENTRY;
	int index = (int)thread_arg;
	
	if (spe_context_run(ctxs[index], &entry, 0, (void*)(&(spe_arg[index])), NULL, NULL) < 0) {
		perror ("Failed running context");
		exit (1);
	}

	pthread_exit(NULL);
}


/* Initialize the structures for SPE */
void initialize_spu_args( int size)
{
	int chunk_size = size / spu_threads;
	int crt_poz = 0;

	for (int i = 0; i < spu_threads; i++) {
		spe_arg[i].buf = file.buf;
		spe_arg[i].key = file.key;
		spe_arg[i].res = file.res;
		spe_arg[i].id = i;
		spe_arg[i].size = chunk_size;
		spe_arg[i].op = file.op;
		spe_arg[i].mod_vect = mod_vect;
		spe_arg[i].mod_dma = mod_dma;

		if (crt_poz >= size) 
			spe_arg[i].poz = -1;
		else spe_arg[i].poz = crt_poz;

		crt_poz += chunk_size;
	}
}


/* Manage data for multiple files */
void process_multi(char *input) {
	int i, size;
	unsigned int mbox_data;

	FILE* fptr = fopen(input, "r");
	char op, in[PATH_SIZE], key[PATH_SIZE], out[PATH_SIZE];  
	if (!fptr) {
		fprintf(stderr, "Can't open %s\n", input);
		return;
	}
	fseek(fptr, 0, SEEK_SET);
	/* ignore first line, it was readen before */
	fscanf(fptr, "%c %s %s %s\n", &op, in, key, out);

	for (i = 0; i < spu_threads; i++) {
		/* wait spe to finish processing the current buffer */
		spe_out_intr_mbox_read(ctxs[i], &mbox_data, 1, SPE_MBOX_ALL_BLOCKING);
	}

	while (1) {
		/* write result to output file */
		_write_file(file.out, file.res, file.size);

		/* prepare structure for the next file */
		memset(file.in, 0, PATH_SIZE);
		memset(file.out, 0, PATH_SIZE);
		memset(file.keyfile, 0, PATH_SIZE);
		free_align(file.buf);
		free_align(file.res);
		free_align(file.key);
		file.buf = NULL;
		file.res = NULL;
		file.key = NULL;

		/* if end of file, announce SPE's */
		if (feof(fptr) || !fscanf(fptr, "%c %s %s %s\n", &file.op,
								file.in, file.keyfile, file.out)) {
			fclose(fptr);
			free_align(file.in);
			free_align(file.out);
			free_align(file.keyfile);
			mbox_data = 0;
			
			for (i = 0; i < spu_threads; i++) {
				spe_in_mbox_write(ctxs[i], &mbox_data, 1, SPE_MBOX_ALL_BLOCKING);
			}
			break;
		}

		/* read input file */
		file.buf = (char*)_read_file(file.in, &file.size);
		/* alloc memory for the result */
		file.res = (char*) malloc_align(size * sizeof(char), 16);
		/* read de key */
		file.key = (char *)_read_file(file.keyfile, &size);

		initialize_spu_args(file.size);

		/* let the SPE's know they can get information from the structures */
		for (i = 0; i < spu_threads; i++) {
			mbox_data = 1;
			spe_in_mbox_write(ctxs[i], &mbox_data, 1, SPE_MBOX_ALL_BLOCKING);
		}
		/* wait for SPE's to finish processing */
		for (i = 0; i < spu_threads; i++) {
			spe_out_intr_mbox_read(ctxs[i], &mbox_data, 1, SPE_MBOX_ALL_BLOCKING);
		}
	}
}

int main(int argc, char **argv) {

	struct timeval t1, t2;
	double total_time = 0;
	gettimeofday(&t1, NULL);
	int size;
	FILE* fptr = NULL;

	spu_threads = atoi(argv[1]);
	mod_vect = argv[2][0];
	mod_dma = argv[3][0];

	if (argc == 5) { /* if we have multi file */
		fptr = fopen(argv[4], "r");
		if (!fptr) {
			fprintf(stderr, "Can't open %s\n", argv[4]);
			return -1;
		}

		/* read the first line from the file and populate the file structure */
		file.in = (char *)malloc_align(PATH_SIZE * sizeof(char), 16);
		file.keyfile = (char *)malloc_align(PATH_SIZE * sizeof(char), 16);
		file.out = (char *)malloc_align(PATH_SIZE * sizeof(char), 16);
		fscanf(fptr, "%c %s %s %s\n", &file.op, file.in, file.keyfile, file.out);

		file.buf = (char *)_read_file(file.in, &size);
		file.size = size;
		file.res = (char*) malloc_align(size * sizeof(char), 16);

		file.key = (char *)_read_file(file.keyfile, &size);
		if (size != KEY_SIZE) {
			fprintf(stderr, "Dimensiune cheie gresita\n");
			return -1;
		}

		fclose(fptr);
	} else {
		/* read the command line arguments*/
		file.in = argv[5];
		file.keyfile = argv[6];
		file.out = argv[7];
		file.op = argv[4][0];

 		/* read the key file */
		file.key = (char *)_read_file(file.keyfile, &size);
		if (size != KEY_SIZE) {
			fprintf(stderr, "Dimensiune cheie gresita\n");
			return -1;
		}

		/* read the input file */
		file.buf = (char*)_read_file(argv[5], &size);
		file.size = size;
		file.res = (char*) malloc_align(size * sizeof(char), 16);

		/* use a smaller number of SPE's if necessary */
		if (size < 16 * spu_threads) {
			spu_threads = size / 16;
		}
	}

	pthread_t threads[spu_threads];
	ctxs = (spe_context_ptr_t *)malloc_align(spu_threads * sizeof(spe_context_ptr_t), 16);
	spe_arg = (struct arg_spe *)malloc_align(spu_threads * sizeof(struct arg_spe), 16);
	int i;

	initialize_spu_args( file.size);

	/* Create several SPE-threads to execute 'SPU'. */
	for(i = 0; i < spu_threads; i++) {
		/* Create context */
		if ((ctxs[i] = spe_context_create (0, NULL)) == NULL) {
			perror ("Failed creating context");
			exit (1);
		}

		/* Load program into context */
		if (spe_program_load (ctxs[i], &spu)) {
			perror ("Failed loading program");
			exit (1);
		}

		/* Create thread for each SPE context */
		if (pthread_create (&threads[i], NULL, &ppu_pthread_function, (void *)i)) {
			perror ("Failed creating thread");
			exit (1);
		}
	}

	if (argc == 5) {
		/* process al the files from multi */
		process_multi(argv[4]);
	} else {
		unsigned int mbox_data = 0;
		for (i = 0; i < spu_threads; i++) {
			/* wait spe to finish proccesing */
			spe_out_intr_mbox_read(ctxs[i], &mbox_data, 1, SPE_MBOX_ALL_BLOCKING);
			/* send signal to SPE's that no other file needs processing */
			mbox_data = 0;
			spe_in_mbox_write(ctxs[i], &mbox_data, 1, SPE_MBOX_ALL_BLOCKING);
		}
	}


	/* Wait for SPU-thread to complete execution. */
	for (i = 0; i < spu_threads; i++) {
		if (pthread_join (threads[i], NULL)) {
			perror("Failed pthread_join");
			exit (1);
		}

		/* Destroy context */
		if (spe_context_destroy (ctxs[i]) != 0) {
			perror("Failed destroying context");
			exit (1);
		}
	}

	if (argc != 5) {
		_write_file(file.out, file.res, file.size);
	}

	gettimeofday(&t2, NULL);
	total_time += GET_TIME_DELTA(t1, t2);

	printf("\nThe program has successfully executed in %f\n", total_time);
	return (0);
}




///////////utils.c

int _open_for_read(char* path){
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0){
		fprintf(stderr, "%s: Error opening %s\n", __func__, path);
		exit(0);
	}
	return fd;
}

int _open_for_write(char* path){
	int fd;

	fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (fd < 0){
		fprintf(stderr, "%s: Error opening %s\n", __func__, path);
		exit(0);
	}
	return fd;
}

void _write_file(char* filepath, void* buf, int size){
	char *ptr;
	int left_to_write, bytes_written, fd;

	fd = _open_for_write(filepath);

	ptr = (char*)buf;
	left_to_write = size;

	while (left_to_write > 0){
		bytes_written = write(fd, ptr, left_to_write);
		if (bytes_written <= 0){
			fprintf(stderr, "%s: Error writing buffer. "
					"fd=%d left_to_write=%d size=%d bytes_written=%d\n", 
					__func__, fd, left_to_write, size, bytes_written);
			exit(0);
		}
		left_to_write -= bytes_written;
		ptr += bytes_written;
	}

	close(fd);

}

void* _read_file(char* filepath, int* size_ptr){
	char *ptr;
	int left_to_read, bytes_read, size, fd;
	void* buf;

	fd = _open_for_read(filepath);

	size = lseek(fd, 0, SEEK_END);
	if (size <= 0) {
		fprintf(stderr, "%s: Error getting file size. filepath=%s\n",
				__func__, filepath);
		exit(0);
	}
	buf =  (void *)malloc_align(size, 16);
	if (!buf) {
		fprintf(stderr, "%s: Error allocating %d bytes\n", __func__,
				size);
		exit(0);   
	}
	lseek(fd, 0, SEEK_SET);

	ptr = (char*) buf;
	left_to_read = size;
	while (left_to_read > 0){
		bytes_read = read(fd, ptr, left_to_read);
		if (bytes_read <= 0){
			fprintf(stderr, "%s: Error reading buffer. "
					"fd=%d left_to_read=%d size=%d bytes_read=%d\n", 
					__func__, fd, left_to_read, size, bytes_read);
			exit(0);
		}
		left_to_read -= bytes_read;
		ptr += bytes_read;
	}

	close(fd);
	*size_ptr = size;

	return buf;
}

