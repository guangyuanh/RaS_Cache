CC = gcc
LINK_TYPE = -static

clean:
	rm -rf exe_generate_sample exe_process_cache_collision

generate_sample: generate_sample.c aes_core.c aes.h
	$(CC) $(LINK_TYPE) -o exe_generate_sample generate_sample.c aes_core.c
process_cache_collision: process_cache_collision.c
	$(CC) -o exe_process_cache_collision process_cache_collision.c
