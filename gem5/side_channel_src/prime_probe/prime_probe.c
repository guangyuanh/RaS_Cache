#include <stdio.h>
#include <stdlib.h>
#include "aes.h"
#include <time.h>
#include <inttypes.h>
#include <malloc.h>
#define KEY_LENGTH 16
#define BUF_SIZE 256
//#define TRIALS 65536
#define LEN 16
#define L1_CACHE_SIZE 0x8000
#define L1_ASSOC 8
#define L1_LINE_SIZE 64
#define WAY_SIZE L1_CACHE_SIZE/L1_ASSOC
#define NUMSET L1_CACHE_SIZE/L1_ASSOC/L1_LINE_SIZE

#define ATTACKER_ID 0
#define VICTIM_ID 1
#define MSR_DOMAIN 0xC0010118

/*
 * Get accurate cycle count from processor. for x86
 */
#define rdtscll(high, low) \
	do { \
		asm volatile("rdtsc\n" \
			     "movl %%edx, %0\n" \
			     "movl %%eax, %1\n" \
			     : "=r" (high), "=r" (low) \
			     :	 	\
			     : "%eax", "%edx");\
	} while (0)

char in[LEN];
char workarea[LEN];
char out[BUF_SIZE][NUMSET*2+16];
AES_KEY expanded; 
FILE *infile;
char* mem_start;


int main(int argc, char** argv)
{
    const unsigned char key_byte[KEY_LENGTH]={0x0f, 0x1e, 0xdb, 0x65, 0xe6, 0xd1, 0x03, 0x5e, 0xfa, 0x94, 0x1f, 0x0c, 0x4b, 0x41, 0xff, 0xbb};
    // const unsigned char key_byte[KEY_LENGTH]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int n, i, k;
    FILE *outfile = 0;
    int len;
    int set, index;
    int trials = atoi(argv[2]);
    unsigned int cycles_begin_hi, cycles_begin_low;
    unsigned int cycles_end_hi, cycles_end_low;
    uint16_t t_diff;

#ifdef MSR_EN
    printf("MSR_EN defined: %x\n", MSR_EN);
    asm volatile ( "wrmsr": : "c"((uint32_t) MSR_EN),
            "a"(0), "d"(0));
#endif
    printf("Number of sets: %d\n", NUMSET);  
    printf("First key byte: %#x\n", key_byte[0]);

    //mem_start = malloc(L1_CACHE_SIZE);
    // Make mem_start aligned
    mem_start = memalign(8192, L1_CACHE_SIZE);
    printf("Base address of mem_start: %p\n", mem_start);

    infile = fopen("random_file_large", "r");
    if(infile == NULL){
        printf("\ncould not open file\n");
        return -1;
    }
    outfile = fopen(argv[1], "w");
    if(outfile == NULL){
        printf("\nCould not open file\n");
        return -1;
    }

    AES_set_encrypt_key(key_byte, 128, &expanded);
 
    for (k=0; k<trials; k++){
        if(k%32 == 0)
            printf("Executing trial No.%d\n", k);
        for (n=0; n<BUF_SIZE; n++){
	    len = fread(in, LEN, 1, infile);
	    // prime
	    for (set=0; set<NUMSET; set++){
		for (i=0; i<L1_ASSOC; i++){
		    mem_start[WAY_SIZE*i+set*L1_LINE_SIZE]--;
		}
	    }
#ifdef MSR_EN
        // wrmsr(MSR_EN, 1);
        asm volatile ( "wrmsr": : "c"((uint32_t) MSR_EN),
            "a"(1), "d"(0));
#endif
	    AES_encrypt(in,workarea,&expanded);
#ifdef MSR_EN
        // wrmsr(MSR_EN, 0);
        asm volatile ( "wrmsr": : "c"((uint32_t) MSR_EN),
            "a"(0), "d"(0));
#endif
            // probe
	    for (set=0; set<NUMSET; set++){
		rdtscll(cycles_begin_hi, cycles_begin_low);
		index = set*L1_LINE_SIZE;
		mem_start[index+WAY_SIZE*7] = mem_start[index+WAY_SIZE*7]*3;
		mem_start[index+WAY_SIZE*6] = mem_start[index+WAY_SIZE*6]*3;
		mem_start[index+WAY_SIZE*5] = mem_start[index+WAY_SIZE*5]*3;
		mem_start[index+WAY_SIZE*4] = mem_start[index+WAY_SIZE*4]*3;
		mem_start[index+WAY_SIZE*3] = mem_start[index+WAY_SIZE*3]*3;
		mem_start[index+WAY_SIZE*2] = mem_start[index+WAY_SIZE*2]*3;
		mem_start[index+WAY_SIZE] = mem_start[index+WAY_SIZE]*3;
		mem_start[index] = mem_start[index]*3;
		rdtscll(cycles_end_hi, cycles_end_low);
		t_diff = cycles_end_low - cycles_begin_low;
		*(uint16_t *) (out[n] + 16 + set*2) = t_diff;
	    }
	    for (i = 0;i < 16;++i) {
		out[n][i] = in[i];
	    }
        }
        fwrite(out, NUMSET*2+16, BUF_SIZE, outfile);
    }

    //printf("Print mem_start: ");
    //for (i = 0; i < NUMSET*L1_ASSOC; i++)
    //  printf("%#x", mem_start[i*L1_LINE_SIZE]);
    fclose(outfile);
    fclose(infile);
    return 0;
}

  
