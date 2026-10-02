CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -O2
TARGET   = signalHunter
SRCS     = Signal_Hunters.cpp Signal_Hunters_Driver.cpp

all: $(TARGET)

$(TARGET): $(SRCS) Signal_Hunters.h
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
