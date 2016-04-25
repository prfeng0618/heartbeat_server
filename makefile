OPTIMIZE = -O0 -g3
## OPTIMIZE = -O3

LDFLAGS += -rdynamic
CFLAGS += -I./src/ -gstabs+
CFLAGS += -g
CFLAGS += -Wno-write-strings
CFLAGS += -D_DEBUG
# rabbit mq
CFLAGS += -I/usr/local/include
LDFLAGS +=




OBJS += \
./obj/client.o \
./obj/listenfd.o \
./obj/main.o \
./obj/main_event.o \
./obj/socket_util.o \
./obj/timers.o \
./obj/worker.o 

OBJS += ./obj/printstack.o
OBJS += ./obj/aborthandle.o

OBJS += ./obj/des.o 
OBJS += ./obj/deskey.o 
OBJS += ./obj/XORcode.o 

OBJS += ./obj/debug.o

OBJS += ./obj/business.o
OBJS += ./obj/receive.o
OBJS += ./obj/readconfig.o
OBJS += ./obj/operateredis.o

OBJS += ./obj/elfhash.o
OBJS += ./obj/hashlist.o

LIBS += -lpthread -L/usr/local/lib -lrabbitmq

# All Target
all: heartbeat

# Tool invocations
heartbeat: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "heartbeat" $(LDFLAGS) $(OBJS) $(USER_OBJS) ./libhiredis.a  $(LIBS) 
	@echo 'Finished building target: $@'
	@echo ' '
	
./obj/%.o: ./src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(OPTIMIZE) $(CFLAGS) -Wall -c -fmessage-length=0 -L/usr/lib64/mysql -lmysqlclient   -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

./obj/%.o: ./src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(OPTIMIZE) $(CFLAGS) -Wall -c -fmessage-length=0 -L/usr/lib64/mysql -lmysqlclient  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
# Other Targets
clean:
	-$(RM) ./obj/*.o $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) heartbeat
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

