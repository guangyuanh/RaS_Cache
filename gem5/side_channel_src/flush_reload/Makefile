CC = gcc
LINK_TYPE = -static
MSR_DOMAIN= -D MSR_DOMAIN=0xC0010118
MSR_EN = -D MSR_EN=0xC0010119

flush_reload: flush_reload.c aes_core.c aes.h e_os2.h opensslconf.h
	$(CC) $(LINK_TYPE) -o exe_fr_d flush_reload.c aes_core.c
process_flush_reload: process_flush_reload.c
	$(CC) $(LINK_TYPE) -o exe_process_fr_d process_flush_reload.c

clean:
	rm exe_fr_d process_fr_d
