#include <stdio.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <sys/time.h>

#include "../common.h"

#define wait_tag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();

#define KEY_SIZE 4
#define MAX_DMA_SIZE 16 * 1024

#define DELTA    0x9e3779b9
#define DPRINT printf 
#define NUM_ROUNDS 32
#define DEC_SUM  0xC6EF3720
 

void encrypt (unsigned int* v, unsigned int* k, int num_ints, char mod_vect);
void decrypt (unsigned int* v, unsigned int* k, int num_ints, char mod_vect);

int main(unsigned long long speid, unsigned long long argp, unsigned long long envp){

	printf("[SPU 0x%llx] started\n", speid);

	struct timeval t1, t2, t3, t4;
	double total_time = 0, cpu_time = 0;
	unsigned int mbox_data = 1;
	struct arg_spe args __attribute__ ((aligned(16)));
	unsigned int key[KEY_SIZE] __attribute__ ((aligned(16)));

	gettimeofday(&t3, NULL);

	envp = envp; //silence warnings

	uint32_t tag_id = mfc_tag_reserve();
	if (tag_id==MFC_TAG_INVALID){
		printf("SPU: ERROR can't allocate tag ID\n"); 
		return -1;
	}

	/* get initial structure */
	mfc_get((void *)&args, (unsigned int)argp, 32, tag_id, 0, 0);
	wait_tag(tag_id);

	/* get the key from structure */
	mfc_get((void *)key, (unsigned int)args.key, KEY_SIZE * sizeof(unsigned int), tag_id, 0, 0);
	wait_tag(tag_id);

	if (args.mod_dma == NO_DOUBLE) {
		while (1) {
			/* verifiy transfer size */
			int transfer_size = args.size < MAX_DMA_SIZE ? args.size : MAX_DMA_SIZE;
			unsigned int buf[transfer_size / sizeof(int)] __attribute__ ((aligned(16)));
			int count = 0;

			/* process the corresponding buffer */
			while (count < args.size) {
				/* get part of the buffer */
				mfc_get((void *)buf, args.buf + args.poz + count, transfer_size, tag_id, 0, 0);
				wait_tag(tag_id);

				gettimeofday(&t1, NULL);

				/* apply operation on buffer */
				if (args.op == 'e') {
					encrypt(buf, key, transfer_size / sizeof(int), args.mod_vect);
				} else {
					decrypt(buf, key, transfer_size / sizeof(int), args.mod_vect);
				}

				gettimeofday(&t2, NULL);
				cpu_time += GET_TIME_DELTA(t1, t2);

				/* write back the result */
				mfc_put((void *)buf, args.res + args.poz + count, transfer_size, tag_id, 0, 0);
				wait_tag(tag_id);

				count += transfer_size;
			}

			/* send message to PPU that the current buffer is ready*/
			spu_write_out_intr_mbox(mbox_data);
			/* wait for response from PPU */
			mbox_data = spu_read_in_mbox();

			/* if there is still data to process, update the structure*/
			if (mbox_data != 0) {
				mfc_get((void *)&args, (unsigned int)argp, 32, tag_id, 0, 0);
				wait_tag(tag_id);
				/* get the new key */
				mfc_get((void *)key, (unsigned int)args.key, KEY_SIZE * sizeof(unsigned int), tag_id, 0, 0);
				wait_tag(tag_id);
			} else break;
		}

	} else { /* if double buffering selected */
		uint32_t tag[2];	  
		tag[0] = mfc_tag_reserve();
		if (tag[0]==MFC_TAG_INVALID){
			printf("SPU: ERROR can't allocate tag ID\n"); return -1;
		}
		tag[1] = mfc_tag_reserve();
		if (tag[1]==MFC_TAG_INVALID){
			printf("SPU: ERROR can't allocate tag ID\n"); return -1;
		}

		while (1) {
			/* verify the transfer size */
			int transfer_size = args.size < MAX_DMA_SIZE ? args.size : MAX_DMA_SIZE;
			/* alloc 2 buffers */
			unsigned int buf[2][transfer_size / sizeof(int)] __attribute__ ((aligned(16)));
			int crt_buf = 0, nxt_buf, count = 0;

			/* get data for the first buffer */
			mfc_get((void *)buf[0], args.buf + args.poz + count, transfer_size, tag[0], 0, 0);
			count += transfer_size;

			while (count < args.size) {
				nxt_buf = crt_buf^1;

				/* send dma command to get data for the other buffer */
				mfc_getb((void *)buf[nxt_buf], args.buf + args.poz + count, transfer_size, tag[nxt_buf], 0, 0);
				/* wait for the current buffer*/
				wait_tag(tag[crt_buf]);

				gettimeofday(&t1, NULL);
				/* apply operation on buffer */
				if (args.op == 'e') {
					encrypt(buf[crt_buf], key, transfer_size / sizeof(int), args.mod_vect);
				} else {
					decrypt(buf[crt_buf], key, transfer_size / sizeof(int), args.mod_vect);
				}

				gettimeofday(&t2, NULL);
				cpu_time += GET_TIME_DELTA(t1, t2);

				/* write result to PPU memory */
				mfc_put((void *)buf[crt_buf], args.res + args.poz + count - transfer_size, transfer_size, tag[crt_buf], 0, 0);

				crt_buf = nxt_buf;
				count += transfer_size;
			}

			wait_tag(tag[crt_buf]);

			gettimeofday(&t1, NULL);
			/* preocess the last buffer */
			if (args.op == 'e') {
				encrypt(buf[crt_buf], key, transfer_size / sizeof(int), args.mod_vect);
			} else {
				decrypt(buf[crt_buf], key, transfer_size / sizeof(int), args.mod_vect);
			}

			gettimeofday(&t2, NULL);
			cpu_time += GET_TIME_DELTA(t1, t2);

			mfc_put((void *)buf[crt_buf], args.res + args.poz + count - transfer_size, transfer_size, tag[crt_buf], 0, 0);
			wait_tag(tag[crt_buf]);

			/* send message to PPU */
			spu_write_out_intr_mbox(mbox_data);
			/* wait answer from PPU */
			mbox_data = spu_read_in_mbox();

			/* if there is still data to process, update info*/
			if (mbox_data != 0) {
				mfc_get((void *)&args, (unsigned int)argp, 32, tag_id, 0, 0);
				wait_tag(tag_id);

				/* get the new key */
				mfc_get((void *)key, (unsigned int)args.key, KEY_SIZE * sizeof(unsigned int), tag_id, 0, 0);
				wait_tag(tag_id);
			} else break;
		}

		mfc_tag_release(tag[0]);
		mfc_tag_release(tag[1]);

	}

	gettimeofday(&t4, NULL);
	total_time += GET_TIME_DELTA(t3, t4);

	printf("[SPU %d] finished with TOTAL TIME %f and CPU TIME %f\n", args.id, total_time, cpu_time);

	return 0;
}


