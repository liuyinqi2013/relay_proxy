include ../Inc.mk

LIB_STATIC = ../lib/librelay_api.a

OBJS = middle_api.o trpc_package.o relay_api.o \
	 api_base.o api_pool.o 

OTHER_OBJS = 
	
	
all: $(OBJS) $(OTHER_OBJS) $(LIB_STATIC)

DEPS = $(patsubst %.o, .%.d, $(OBJS))

$(LIB_STATIC): $(OBJS) $(OTHER_OBJS) $(OBJS)
	rm -f $@
	ar q $@ $^

-include $(DEPS)
#CFLAGS += -fPIC

clean:
	rm -f *.o .*.d $(OBJS) $(LIB_STATIC)
