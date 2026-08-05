# Test Makefile for GW-BASIC C Port
# Builds test_gwbasic.exe linking all module objects except gwmain.o

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I../src -I.
LDFLAGS = -mconsole -lwinmm

# Source files (matching original GW-BASIC ASM modules, excluding gwmain.c)
TEST_SRCS = \
	../src/advgrp.c \
	../src/biboot.c \
	../src/bimisc.c \
	../src/biprtu.c \
	../src/biptrg.c \
	../src/bistrs.c \
	../src/call86.c \
	../src/dskcom.c \
	../src/fiveo.c \
	../src/gengrp.c \
	../src/gio86.c \
	../src/giocas.c \
	../src/giocom.c \
	../src/giocon.c \
	../src/giodsk.c \
	../src/giokyb.c \
	../src/giolpt.c \
	../src/gioiscn.c \
	../src/giotbl.c \
	../src/gwdata.c \
	../src/gweval.c \
	../src/gwinit.c \
	../src/gwlist.c \
	../src/gwram.c \
	../src/gwsts.c \
	../src/ibmres.c \
	../src/itsa86.c \
	../src/kanj86.c \
	../src/maclng.c \
	../src/math1.c \
	../src/math2.c \
	../src/next86.c \
	../src/oem.c \
	../src/oemcbk.c \
	../src/oemev.c \
	../src/oemsnd.c \
	../src/scndrv.c \
	../src/scnedt.c \
	../tests/test_gwdata.c \
	../tests/test_gwram.c \
	../tests/test_gwinit.c \
	../tests/test_gwsts_gweval.c \
	../tests/test_giocon.c \
	../tests/test_giokyb_gioscn.c \
	../tests/test_gwlist_bistrs.c \
	../tests/test_biprtu_biptrg.c \
	../tests/test_dskcom.c \
	../tests/test_math.c \
	../tests/test_advgrp.c \
	../tests/test_call86.c \
	../tests/test_gweval_helpers.c \
	../tests/test_itsa86.c \
	../tests/test_next86.c \
	../tests/test_kanj86.c \
	../tests/test_maclng.c \
	../tests/test_giocas.c

TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_RUNNER_OBJ = test_runner.o
TARGET = test_gwbasic.exe

all: $(TARGET)

$(TARGET): $(TEST_OBJS) $(TEST_RUNNER_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TEST_OBJS) $(TEST_RUNNER_OBJ) $(TARGET)

.PHONY: all test clean