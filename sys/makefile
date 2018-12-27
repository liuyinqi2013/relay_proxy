include ../Inc.mk

LIB_STATIC = ../lib/librelay_comm.a

OBJS = epoll_monitor.o event_base.o event_monitor.o event_queue.o event_thread.o \
	log.o log_mutex.o thread_mutex.o trpc_config.o trpc_key_value_conf.o \
	trpc_socket_opt.o trpc_time_log.o socket_base.o

OTHER_OBJS =  ../base/trpc_comm_func.o ../base/trpc_exception.o ../base/trpc_url_param_map.o
	
	
all: $(OBJS) $(OTHER_OBJS) $(LIB_STATIC)

DEPS = $(patsubst %.o, .%.d, $(OBJS))

$(LIB_STATIC): $(OBJS) $(OTHER_OBJS) $(OBJS)
	rm -f $@
	ar q $@ $^

-include $(DEPS)
#CFLAGS += -fPIC

clean:
	rm -f *.o .*.d $(OBJS) $(LIB_STATIC)
