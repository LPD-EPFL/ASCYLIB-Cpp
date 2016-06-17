
ifeq ($(GC),0)
        CPPFLAGS += -DGC=0
else
        CPPFLAGS += -DGC=1
endif

VERSION ?= O3
ifeq ($(VERSION),DEBUG)
        CPPFLAGS += -g -DDEBUG -O0
endif
ifeq ($(VERSION),SYMBOL)
        CPPFLAGS  += -O3 -g
endif
ifeq ($(VERSION),O0)
        CPPFLAGS  += -O0
endif
ifeq ($(VERSION),O1)
        CPPFLAGS  += -O1
endif
ifeq ($(VERSION),O2)
        CPPFLAGS  += -O2
endif
ifeq ($(VERSION),O3)
        CPPFLAGS  += -O3
endif

ifeq ($(SET_CPU),0)
        CPPFLAGS += -DNO_SET_CPU
endif

ifeq ($(RO_FAIL),0)
        CPPFLAGS += -DRO_FAIL=0
        SUFFIX = _no_ro
else
        CPPFLAGS += -DRO_FAIL=1
endif

ifeq ($(INIT), all)
        CPPFLAGS += -DINITIALIZE_FROM_ONE=0
else
        CPPFLAGS += -DINITIALIZE_FROM_ONE=1
endif

# Compile with global lock
ifeq ($(GRANULARITY),GLOBAL_LOCK)
        CPPFLAGS += -DLL_GLOBAL_LOCK
        BIN_SUFFIX = _gl
endif
ifeq ($(G),GL)
        CPPFLAGS += -DLL_GLOBAL_LOCK
        BIN_SUFFIX = _gl
endif

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(LOCK),)
        LOCK ?= TAS
endif

ifeq ($(STM),SEQUENTIAL)
        CPPFLAGS += -DSEQUENTIAL
endif
ifeq ($(STM),LOCKFREE)
        CPPFLAGS += -DLOCKFREE
endif

ifeq ($(CACHE),1)
        SSMEM_SUFFIX=_manual
endif

#############################
# Platform dependent settings
#############################
#
# GCC thread-local storage requires "significant 
# support from the linker (ld), dynamic linker
# (ld.so), and system libraries (libc.so and libpthread.so), so it is
# not available everywhere." source: GCC-doc.
#
# pthread_spinlock is replaced by pthread_mutex 
# on MacOS X, as it might not be supported. 
# Comment LOCK = MUTEX below to enable.

ifndef OS_NAME
        OS_NAME = $(shell uname -s)
endif

ifeq ($(OS_NAME), Darwin)
        OS = MacOS
        DEFINES += -UTLS
        LOCK = MUTEX
endif

ifeq ($(OS_NAME), Linux)
        OS = Linux
        DEFINES += -DTLS
endif

ifeq ($(OS_NAME), SunOS)
        OS = Solaris
        CC = $(SOLARIS_CC1)
        DEFINES += -DTLS
endif

ifndef STM
        CPPFLAGS += -D$(LOCK)
endif

PLATFORM_NUMA := 0
ifeq ($(shell dmesg | grep "No NUMA" | wc -l), 0)
        ifneq ($(shell dmesg | grep "NUMA" | wc -l), 0)
                PLATFORM_NUMA := 1
        endif
endif

ifneq ($(PLATFORM_KNOWN), 1)
        CPPFLAGS += -DDEFAULT
        CORE_NUM ?= $(shell nproc)
        CORE_SPEED_KHz := $(shell cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq 2>/dev/null)
        ifeq ($(CORE_SPEED_KHz),)
                $(info ******* Unable to detect MAX CPU freq, using current CPU freq)
                CORE_SPEED_MHz := $(shell lscpu | grep "CPU MHz" | cut -d: -f2 | tr -d " ")
                CORE_SPEED_KHz := $(shell echo "${CORE_SPEED_MHz}*1000" | bc -l)
        endif
        FREQ_GHZ := $(shell echo "scale=3; ${CORE_SPEED_KHz}/1000000" | bc -l)
        $(info ********************************** Using as a default number of cores: $(CORE_NUM) on 1 socket)
        $(info ********************************** Using as a default frequency      : $(FREQ_GHZ) GHz)
        $(info ********************************** If incorrect, create a manual entry in comp_flags.mk)
        CPPFLAGS += -DCORE_NUM=${CORE_NUM}
        CPPFLAGS += -DFREQ_GHZ=${FREQ_GHZ}
endif
 
#################################
# Architecture dependent settings
#################################

ifndef ARCH
        ARCH_NAME = $(shell uname -m)
endif

ifeq ($(ARCH_NAME), i386)
        ARCH = x86
        CPPFLAGS += -m32
        SSPFD = -lsspfd_x86
        LBLIBS += -lssmem_x86$(SSMEM_SUFFIX)
endif

ifeq ($(ARCH_NAME), i686)
        ARCH = x86
        CPPFLAGS += -m32
        SSPFD = -lsspfd_x86
        LDLIBS += -lssmem_x86$(SSMEM_SUFFIX)
endif

ifeq ($(ARCH_NAME), x86_64)
        ARCH = x86_64
        CPPFLAGS += -m64
        SSPFD = -lsspfd_x86_64
        LDLIBS += -lssmem_x86_64$(SSMEM_SUFFIX)
endif

ifeq ($(ARCH_NAME), sun4v)
        ARCH = sparc64
        CPPFLAGS += -DSPARC=1 -DINLINED=1 -m64
        LDFLAGS += -lrt
        SSPFD = -lsspfd_sparc64
        LDLIBS += -lssmem_sparc64$(SSMEM_SUFFIX)
endif

ifeq ($(ARCH_NAME), tile)
        LDLIBS += -lssmem_tile$(SSMEM_SUFFIX)
        SSPFD = -lsspfd_tile
endif

ifeq ($(PLATFORM_NUMA),1)
        LDLIBS += -lnuma
endif

