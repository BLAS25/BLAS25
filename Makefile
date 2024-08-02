
CC = g++
CFLAGS = -O3
LIB =
 

SRCDIR := src
OBJDIR := obj

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

TARGET := main

$(TARGET): $(OBJS)
#$(CC) $(CFLAGS) $^ -o $@ $(LIB)
	g++ main.cpp $(OBJS) -O3 -g -L./lib -I/usr/include/crypto++ -lssl -lcryptopp -lboost_serialization -o main

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

main1:$(OBJS)
	g++ main.cpp $(OBJS) -O3 -g -L./lib -I/usr/include/crypto++ -lssl -lcryptopp -lboost_serialization -o main

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(TARGET)

