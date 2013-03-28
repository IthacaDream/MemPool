CXX=g++
RM=rm -rf
CFLAGS=-g -lpthread
OBJS=main.o memory_pool.o
TARGET=test
$(TARGET):$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(CFLAGS)
$(OBJS):%o:%cc
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-$(RM) $(TARGET) $(OBJS)
