CFLAGS = -g -O3 -Wall -MMD -MP
ifeq ($(USE_CLANG), true)
CC = clang
CFLAGS += -fPIE
else
CC = gcc
endif
AR = ar
ARFLAGS = rs
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all

BINDIR = ./bin

%.a:
	$(AR) $(ARFLAGS) $@ $^

$(BINDIR)/%:
	-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ -Xlinker -Map=$@.map

OBJS = opalloc.o metadata.o
LIB =  libopalloc.a
TEST = opalloc_test.o

OUTPUT = $(BINDIR)/opatest
DEL = $(OUTPUT).map
DS = $(OBJS:.o=.d) $(TEST:.o=.d)

.PHONY : all
all : $(OUTPUT)

$(LIB) : $(OBJS)

$(BINDIR)/opatest : $(TEST) $(LIB)

.PHONY : test
test : grindopa

.PHONY : grindopa
grindopa : $(BINDIR)/opatest
	echo Running ./$^
	$(VALGRIND) ./$^ > /dev/null

-include marker
marker: Makefile
	@touch $@
	$(MAKE) clean

.PHONY : clean
clean :
	rm -vf $(DS)
	rm -vf $(LIB)
	rm -vf $(OBJS)
	rm -vf $(OUTPUT)
	rm -vf $(DEL)
	rm -vf $(TEST)
	rm -vfr $(BINDIR)

-include $(OBJS:.o=.d)
