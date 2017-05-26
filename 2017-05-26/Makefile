# This file is made available under the Creative Commons CC0 1.0
# Universal Public Domain Dedication.
#
# The person who associated a work with this deed has dedicated the
# work to the public domain by waiving all of his or her rights to
# the work worldwide under copyright law, including all related and
# neighboring rights, to the extent allowed by law. You can copy,
# modify, distribute and perform the work, even for commercial
# purposes, all without asking permission.

BIN := simpletm

SRCS := array.h \
        main.c \
        res.c \
        res.h \
        tm.c \
        tm.h

# Language options
CFLAGS += -std=gnu99 -Wall -Wclobbered -O2

# POSIX threads
CFLAGS += -pthread

# Tools
#

RM ?= rm -f

# Automatic variables and rules
#

OBJS :=
OBJS += $(patsubst %.c, %.o, $(filter %.c, $(SRCS)))

.PHONY: all clean mostlyclean

.DEFAULT_GOAL := all

all: $(BIN)

clean: mostlyclean

mostlyclean:
	$(RM) $(BIN)
	$(RM) $(OBJS)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
