CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I./src
DEBUG_CFLAGS = -std=c11 -Wall -Wextra -O2 -I./src -DDEBUG
LDFLAGS = -mconsole -lwinmm

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
TEST_SRCS = tests/test_runner.c tests/test_gwdata.c tests/test_gwram.c tests/test_gwinit.c tests/test_gwsts_gweval.c tests/test_giocon.c tests/test_giokyb_gioscn.c tests/test_gwlist_bistrs.c tests/test_biprtu_biptrg.c tests/test_gweval_helpers.c tests/test_advgrp.c tests/test_call86.c tests/test_gweval_helpers.c tests/test_itsa86.c tests/test_next86.c tests/test_kanj86.c tests/test_maclng.c tests/test_giocas.c tests/test_gio86.c tests/test_giotbl.c tests/test_oem.c tests/test_biboot.c tests/test_ibmres.c tests/test_oemev.c tests/test_oemsnd.c tests/test_scnedt.c tests/test_oemcbk.c tests/test_gengrp.c tests/test_giodsk.c tests/test_giolpt.c tests/test_giocom.c tests/test_gwlist.c tests/test_gwsts.c tests/test_bistrs.c tests/test_bimisc.c tests/test_biptrg.c tests/test_fiveo_ext.c tests/test_scndrv.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_TARGET = test_gwbasic.exe

all: $(TARGET)

debug: $(SRCS)
	$(CC) $(DEBUG_CFLAGS) -o $(TARGET) $^ $(LDFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Test target - delegates to tests/ directory
test:
	$(MAKE) -C tests test

%.o: %.c src/gwbasic.h src/dskcom.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C tests clean

.PHONY: all clean test