CC = gcc
LINK_TYPE = -static

calculation: calculation.c
	$(CC) $(LINK_TYPE) -o calculation calculation.c -lm

evict_time_cov: evict_time_cov.c aes_core.c aes.h 
	$(CC) $(LINK_TYPE) -o evict_time_cov evict_time_cov.c aes_core.c
clean:
	rm -rf evict_time_cov calculation

