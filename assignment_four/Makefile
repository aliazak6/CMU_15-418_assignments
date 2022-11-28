#TARGET = wsp-mpi
#OBJS += wsp-mpi.o
#OBJS += main.o
#
#CC = mpicc
#CFLAGS += -Wall -Wextra -g -O3 -DNDEBUG
#CFLAGS += -MMD -MP
#LDFLAGS += $(LIBS)
#
#all: $(TARGET)
#
#$(TARGET):	$(OBJS)
#	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
#
#%.o: %.c
#	$(CC) $(CFLAGS) -c -o $@ $<
#
#DEPS = $(OBJS:%.o=%.d)
#-include $(DEPS)
#
#clean:
#	rm -f ./wsp-mpi

CXXFLAGS += -Wall -Wextra -pthread -fopenmp -g -O3 -DNDEBUG
HELLOFLAGS = -Wall -Wextra -g -O3 -DNDEBUG
CC = mpicc
.phony: all release

all: release

release: wsp-mpi.c
	$(CC) $? -o wsp-mpi $(CXXFLAGS)

clean:
	rm -f ./wsp-mpi

