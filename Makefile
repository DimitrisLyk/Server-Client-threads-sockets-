# Set the Compiler (C++)
COMP = gcc

# Add debugger if you want (currently no debugger)
DB = -Wall -g

# Flag for objective & executable
OBJF = -c
EXEF = -o
# Name for executable
OUTF_POLLER = poller
OUTF_POLLSWAYER = pollSwayer

# Set all the Source files
SRCS_SERVER = server.c queue.c
SRCS_CLIENT = client.c
# Set all the Header files
HDRS = queue.h
# Set documentation files
#DOCS = Readme.pdf
# All the corresponding object files will be all sources with .o
OBJS_SERVER = ${SRCS_SERVER:.c=.o}
OBJS_CLIENT = ${SRCS_CLIENT:.c=.o}

# Set use of the option < make >
all: $(OUTF_POLLER) $(OUTF_POLLSWAYER)

$(OUTF_POLLER): $(OBJS_SERVER)
	$(COMP) $(DB) $(EXEF) $(OUTF_POLLER) $(OBJS_SERVER) $(ADDLIBS) -lpthread

$(OUTF_POLLSWAYER): $(OBJS_CLIENT)
	$(COMP) $(DB) $(EXEF) $(OUTF_POLLSWAYER) $(OBJS_CLIENT) $(ADDLIBS) -lpthread

server.o: server.c
	$(COMP) $(DB) $(OBJF) $^

client.o: client.c
	$(COMP) $(DB) $(OBJF) $^

# Compile queue.c
queue.o: queue.c
	$(COMP) $(DB) $(OBJF) $^


# Set use of the option < make clean >
clean: rmObj rmExe

rmObj:
	-rm -f $(OBJS_SERVER) $(OBJS_CLIENT)
rmExe:
	-rm -f $(OUTF_POLLER) $(OUTF_POLLSWAYER)

debug: $(OUTF_POLLER)
	gdb ./$(OUTF_POLLER)


# Set use of the option < make rebuilt >
rebuilt: clean all

test: all clean
