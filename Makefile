#Build dir
BUILD_DIR = ./
TEST_DIR  := ./
#Source dir
SRC_DIR = ./
# RISC-V 64-bit
#spike
#CCrisc = riscv64-unknown-elf-gcc 
#CXXrisc = riscv64-unknown-elf-g++

#orangepi gcc
CCrisc = gcc
CXXrisc = g++

#orangepi clang
#CCrisc = clang-19
#CXXrisc = clang-19++

#EPAC
#CCrisc = clang
#CXXrisc = riscv64-linux-gnu-g++
#ARCH = rv64imacv

ARCH = rv64gcv

# Configuration
DEBUG =	0 # Enable debug prints
FLOAT_TYPE = USE_FLOAT #USE_FLOAT or USE_DOUBLE
RVV0_7 = USE_RVV0_7 # Enable RISC-V Vector Extension 0.7 support, set to 1 to enable
rvv = 1 # Enable RISC-V Vector Extension (RVV) support, set to 1 to enable

#gcc orangepi
CFLAGSrisc = -Wall -Wextra -O3 -std=gnu99 -Drvv=$(rvv) -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE) -march=$(ARCH) -lm
CXXFLAGSrisc = -Wall -Wextra -O3 -std=c++11 -Drvv=$(rvv) -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE) -march=$(ARCH) -Isrc

#clang orangepi
#CFLAGSrisc = -Wall -Wextra -O3 -std=gnu99 -Rpass=loop-vectorize -Drvv=$(rvv) -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE) -march=$(ARCH)
#CXXFLAGSrisc = -Wall -Wextra -O3 -std=c++11 -Rpass=loop-vectorize -Drvv=$(rvv) -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE) -march=$(ARCH) -Isrc

#clang EPAC
#CFLAGSrisc = -Wall -Wextra -O3 -std=gnu99 -march=rv64g -mepi -lm -g $(NO_AUTOVEC) -mcpu=avispado $(SDV_TRACE_INCL)  -fno-vectorize  -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE) -D$(RVV0_7) -Drvv=$(rvv)
#CXXFLAGSrisc = -Wall -Wextra -O3 -std=c++11 -Drvv=$(rvv) -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE) -D$(RVV0_7) -march=$(ARCH) -Isrc

TARGET_RVV = $(BUILD_DIR)/vec_add
LDFLAGS =
LDLIBS    := -lgtest -lgtest_main -pthread

# x86
CCx86 = gcc
CXX = g++
CFLAGSx86 = -Wall -Wextra -g -std=gnu99 -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE)
CXXFLAGS = -Wall -Wextra -g -std=c++11 -Isrc -DDEBUG=$(DEBUG) -D$(FLOAT_TYPE)
TARGET_X86 = $(BUILD_DIR)/spmv_x86

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS_RISCV64 = $(SRCS:.c=.risc.o)
OBJS_x86 = $(SRCS:.c=.x86.o)

TEST_FILES := $(wildcard $(TEST_DIR)/*.cpp)
OBJ_FOR_TEST := $(filter-out ./src/main.x86.o, $(OBJS_x86))
TEST_EXE=$(TEST_FILES:.cpp=)

.PHONY: all clean

riscv64: $(TARGET_RVV)
x86: $(TARGET_X86)

test: $(TEST_EXE)

all: $(TARGET_x86) $(TARGET_RVV)

$(TARGET_X86): $(OBJS_x86)
	$(CCx86) $(LDFLAGS) $^ -o $@

$(TARGET_RVV): $(OBJS_RISCV64)
	$(CCrisc) $(LDFLAGS) $^ -o $@

%.risc.o: %.c
	$(CCrisc)  $(CFLAGSrisc) -c $< -o $@

%.x86.o: %.c
	$(CCx86) $(CFLAGSx86) -c $< -o $@

clean:
	rm -f $(OBJS_x86) $(OBJS_RISCV64) $(TARGET_X86) $(TARGET_RVV)
	rm -f $(TEST_OBJ_FILES) $(TEST_EXE)

$(TEST_DIR)/%.test.o: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_DIR)/%: $(TEST_DIR)/%.test.o $(OBJ_FOR_TEST)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)s