/* encrypt one block of data; scalar mode */
void encrypt_block (unsigned int* v, unsigned int* k) {
	unsigned int i, sum = 0;

	for (i=0; i < NUM_ROUNDS; i++) {                     
		sum += DELTA;
		v[0] += ((v[1]<<4) + k[0]) ^ (v[1] + sum) ^ ((v[1]>>5) + k[1]);
		v[1] += ((v[0]<<4) + k[2]) ^ (v[0] + sum) ^ ((v[0]>>5) + k[3]);
	}                               
}


/* encrypt data; vectorial mode */
void encrypt_block_vect (unsigned int* v, unsigned int* key, int num_ints) {
	unsigned int i, sum = 0, j, n;

	vector unsigned int *vect = (vector unsigned int *)v;
	vector unsigned int *k = (vector unsigned int *)key;
	vector unsigned int a, b, c, k1, k2;

	/* masks for vectorial operations*/
	vector unsigned char patternB = (vector unsigned char){4, 5, 6, 7, 0, 1, 2,
											 3, 12, 13, 14, 15, 8, 9, 10, 11};
	vector unsigned char patternK1 = (vector unsigned char){0, 1, 2, 3, 8, 9,
											 10, 11, 0, 1, 2, 3, 8, 9, 10, 11};
	vector unsigned char patternK2 = (vector unsigned char){4, 5, 6, 7, 12, 13,
											 14, 15, 4, 5, 6, 7, 12, 13, 14, 15};
	vector unsigned int mask1 = (vector unsigned int){0xFFFFFFFF, 0x00000000,
													  0xFFFFFFFF, 0x00000000};
	vector unsigned int mask2 = (vector unsigned int){0x00000000, 0xFFFFFFFF,
													  0x00000000, 0xFFFFFFFF};
	vector unsigned int zero = (vector unsigned int)spu_splats(0);
	vector unsigned int aux;

	k1 = spu_shuffle(k[0], k[0], patternK1);
	k2 = spu_shuffle(k[0], k[0], patternK2);

	n = num_ints/(16/sizeof(unsigned int));

	for (j = 0; j < n; j++) {
		sum = 0;
		for (i = 0; i < NUM_ROUNDS; i++) {
			sum += DELTA;

			/* make operations for the even positions */
			b = spu_shuffle(vect[j], vect[j], patternB);
			c = spu_rlmask(b, -5);
			a = spu_sl(b, 4);

			aux = spu_xor((a +k1), spu_xor(b + spu_splats(sum), c + k2));
			vect[j] += spu_sel(aux, zero, mask2);

			/* make operations for the odd positions */
			b = spu_shuffle(vect[j], vect[j], patternB);
			c = spu_rlmask(b, -5);
			a = spu_sl(b, 4);

			aux = spu_xor((a+k1), spu_xor(b + spu_splats(sum), c + k2));	
			vect[j] += spu_sel(aux, zero, mask1);	
		}
	}                             
}


