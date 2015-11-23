OBJS := queue.o
CFLAGS := -std=gnu99 -ggdb
CLIBS := -lpthread

TESTS := test1.out test2.out test3.out
# link

all: $(TESTS)

test%.out: $(OBJS)
	gcc test$*.c $(OBJS) $(CLIBS) $(CFLAGS) -o $@

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
%.o: %.c
	gcc -c $(CFLAGS) $*.c -o $*.o
	gcc -MM $(CFLAGS) $*.c > $*.d

# remove compilation products
clean:
	rm -f *.o $(TESTS)

clean-all: clean
	rm -f *.d
.PHONY: clean all
