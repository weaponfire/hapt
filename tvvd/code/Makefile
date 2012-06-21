#
# Makefile for HAPT
#

#SHELL=/bin/csh

CXX = g++

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

OPT_FLAGS = -O3 -ffast-math

CXX_FLAGS = -Wall -Wno-deprecated $(INCLUDES) $(OPT_FLAGS)

LNK_FLAGS = -fPIC $(OPT_FLAGS)

LIBS =	-lGLee -lglut -lGL -lGLU -lglslKernel

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