/* decrypt onde block of data; scalar mode */
void decrypt_block (unsigned int* v, unsigned int* k) {
	unsigned int sum = DEC_SUM, i;
	for (i=0; i<NUM_ROUNDS; i++) { 
		v[1] -= ((v[0]<<4) + k[2]) ^ (v[0] + sum) ^ ((v[0]>>5) + k[3]);
		v[0] -= ((v[1]<<4) + k[0]) ^ (v[1] + sum) ^ ((v[1]>>5) + k[1]);
		sum -= DELTA;
	}                             
}


/* decrypt data; vectorial mode */
void decrypt_block_vect (unsigned int* v, unsigned int* key, int num_ints) {
	unsigned int i, sum = DEC_SUM, j, n;

	/* cast data to vector types */
	vector unsigned int *vect = (vector unsigned int *)v;
	vector unsigned int *k = (vector unsigned int *)key;
	vector unsigned int a, b, c, k1, k2;

	/* maska for vectorial operations */
	vector unsigned char pattern = (vector unsigned char){4, 5, 6, 7, 0, 1, 2,
											 3, 12, 13, 14, 15, 8, 9, 10, 11};
	vector unsigned char patternK1 = (vector unsigned char){8, 9, 10, 11, 0, 1,
											 2, 3, 8, 9, 10, 11, 0, 1, 2, 3};
	vector unsigned char patternK2 = (vector unsigned char){12, 13, 14, 15, 4,
										 5, 6, 7, 12, 13, 14, 15, 4, 5, 6, 7};
	vector unsigned int mask1 = (vector unsigned int){0xFFFFFFFF, 0x00000000,
													  0xFFFFFFFF, 0x00000000};
	vector unsigned int mask2 = (vector unsigned int){0x00000000, 0xFFFFFFFF,
													  0x00000000, 0xFFFFFFFF};
	vector unsigned int zero = (vector unsigned int)spu_splats(0);
	vector unsigned int aux;

	k1 = spu_shuffle(k[0], k[0], patternK1);
	k2 = spu_shuffle(k[0], k[0], patternK2);

	/* number of elements in vect */
	n = num_ints/(16/sizeof(unsigned int));

	for (j = 0; j < n; j++) {
		sum = DEC_SUM;
		for (i = 0; i < NUM_ROUNDS; i++) {
			/* make operations for the odd positions */
			b = vect[j];
			a = spu_sl(b, 4);
			c = spu_rlmask(b, -5);

			aux = spu_xor(a + k1, spu_xor(b + spu_splats(sum), c + k2));
			aux = spu_shuffle(aux, aux, pattern);
			vect[j] -= spu_sel(aux, zero, mask1);

			/* make operations for the even positions */
			b = vect[j];
			a = spu_sl(b, 4);
			c = spu_rlmask(b, -5);

			aux = spu_xor(a + k1, spu_xor(b + spu_splats(sum), c + k2));
			aux = spu_shuffle(aux, aux, pattern);
			vect[j] -= spu_sel(aux, zero, mask2);

			sum -= DELTA;
		}
	}     
}


void encrypt (unsigned int* v, unsigned int* k, int num_ints, char mod_vect) {

	if (mod_vect == VECTORIAL) 
		encrypt_block_vect(v, k, num_ints);
	else {
		for (int i = 0; i < num_ints; i += 2) { 
			encrypt_block(&v[i], k);
		}
	}
}

void decrypt (unsigned int* v, unsigned int* k, int num_ints, char mod_vect)  {

	if (mod_vect == VECTORIAL)
		decrypt_block_vect(v, k, num_ints);
	else {
		for (int i = 0; i < num_ints; i += 2) {
			decrypt_block(&v[i], k);
		}
	}
}

