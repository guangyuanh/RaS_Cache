#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#define LINESIZE_BITS 4 // 16 AES entries in a 64-byte line
#define NUM_VALUE (256 >> LINESIZE_BITS)
#define KEY_LENGTH 16

double exec_time[KEY_LENGTH][KEY_LENGTH][NUM_VALUE];
unsigned num_run[KEY_LENGTH][KEY_LENGTH][NUM_VALUE];

void tally(char *response)
{
	uint32_t cur_time = *(uint32_t *)response;
	unsigned i,j,k;
	unsigned char bytes[KEY_LENGTH];
	unsigned char res_xor;
	if (cur_time > 0 && cur_time < 10000000) {
		for (i = 0; i < KEY_LENGTH; i++)
			bytes[i] = *(unsigned char *) (response+4+i);
		for (i = 0; i < KEY_LENGTH; i++) {
			for (j = 0; j < KEY_LENGTH; j++) {
				res_xor = (bytes[i] ^ bytes[j]) >> LINESIZE_BITS;
				exec_time[i][j][res_xor] += cur_time;
				num_run[i][j][res_xor] += 1;
			}
		}
	}
}

int main(int argc, char **argv)
{
	FILE *fin, *fout;
	char *fname;

	unsigned i, j, k;

    unsigned sample_size = 4+KEY_LENGTH;
    char *response = malloc(sample_size);
    unsigned numread = 0;

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
    	for (j = 0; j < KEY_LENGTH; j++)
    		for (k = 0; k < NUM_VALUE; k++) {
    			exec_time[i][j][k] = 0;
    			num_run[i][j][k] = 0;
    		}

    while (!feof(fin)) {
    	fread(response, sample_size, 1, fin);
    	numread++;
    	tally(response);
    }
    printf("numread: %u\n", numread);

    for (i = 0; i < KEY_LENGTH; i++) 
    	for (j = 0; j < KEY_LENGTH; j++) {
    		if (i != j) {
    			for (k = 0; k < NUM_VALUE; k++) {
    				if (num_run[i][j][k] == 0) {
    					printf("No timing measured for bytes %u, %u, value: %u\n", i, j, k);
    					assert(exec_time[i][j][k] == 0);
    				}
    				else {
    					exec_time[i][j][k] = exec_time[i][j][k]/num_run[i][j][k];
    				}
    				fprintf(fout, "%f ", exec_time[i][j][k]);
    			}
    			fprintf(fout, "\n");
    		}
    	}

    fclose(fin);
    fclose(fout);
    return 0;
}
