CXXFLAGS = -I. -Iglm -Iimgui -Iimgui/backends

all:
#	g++ $(CXXFLAGS) -c imgui/imgui.cpp -o imgui.o
#	g++ $(CXXFLAGS) -c imgui/imgui_draw.cpp -o imgui_draw.o
#	g++ $(CXXFLAGS) -c imgui/imgui_tables.cpp -o imgui_tables.o
#	g++ $(CXXFLAGS) -c imgui/imgui_widgets.cpp -o imgui_widgets.o
#	g++ $(CXXFLAGS) -c imgui/backends/imgui_impl_glfw.cpp -o imgui_impl_glfw.o
#	g++ $(CXXFLAGS) -c imgui/backends/imgui_impl_opengl3.cpp -o imgui_impl_opengl3.o
	g++ $(CXXFLAGS) main.cpp -o main -lglfw glad/glad.o imgui.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o

clean:
	rm -f main
	rm -f *.o
