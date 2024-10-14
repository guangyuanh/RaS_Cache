#include <stdio.h>
#include <stdlib.h>
#include "aes.h"
#include <time.h>
#include <inttypes.h>

#define KEY_LENGTH 16
// number of block encryption
#define BLOCK_NUM 0x80000
#define LEN 16

#define L1_CACHE_SIZE 0x80000
#define L1_ASSOC 8

int way_size = L1_CACHE_SIZE/L1_ASSOC;
char in[LEN];
char out[48];
unsigned char scrambledzero[16];
unsigned char zero[16];
AES_KEY expanded; 

char* mem_start;

uint64_t timestamp(void);

void timing_sample(char out[48],char in[],int len);

void cache_evict(){
  int i;
  for (i=0; i<L1_ASSOC; i++)
      mem_start[way_size*i]++;
}

int main(int argv, char **argc)
{
   const unsigned char key_byte[KEY_LENGTH]={0x0f, 0x1e, 0xdb, 0x65, 0xe6, 0xd1, 0x03, 0x5e, 0xfa, 0x94, 0x1f, 0x0c, 0x4b, 0x41, 0xff, 0xbb};
   // const unsigned char key_byte[KEY_LENGTH]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

   mem_start = malloc(L1_CACHE_SIZE);

   int len;

   FILE* input = fopen("random_file_large", "r");
   FILE* output = fopen(argc[1], "w");

   int n;

   printf("%c", key_byte[0]);

   AES_set_encrypt_key(key_byte, 128, &expanded);
   AES_encrypt(zero, scrambledzero, &expanded);
   
   for (n=0; n<BLOCK_NUM; n++)
   {
       if (n % 4096 == 4095)
           printf("Executed runs: %u\n", n);
       len = fread(in, LEN, 1, input);
       cache_evict();

       timing_sample(out,in,LEN);
  
       fwrite(out,48, 1, output);
   }
   fclose(output);
   fclose(input);
   return 0;
}

  
/*
 * Get accurate cycle count from processor.
 */
uint64_t timestamp()
{
  uint64_t tick;		
//  asm volatile ("rpcc %0" : "=r"(tick));
  asm volatile ("rdtsc" : "=A"(tick));
  return tick;	
} 

void timing_sample(char out[48], char in[], int len)
{  
    unsigned char workarea[len * 3];
    int i;
    for (i = 0;i < 48;++i)
        out[i] = 0;
    for (i = 0;i < 16;++i)
        out[i] = in[i];
    if (len < 16)
        return;
    for (i = 16;i < len;++i)
        workarea[i] = in[i];
    *(uint64_t *) (out + 32) = timestamp();
    AES_encrypt(in,workarea,&expanded);
    *(uint64_t *) (out + 40) = timestamp();
    /* a real server would now check AES-based authenticator, */
    /* process legitimate packets, and generate useful output */
    for (i = 0;i < 16;++i)
        out[16 + i] = scrambledzero[i];
}
