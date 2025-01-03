CXXFLAGS = -I. -Iglm

all:
	g++ $(CXXFLAGS) main.cpp -o main -lglfw glad/glad.o

clean:
	rm -f main
