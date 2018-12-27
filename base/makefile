include	../Inc.mk

OBJS = trpc_comm_func.o trpc_exception.o trpc_url_param_map.o

all: $(OBJS)

DEPS = $(patsubst %.o, .%.d, $(OBJS))

-include $(DEPS)
#CFLAGS	+= -fPIC

clean:
	rm -f .*.d *.o $(OBJS)
