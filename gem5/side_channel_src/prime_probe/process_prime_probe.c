// Usage: ./exe_process_prime_probe timing_file #set (none)_or_1_or_2
// third argument:
//   none (no print raw data),
//   1 (print consecutive lines)
//   2 (print empty line between each trial)

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

double * packets;
double * ttotal;
double* t[16][256];
//double* tsq[16][256];
long long * tnum[16][256];
double * u[16][256];
//double * udev[16][256];
unsigned char n[16];
int print_raw;
FILE * fset;
FILE * fraw;

void tally(char * response, int numsets)
{
    int j;
    int b;
    int set;
    double timing;
    for (set = 0; set < numsets; set++) {
        timing = *(uint16_t *)(response+set*2);
        if (print_raw) {
            fprintf(fraw, "%u", *(uint16_t *)(response+set*2));
            if (set%8 == 7)
                fprintf(fraw, "\n");
            else
                fprintf(fraw, ",");
        }
        if (timing < 100000) {
            for (j = 0;j < 16;++j) {
                b = 255 & (int) n[j];
                ++packets[set];
                ttotal[set] += timing;
                t[j][b][set] += timing;
                // tsq[j][b] += timing * timing;
                tnum[j][b][set] += 1;
            }
        }
    }
    if (print_raw == 2)
        fprintf(fraw, "\n");
}


void printpatterns(FILE * fout, int numsets)
{
    int j;
    int b;
    int set;
    double *taverage = malloc(numsets*sizeof(double));
    for (set = 0; set < numsets; set++) {
        taverage[set] = ttotal[set] / packets[set];
        for (j = 0;j < 16;++j) {
            for (b = 0;b < 256;++b) {
                u[j][b][set] = t[j][b][set] / tnum[j][b][set];
                // udev[j][b] = tsq[j][b] / tnum[j][b];
                // udev[j][b] -= u[j][b] * u[j][b];
                // udev[j][b] = sqrt(udev[j][b]);
            }
        }
    }
    //for (set = 0; set < numsets; set++)
    //    printf("Total counts set[%d]: %f\n", set, packets[set]);
    //for (set = 0; set < numsets; set++)
    //    printf("%f ", t[0][0][set]);//  - taverage[set]);
    //printf("\n");
    //for (set = 0; set < numsets; set++)
    //    printf("%lld ", tnum[0][0][set]);//  - taverage[set]);
    //printf("\n");
    for (j = 0;j < 16;++j) {
        for (b = 0;b < 256;++b) {
            for (set = 0; set < numsets; set++)
                fprintf(fout, "%.3f ", u[j][b][set] - taverage[set]);
            fprintf(fout, "\n");
        }
    }
}

int main(int argc, char** argv)
{
    FILE * fin = 0;
    FILE * fout = 0;
    int numread=0;
    int count;
    int numsets = atoi(argv[2]);
    int sample_size = 16 + 2*numsets;
    char * response = malloc(sample_size);
    unsigned int timing;
    int i,j;
    char * fname;
    const unsigned char key_byte[16]={0x0f, 0x1e, 0xdb, 0x65, 0xe6, 0xd1, 0x03, 0x5e, 0xfa, 0x94, 0x1f, 0x0c, 0x4b, 0x41, 0xff, 0xbb};

    fin = fopen(argv[1], "r");
    if ( fin == NULL) {
        printf("\ncould not open file\n");
        return -1;
    }

    fname = (char *) malloc(100);
    sprintf(fname, "%s_noavg.csv", argv[1]);
    fout = fopen(fname, "w");
    if (fout == NULL) {
        printf("\ncould not open fout\n");
        return -1;
    }

    printf("argc: %u\n", argc);
    if (argc > 3)
        print_raw = atoi(argv[3]);
    else
        print_raw = 0;
    if (print_raw) {
        sprintf(fname, "%s_set.csv", argv[1]);
        fset = fopen(fname, "w");
        sprintf(fname, "%s_raw.csv", argv[1]);
        fraw = fopen(fname, "w");
    }

    for (i = 0; i < 16; i++){
        for ( j = 0; j < 256; j++) {
            t[i][j] = malloc(numsets*sizeof(double));
            tnum[i][j] = malloc(numsets*sizeof(double));
            u[i][j] = malloc(numsets*sizeof(double));
	}
    }

    ttotal = malloc(numsets*sizeof(double));
    packets = malloc(numsets*sizeof(double));

    fread(response,sample_size,1,fin);
    while (!feof(fin)){
        numread++;
        for (count=0;count<16;++count) {
            n[count]=response[count];
            if (print_raw)
                fprintf(fset, "%u,", (n[count]^key_byte[count]) >> 4);
        }
        if (print_raw)
            fprintf(fset, "\n");
        tally(response+16, numsets);
        fread(response,sample_size,1,fin);
    }
    printf("numread: %d\n", numread);
    printpatterns(fout, numsets);
    fclose(fin);
    fclose(fout);
    if (print_raw) {
        fclose(fset);
        fclose(fraw);
    }
}
