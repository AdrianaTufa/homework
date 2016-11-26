#ifndef COMMON_H_
#define COMMON_H_

#define GET_TIME_DELTA(t1, t2) ((t2).tv_sec - (t1).tv_sec + \
		((t2).tv_usec - (t1).tv_usec) / 1000000.0)

#define SCALAR '0'
#define VECTORIAL '1'
#define NO_DOUBLE '0'
#define DOUBLE_BUFF '1'


/* Structure from which SPU will get information */
struct arg_spe {
	int id;  /* SPU id */
	int poz; /* assigned start position from buf */
	int size; /* number of bytes to be processed */
	char *buf; /* input file content */
	char *key;
	char *res; /* result after encrypting/ decrypting */
	char op; /* encrypt/ decrypt*/
	char mod_vect; /* scalar/ vectorial */
	char mod_dma; /* single / double buffering */
	char cv[5]; /* dummy chars to have a dimension which is multiple of 16 */
};

#endif
