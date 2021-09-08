CXX = g++
CFLAGS = -std=c++11 -O2 -Wall -g 

TARGET = ./bin/server
OBJS = ./code/*.cpp 
HEADERS = ./include

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET) -I $(HEADERS) -pthread -lmysqlclient

clean:
	rm -rf ./bin/$(OBJS) $(TARGET)




