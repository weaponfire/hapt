#
# Makefile for HAPT
#

#SHELL=/bin/csh

CXX = g++
#CXX = icpc
CUX = nvcc

VCGDIR = $(HOME)/vcglib
VCGGUI = $(VCGDIR)/wrap/gui

LDIR = $(HOME)/lcgtk

GDIR = $(LDIR)/geomTypes
EDIR = $(LDIR)/errHandle
KDIR = $(LDIR)/glslKernel

CUDA_HOME =	/usr/local/cuda
CUDA_SDK =	$(CUDA_HOME)/SDK

CUH =	cudah

LIBDIR = -L$(LDIR)/lib -L$(CUDA_HOME)/lib64 -L$(CUDA_SDK)/C/lib

SRC =	src
OBJ =	obj
CU =	cudac

INCLUDES =	-I$(LDIR) -I$(GDIR) -I$(EDIR) -I$(KDIR) \
		-Iinclude -I$(CUH) -I$(VCGDIR)

OBJS =	$(OBJ)/hapt.o $(OBJ)/haptVol.o $(OBJ)/appVol.o \
	$(OBJ)/ptGLut.o $(OBJ)/tfGLut.o $(OBJ)/isoGLut.o \
	$(OBJ)/centroid.cu_o \
	$(VCGGUI)/trackmode.o $(VCGGUI)/trackball.o

SRCS =	$(SRC)/hapt.cc $(SRC)/haptVol.cc $(SRC)/appVol.cc \
	$(SRC)/ptGLut.cc $(SRC)/tfGLut.cc $(SRC)/isoGLut.cc \
	$(CU)/centroid.cu \
	$(VCGGUI)/trackmode.cpp $(VCGGUI)/trackball.cpp

APP = hapt

DEBUG_FLAGS = #-g
OPT_FLAGS = -O3 -ffast-math
ICPC_FLAGS = -D_GLIBCXX_GTHREAD_USE_WEAK=0 -pthread

CXX_FLAGS = -Wall -Wno-deprecated $(DEBUG_FLAGS) $(INCLUDES) $(OPT_FLAGS) $(ICPC_FLAGS)

NVCC_FLAGS =	-O3 --compiler-options='-fPIC' \
		--compiler-bindir=/usr/bin/gcc-4.3 \
		-I$(CUDA_HOME)/include \
		-I$(CUDA_SDK)/C/common/inc \
		-I$(CUH)

LNK_FLAGS = -fPIC $(OPT_FLAGS) $(ICPC_FLAGS)

LIBS =	-lGLee -lglut -lGL -lglslKernel \
	-lcuda -lcudart -lcutil

#-----------------------------------------------------------------------------

$(APP): $(OBJS)
	@echo "Linking ..."
	$(CXX) $(LNK_FLAGS) -o $(APP) $(OBJS) $(LIBDIR) $(LIBS)

depend:
	rm -f .depend
	$(CXX) -M $(CXX_FLAGS) $(SRCS) > .depend

$(OBJ)/%.cu_o: $(CU)/%.cu
	@echo "CUDA compiling ..."
	$(CUX) $(NVCC_FLAGS)  -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.cc
	@echo "Compiling ..."
	$(CXX) $(CXX_FLAGS) -c $< -o $@

$(VCGGUI)/%.o: $(VCGGUI)/%.cpp
	@echo "Compiling ..."
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)/* $(SRC)/*~ $(CU)/*~ $(APP) .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
