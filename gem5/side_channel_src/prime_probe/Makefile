CC = gcc
LINK_TYPE = -static
MSR_EN = -D MSR_EN=0xC0010119

prime_probe: prime_probe.c aes_core.c aes.h e_os2.h opensslconf.h
	$(CC) $(LINK_TYPE) -o exe_prime_probe prime_probe.c aes_core.c
process_prime_probe: process_prime_probe.c
	$(CC) $(LINK_TYPE) -o exe_process_prime_probe process_prime_probe.c

clean:
	rm prime_probe_cov exe_process_prime_probe
