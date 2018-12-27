# middle Makefile公共规则定义
#
# 2011-05-16 vermin

CC = gcc
CXX	= g++
#CFLAGS	= -Wall	-g
#CFLAGS	= -Wall	-g -DDEBUG_LOG_ARGS
#CFLAGS	= -O3 -DNDEBUG -Wall

CFLAGS = -g	-Wall -DDEBUG

#RULE_FROM_FILE := yes
ifeq (yes, $(RULE_FROM_FILE))
CFLAGS += -DRULE_FROM_FILE
endif

RELAY_HOME = /data/adam/relay_proxy_140
CFT_LIB = /data/adam/relay_proxy_140/cftlib

INCLUDE	=  -I$(RELAY_HOME)/src -I$(RELAY_HOME)/sys -I$(RELAY_HOME)/base -I$(RELAY_HOME)/api -I$(CFT_LIB)/tinyxml

LIB_COMM = $(RELAY_HOME)/lib/librelay_comm.a $(CFT_LIB)/tinyxml/libtinyxml.a

# 自动计算文件的依赖关系
.%.d: %.cpp
	$(CC) $(INCLUDE) -MM $<	> $@
	@$(CC) $(INCLUDE) -MM $< | sed s/"^"/"\."/	|  sed s/"^\. "/" "/  |	\
				sed	s/"\.o"/"\.d"/	>> $@
%.o: %.cpp 
	$(CXX) $(CFLAGS) $(INCLUDE)	-c $<

.%.d: %.c
	$(CC) $(INCLUDE) -MM $<	> $@
	@$(CC) $(INCLUDE) -MM $< | sed s/"^"/"\."/	|  sed s/"^\. "/" "/  |	\
				sed	s/"\.o"/"\.d"/	>> $@
%.o: %.c
	$(CC) $(CFLAGS)	$(INCLUDE) -c $<

