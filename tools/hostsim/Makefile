#
# Builds the PC test harness for the emulator port (see host_sim.c).
#
#   make -C tools/hostsim
#   tools/hostsim/run_tests.sh
#

ROOT := ../..
HATARI := $(ROOT)/third_party/hatari/src
ZLIB := $(ROOT)/third_party/zlib
include $(ROOT)/src/port/hatari.mk

CC ?= cc
HOSTCC ?= cc
OUT := build
GEN := $(OUT)/gen

HATARI_INCLUDES = -I $(ROOT)/src/port -I $(HATARI)/includes -I $(HATARI)/debug \
                  -I $(HATARI)/falcon -I $(HATARI)/cpu -I $(HATARI)/cpu/softfloat \
                  -I $(HATARI) -I $(GEN) -I $(ZLIB)
CFLAGS = -O2 -g -DBMATARIST_HOST_LIBC
HATARI_FLAGS = $(CFLAGS) $(HATARI_CFLAGS) $(HATARI_INCLUDES) \
               -include $(ROOT)/src/port/compat/hatari_prefix.h

PORT_SRCS = platform.c menu.c osd.c posix_extra.c $(HATARI_PORT_SRCS)

OBJS := $(addprefix $(OUT)/hatari/,$(HATARI_CORE_SRCS:.c=.o)) \
        $(addprefix $(OUT)/hatari/cpu/,$(HATARI_CPU_SRCS:.c=.o)) \
        $(addprefix $(OUT)/gen/,$(HATARI_CPUGEN_SRCS:.c=.o) $(HATARI_CPUDEFS_SRC:.c=.o)) \
        $(addprefix $(OUT)/hatari/falcon/,$(HATARI_FALCON_SRCS:.c=.o)) \
        $(addprefix $(OUT)/hatari/debug/,$(HATARI_DEBUG_SRCS:.c=.o)) \
        $(addprefix $(OUT)/zlib/,$(ZLIB_SRCS:.c=.o)) \
        $(addprefix $(OUT)/port/,$(PORT_SRCS:.c=.o)) \
        $(OUT)/host_sim.o $(OUT)/live.o

all: $(OUT)/bmatarist_sim

$(OUT)/bmatarist_sim: $(OBJS)
	$(CC) -o $@ $(OBJS) -lm

# The CPU core's generated sources, as Hatari's build makes them
# (src/cpu/CMakeLists.txt).
$(GEN)/build68k: $(HATARI)/cpu/build68k.c $(HATARI)/cpu/writelog.c
	@mkdir -p $(GEN)
	$(HOSTCC) -I $(HATARI)/cpu -I $(HATARI)/includes -I $(ROOT)/src/port -o $@ $^

$(GEN)/cpudefs.c: $(GEN)/build68k $(HATARI)/cpu/table68k
	$(GEN)/build68k < $(HATARI)/cpu/table68k > $@

$(GEN)/gencpu: $(GEN)/cpudefs.c $(HATARI)/cpu/gencpu.c $(HATARI)/cpu/readcpu.c
	$(HOSTCC) -I $(HATARI)/cpu -I $(HATARI)/includes -I $(ROOT)/src/port -o $@ $^

# gencpu writes into the current folder.
$(GEN)/cpustbl.c: $(GEN)/gencpu
	cd $(GEN) && ./gencpu > /dev/null

$(addprefix $(GEN)/,$(filter-out cpustbl.c,$(HATARI_CPUGEN_SRCS))) $(GEN)/cputbl.h: $(GEN)/cpustbl.c

$(OUT)/gen/%.o: $(GEN)/%.c
	@$(CC) $(HATARI_FLAGS) -c -o $@ $<

$(OUT)/hatari/%.o: $(HATARI)/%.c $(GEN)/cputbl.h
	@mkdir -p $(dir $@)
	@$(CC) $(HATARI_FLAGS) -c -o $@ $<

$(OUT)/zlib/%.o: $(ZLIB)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(ZLIB_CFLAGS) -c -o $@ $<

$(OUT)/port/%.o: $(ROOT)/src/port/%.c $(ROOT)/src/port/bmatarist.h \
                 $(ROOT)/src/port/osd.h $(ROOT)/src/host.h $(GEN)/cputbl.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -std=gnu11 $(HATARI_INCLUDES) -include $(ROOT)/src/port/compat/hatari_prefix.h \
	      -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -c -o $@ $<

$(OUT)/host_sim.o: host_sim.c live.h $(ROOT)/src/host.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

$(OUT)/live.o: live.c live.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wall -Wextra -DLIVE_HTML=\"$(abspath live.html)\" -c -o $@ $<

clean:
	rm -rf $(OUT)

.PHONY: all clean
