CC = gcc
CC44 = gcc-4.4
MSR_EN = -D MSR_EN=0xC0010119
DEF_SECRET = -D SECRET=30
FLAGS = -static -std=c99 $(DEF_SECRET)
FLAGS44 = -static -std=c99 -msse2 $(DEF_SECRET)

spectre: spectre.c
	$(CC) $(FLAGS) spectre.c -o secret$(SECRET)_exe
pp: pp_spectre.c
	$(CC) $(FLAGS) pp_spectre.c -o pp_secret$(SECRET)_exe
hello:   hello.c
	$(CC) $(FLAGS) hello.c -o hello_static

clean:
	rm *_exe hello_static
