#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#define LINESIZE_BITS 0 // 16 AES entries in a 64-byte line
#define NUM_VALUE (256 >> LINESIZE_BITS)
#define KEY_LENGTH 16

double packets;
double ttotal;
double t[16][256];
double tsq[16][256];
long long tnum[16][256];
double u[16][256];
double udev[16][256];
unsigned char n[16];

void tally(double timing)
{
    if(timing <= 0 || timing > 1000000)
        return;
    int j;
    int b;
    for (j = 0;j < 16;++j) {
        b = 255 & (int) n[j];
        ++packets;
        ttotal += timing;
        t[j][b] += timing;
        tsq[j][b] += timing * timing;
        tnum[j][b] += 1;
    }
}

int main(int argc, char** argv)
{
    FILE *fin, *fout;
    char *fname;
    unsigned numread = 0;
    unsigned i, j, k, b;
    char response[48];
    unsigned int timing;
    double taverage;

    fin = fopen(argv[1], "r");
    if (fin == NULL) {
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

    for (i = 0; i < KEY_LENGTH; i++)
        for (j = 0; j < NUM_VALUE; j++) {
            t[i][j] = 0;
            tsq[i][j] = 0;
            tnum[i][j] = 0;
            u[i][j] = 0;
            udev[i][j] = 0;
        }

    while (!feof(fin)) {
        fread(response,48,1,fin);
        numread++;
        for (i = 0; i < KEY_LENGTH; i++)
            n[i]=response[i];
        timing = *(uint64_t *) (response + 40);
        timing -= *(uint64_t *) (response + 32);
        if (timing < 100000) { /* clip tail to reduce noise */
            tally(timing);
        }
    }
    printf("numread: %u\n", numread);

    taverage = ttotal / packets;
    for (j = 0;j < 16;++j)
        for (b = 0;b < 256;++b) {
            u[j][b] = t[j][b] / tnum[j][b];
            udev[j][b] = tsq[j][b] / tnum[j][b];
            udev[j][b] -= u[j][b] * u[j][b];
            udev[j][b] = sqrt(udev[j][b]);
        }
    for (j = 0;j < 16;++j) {
        for (b = 0;b < 256;++b) {
            fprintf(fout, "%.4f ", u[j][b] - taverage);
        }
        fprintf(fout, "\n");
    }
    fclose(fin);
    fclose(fout);
    return 0;
}
