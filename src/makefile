include ../Inc.mk

EXE = ../bin/RelaySvr

OBJS =   client_socket.o core_server_conf.o \
	hasite_conf.o main.o relay_log.o relay_server.o \
	request_type_conf.o system_conf.o access_control.o frequency_control.o \
        errcode_escape_conf.o detail_info_service_conf.o \
        switch_mgr.o service_fixed_control_rule.o service_request_control.o service_authorize_access.o \
        relay_mgr_command.o time_count_policy.o
	
all: $(EXE) 

LIB_API = ../lib/librelay_api.a

DEPS = $(patsubst %.o, .%.d, $(OBJS))

$(EXE): $(OBJS) $(LIB_API) $(LIB_COMM)
	$(CXX) -o $@ $(CFLAGS) $^ -lpthread -ldl -lz 

-include $(DEPS)
#CFLAGS += -fPIC

clean:
	rm -f *.o .*.d  $(OBJS)
	
install:
	#cp $(EXE) /usr/local/qpay/PayRelaySvr -rvf
	cp $(EXE) /usr/local/PayRelaySvr/RelaySvr -rvf
