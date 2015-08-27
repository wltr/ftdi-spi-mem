TARGET = ftdi-spi-mem
SRC = main.cpp ftdi_spi_mem.cpp
OBJ = $(SRC:%.cpp=%.o)

CC = g++
CFLAGS =
LFLAGS = -lftd2xx -lMPSSE

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm *.o
