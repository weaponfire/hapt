#
# Makefile for HAPT
#

VCGDIR = $(HOME)/vcglib
VCGGUI = $(VCGDIR)/wrap/gui

LDIR = $(HOME)/lcgtk

GDIR = $(LDIR)/geomTypes
EDIR = $(LDIR)/errHandle
KDIR = $(LDIR)/glslKernel

CUDA_HOME =	/usr/local/cuda
CUDA_SDK = $(CUDA_HOME)/NVIDIA_GPU_Computing_SDK

CXX = c++
CUX = $(CUDA_HOME)/bin/nvcc

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

OPT_FLAGS = -O3 -ffast-math

CXX_FLAGS = -Wall -Wno-deprecated $(INCLUDES) $(OPT_FLAGS)

NVCC_FLAGS = -m64 -Xcompiler ,\"-g\" -gencode arch=compute_20,code=sm_20 --ptxas-options=-v \
	-ftz=true -prec-div=false -prec-sqrt=false -DNVCC \
	-I$(CUDA_HOME)/include \
	-I$(CUDA_SDK)/C/common/inc \
	-I$(CUH)

LNK_FLAGS = -fPIC $(OPT_FLAGS)

LIBS =	-lGLee -lglut -lGL -lGLU -lglslKernel -lcudart

#-----------------------------------------------------------------------------

$(APP): $(OBJS)
	@echo "Linking ..."
	$(CXX) $(LNK_FLAGS) -o $(APP) $(OBJS) $(LIBDIR) $(LIBS)

depend:
	rm -f .depend
	$(CXX) -M $(CXX_FLAGS) $(SRCS) > .depend

$(OBJ)/%.cu_o: $(CU)/%.cu
	@echo "CUDA compiling ..."
	$(CUX) $< -c -o $@ $(NVCC_FLAGS)

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
