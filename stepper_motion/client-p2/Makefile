PROGRAM = p2_move
LDFLAGS = -lpthread -lcrypt
SRCS = $(wildcard *.c)
CC = gcc
DEFINES = -D_XOPEN_SOURCE=601
CXX = gcc
CFLAGS = -Wall -Werror $(DEFINES)
OBJS = $(SRCS:.c=.o)
all : $(PROGRAM) clean
$(PROGRAM) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

# some addition dependencies
# %.o: %.c
#        $(CC) $(LDFLAGS) $(CFLAGS) $< -o $@
#$(SRCS) : %.c : %.h $(INDEPENDENT_HEADERS)
#        @touch $@

clean:
	/bin/rm -f *.o *~
depend:
	$(CXX) -MM $(CXX.SRCS)
