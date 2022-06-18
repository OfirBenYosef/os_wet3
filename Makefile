# Makefile for the ttftps program
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Werror -pedantic-errors -DNDEBUG
CCLINK = $(CXX)
OBJS = main.o
RM = rm -f
# Creating the  executable
ttftps: $(OBJS)
	$(CCLINK) -o ttftps $(OBJS)
# Creating the object files
main.o: main.cpp tftp.h
# Cleaning old files before new make
clean:
	$(R M) $(TARGET) *.o *~ "#"* core.* ttftps

