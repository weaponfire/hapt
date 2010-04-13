#
# Makefile for HAPT
#

#SHELL=/bin/csh

CXX = g++
#CXX = icpc

VCGDIR = $(HOME)/vcglib
VCGGUI = $(VCGDIR)/wrap/gui

LDIR = $(HOME)/lcgtk

GDIR = $(LDIR)/geomTypes
EDIR = $(LDIR)/errHandle
KDIR = $(LDIR)/glslKernel

LIBDIR = -L$(LDIR)/lib

INCLUDES = -I$(LDIR) -I$(GDIR) -I$(EDIR) -I$(KDIR) -I$(VCGDIR)

OBJS =	appVol.o ptGLut.o tfGLut.o haptVol.o hapt.o \
	$(VCGGUI)/trackmode.o $(VCGGUI)/trackball.o

SRCS = appVol.cc glslKernel.cc ptGLut.cc tfGLut.cc haptVol.cc hapt.cc \
	$(VCGGUI)/trackmode.cpp $(VCGGUI)/trackball.cpp

APP = hapt

DEBUG_FLAGS = #-g
OPT_FLAGS = -O3 -ffast-math
ICPC_FLAGS = -D_GLIBCXX_GTHREAD_USE_WEAK=0 -pthread

CXX_FLAGS = -Wall -Wno-deprecated $(DEBUG_FLAGS) $(INCLUDES) $(OPT_FLAGS) $(ICPC_FLAGS)

LNK_FLAGS = -fPIC $(OPT_FLAGS) $(ICPC_FLAGS)

LIBS =	-lGLee -lglut -lGL -lglslKernel

#-----------------------------------------------------------------------------

$(APP): $(OBJS)
	@echo "Linking ..."
	$(CXX) $(LNK_FLAGS) -o $(APP) $(OBJS) $(LIBDIR) $(LIBS)

depend:
	rm -f .depend
	$(CXX) -M $(CXX_FLAGS) $(SRCS) > .depend

%.o: %.cc
	@echo "Compiling ..."
	$(CXX) $(CXX_FLAGS) -c $< -o $@

%.o: %.cpp
	@echo "Compiling ..."
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -f *.o *~ \#* $(APP) .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
