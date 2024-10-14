#include <stdio.h>
#include <stdlib.h>
#include "aes.h"
#include <time.h>
#include <inttypes.h>
#define KEY_LENGTH 16
#define L1_LINE_SIZE 64
#define L1_CACHE_SIZE 0x8000
#define BUF_SIZE 4096

typedef struct {
  uint32_t time;
  unsigned char value[KEY_LENGTH];
} timing_pair;

int64_t scratch = 0;
char * mem_start;
FILE *infile;

void l1_cache_evict(void);
int64_t timestamp(void);
int timing_sample(AES_KEY *key, timing_pair * data);

int main(int argc, char **argv)
{
    AES_KEY expanded;
    const unsigned char key_byte[KEY_LENGTH]={0x0f, 0x1e, 0xdb, 0x65, 0xe6, 0xd1, 0x03, 0x5e, 0xfa, 0x94, 0x1f, 0x0c, 0x4b, 0x41, 0xff, 0xbb};
//   const unsigned char key_byte[KEY_LENGTH]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    int i,n;
    FILE *out = 0;
    unsigned char plaintext[16];
    unsigned char ciphertext[16];
    unsigned trials = atoi(argv[2]);
    mem_start = malloc(L1_CACHE_SIZE);
    timing_pair * buffer = malloc(BUF_SIZE * sizeof(timing_pair));

    infile = fopen("random_file_large", "r");
    if(infile == NULL){
        printf("\ncould not open file\n");
        return -1;
    }
    out = fopen(argv[1], "w");
    if(out == NULL){
      printf("\nCould not open file\n");
      return -1;
    }
    // seed = atoi(argv[1]);
    // srandom(seed);
    srandom(100);
    AES_set_encrypt_key(key_byte, 128, &expanded);

    for (i=0; i < trials; i++) {
      printf("%u/%u runs finished\n", i*BUF_SIZE, trials*BUF_SIZE);
      for (n=0; n < BUF_SIZE; n++){
        l1_cache_evict();
        timing_sample(&expanded, buffer+n);
        // printf("encryption time is: %d\n", buffer[n].time);
      }
      fwrite(buffer, sizeof(timing_pair), BUF_SIZE, out);
    }
    fclose(out);
    return 0;
}

void l1_cache_evict()
{
  int i;
  for(i = 0; i < L1_CACHE_SIZE; i += L1_LINE_SIZE)
    mem_start[i]++;
}
  
/*
 * Get accurate cycle count from processor.
 */
 int64_t timestamp()
{
  int64_t tick;		
  asm volatile ("rdtsc" : "=A"(tick));		
} 

int timing_sample(AES_KEY *key, timing_pair * data)
{  

  int i;
  int64_t timing = 0;
  unsigned char plaintext[16];

  //for (i = 0;i < 16;++i) 
  //  plaintext[i] = random();  
  unsigned len = fread(plaintext, KEY_LENGTH, 1, infile);

  timing = timestamp();
  AES_encrypt(plaintext, data->value, key);
  timing = timestamp() - timing;

  for (i = 0;i < 16;++i) 
    scratch += data->value[i];
  for (i = 0;i < 16;++i) 
    data->value[i] = plaintext[i];
  data->time = timing;

  return scratch;
}
