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

void tally(char * response, int numsets)
{
    int j;
    int b;
    int set;
    double timing;
    for (set = 0; set < numsets; set++) {
        timing = *(uint64_t *)(response+set*8);
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
                //printf("%.3f ", u[j][b][set] - taverage[set]);
                fprintf(fout, "%.3f ", u[j][b][set]);
            fprintf(fout, "\n");
        }
    }
    //fflush(stdout);
}

int main(int argc, char** argv)
{
    FILE *fin, *fout;
    int numread=0;
    int count;
    int numsets = 1024*4/64;
    int sample_size = 16 + 8*numsets;
    char * response = malloc(sample_size);
    unsigned int timing;
    int i,j;
    char *fname;

    fin = fopen(argv[1], "r");
    if ( fin == NULL) {
        printf("\ncould not open file\n");
        return -1;
    }

    fname = (char *) malloc(100);
    sprintf(fname, "%s.csv", argv[1]);
    fout = fopen(fname, "w");
    if (fout == NULL) {
        printf("\ncould not open file\n");
        return -1;
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

    while (!feof(fin)){
        fread(response,sample_size,1,fin);
        numread++;
	for (count=0;count<16;++count)
            n[count]=response[count];
        tally(response+16, numsets);
    }
    printf("numread: %d\n", numread);
    printpatterns(fout, numsets);

    fclose(fin);
    fclose(fout);
}
