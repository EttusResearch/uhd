INCLUDES = usrp3_regs.h uhd_dump.h

BINARIES = chdr_log

OBJECTS = uhd_dump.o 

CFLAGS = -g -O0 -Wall

LDFLAGS = -lpcap -lm 

CC = cc

.PHONY: all
all: $(BINARIES)

chdr_log: uhd_dump.o chdr_log.o $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ uhd_dump.o chdr_log.o  $(LDFLAGS)



clean:
	rm $(BINARIES)  $(BINARIES:%=%.o) $(OBJECTS)

