CXX := mpic++

EXE := hemelb

HEMELB_DEBUG_LEVEL := 0
HEMELB_STEERING_LIB := $(or $(HEMELB_STEERING_LIB),basic)

HEMELB_CFLAGS :=
HEMELB_CXXFLAGS := -g -pedantic -Wall -Wextra -Wno-unused-result
HEMELB_DEFS := HEMELB_STEERING_LIB=$(HEMELB_STEERING_LIB)
HEMELB_INCLUDEPATHS :=
HEMELB_LIBPATHS :=

