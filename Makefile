CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I./src
LDFLAGS = -mconsole

# Source files (matching original GW-BASIC ASM modules)
SRCS = \
	src/advgrp.c \
	src/biboot.c \
	src/bimisc.c \
	src/biprtu.c \
	src/biptrg.c \
	src/bistrs.c \
	src/call86.c \
	src/dskcom.c \
	src/fiveo.c \
	src/gengrp.c \
	src/gio86.c \
	src/giocas.c \
	src/giocom.c \
	src/giocon.c \
	src/giodsk.c \
	src/giokyb.c \
	src/giolpt.c \
	src/gioiscn.c \
	src/giotbl.c \
	src/gwdata.c \
	src/gweval.c \
	src/gwinit.c \
	src/gwlist.c \
	src/gwmain.c \
	src/gwram.c \
	src/gwsts.c \
	src/ibmres.c \
	src/itsa86.c \
	src/kanj86.c \
	src/maclng.c \
	src/math1.c \
	src/math2.c \
	src/next86.c \
	src/oem.c \
	src/oemcbk.c \
	src/oemev.c \
	src/oemsnd.c \
	src/scndrv.c \
	src/scnedt.c

OBJS = $(SRCS:.c=.o)
TARGET = gwbasic.exe

# Test infrastructure
TEST_SRCS = tests/test_runner.c tests/test_gwdata.c tests/test_gwram.c tests/test_gwinit.c tests/test_gwsts_gweval.c tests/test_giocon.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_TARGET = test_gwbasic.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Test target - delegates to tests/ directory
test:
	$(MAKE) -C tests test

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C tests clean

.PHONY: all clean test