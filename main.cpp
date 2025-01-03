
#include <stdio.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define WIDTH  640
#define HEIGHT 480

// NOTE: Uncomment the following line for GL error handling
//#define GL_DEBUG

#ifdef GL_DEBUG
#define GLCALL(function) \
   { \
      GLenum error = GL_INVALID_ENUM; \
      while (error != GL_NO_ERROR) \
      { \
         error = glGetError(); \
      } \
      function; \
      error = glGetError(); \
      if (error != GL_NO_ERROR) \
      { \
         fprintf(stderr, "OpenGL Error: GL_ENUM(%d) at %s:%d\n", error, __FILE__, __LINE__); \
      } \
   }
#else
#define GLCALL(function) function;
#endif

std::string vertex_shader_source = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec4 aColor;
    layout (location = 2) in vec2 aTexCoord;

    out vec2 tex_coord;
    out vec4 color;
    uniform mat4 mvp;

    void main()
    {
        gl_Position = mvp * vec4(aPos, 1.0);
        tex_coord = aTexCoord;
        color = aColor;
    }
)";

std::string fragment_shader_source = R"(
    #version 330 core

    in vec2 tex_coord;                                                              
    in vec4 color;
    uniform vec2 rectSize;                                                          
    uniform float radius;
    uniform float border_thickness;

    const vec4 fillColor = vec4(1.0, 0.0, 0.0, 1.0);                                
    const vec4 borderColor = vec4(1.0, 1.0, 0.0, 1.0);                              

    float RectSDF(vec2 p, vec2 b, float r)                                          
    {                                                                               
        vec2 d = abs(p) - b + vec2(r);                                              
        return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r;                   
    }                                                                               

    void main()
    {
        vec2 pos = rectSize * tex_coord;                                            
                                                                                 
        float fDist;
        float fBlendAmount;
        vec4 oColor;

        if (abs(border_thickness) < 0.01)
        {
            fDist = RectSDF(pos-rectSize/2.0, rectSize/2.0, radius);
            fBlendAmount = smoothstep(-1.0, 1.0, abs(fDist));
            oColor = (fDist < 0.0) ? color : vec4(0.0);
        }
        else
        {
            fDist = RectSDF(pos-rectSize/2.0, rectSize/2.0 - border_thickness/2.0-1.0, radius);
            fBlendAmount = smoothstep(-1.0, 1.0, abs(fDist) - border_thickness / 2.0);
            vec4 v4FromColor = borderColor;                                             
            vec4 v4ToColor = (fDist <= 0.0) ? color : vec4(0.0);                     
            oColor = mix(v4FromColor, v4ToColor, fBlendAmount);                   
        }
                                                                                 
        gl_FragColor = oColor;
    }
)";

void resize(GLFWwindow* window, int width, int height)
{
   GLCALL(glViewport(0, 0, width, height));
}

bool initialize_buffers = true;
GLuint vao;
GLuint vbo;
GLuint ebo;
GLuint program;
float radius = 0.0;
float border_thickness = 10.0;
glm::vec4 color_ul = glm::vec4(1.0f);
glm::vec4 color_ur = glm::vec4(1.0f);
glm::vec4 color_lr = glm::vec4(1.0f);
glm::vec4 color_ll = glm::vec4(1.0f);

float rect[4] = { 50.0f, 50.0f, 250.0f, 250.0f };

void render()
{
   float vertices[4][9] =
   {
        // aPos                 // aColor                                       // aTexCoord
      { rect[0], rect[1], 0.0f, color_ul.r, color_ul.g, color_ul.b, color_ul.a, 0.0f, 1.0f },
      { rect[2], rect[1], 0.0f, color_ur.r, color_ur.g, color_ur.b, color_ur.a, 1.0f, 1.0f },
      { rect[2], rect[3], 0.0f, color_lr.r, color_lr.g, color_lr.b, color_lr.a, 1.0f, 0.0f },
      { rect[0], rect[3], 0.0f, color_ll.r, color_ll.g, color_ll.b, color_ll.a, 0.0f, 0.0f },
   };

   if (initialize_buffers)
   {

      GLuint indices[6] =
      {
         0, 1, 2,
         0, 2, 3,
      };

      printf("Initialize buffers, program %d\n", program);
      GLCALL(glGenVertexArrays(1, &vao));
      GLCALL(glGenBuffers(1, &vbo));
      GLCALL(glGenBuffers(1, &ebo));

      GLCALL(glBindVertexArray(vao));
      GLCALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
      GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW));
      GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
      GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));
      GLCALL(glEnableVertexAttribArray(0));
      GLCALL(glVertexAttribPointer(0, 3, GL_FLOAT, false, 9 * sizeof(float), (void*)0));
      GLCALL(glEnableVertexAttribArray(1));
      GLCALL(glVertexAttribPointer(1, 4, GL_FLOAT, false, 9 * sizeof(float), (void*)(3 * sizeof(float))));
      GLCALL(glEnableVertexAttribArray(2));
      GLCALL(glVertexAttribPointer(2, 2, GL_FLOAT, false, 9 * sizeof(float), (void*)(7 * sizeof(float))));

      initialize_buffers = false;
   }

   GLCALL(glUseProgram(program));
   int mvp_loc;
   GLCALL(mvp_loc = glGetUniformLocation(program, "mvp"));
   glm::mat4 mvp = glm::ortho(0.0, 640.0, 480.0, 0.0, -1.0, 1.0);
   GLCALL(glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp[0][0]));
   int rect_loc;
   GLCALL(rect_loc = glGetUniformLocation(program, "rectSize"));
   GLCALL(glUniform2f(rect_loc, rect[2]-rect[0], rect[3]-rect[1] ));
   int radius_loc;
   GLCALL(radius_loc = glGetUniformLocation(program, "radius"));
   GLCALL(glUniform1f(radius_loc, radius));
   int border_thickness_loc;
   GLCALL(border_thickness_loc = glGetUniformLocation(program, "border_thickness"));
   GLCALL(glUniform1f(border_thickness_loc, border_thickness));

   GLCALL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices));

   GLCALL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
}

int main(int argc, char* argv[])
{
   GLFWwindow* window = nullptr;

   // initialize glfw
   if (!glfwInit())
      return 0;

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   // Create window
   window = glfwCreateWindow(WIDTH, HEIGHT, "SDF Rounded Rectangle", NULL, NULL);

   if (!window)
   {
      glfwTerminate();
      return 0;
   }

   // make the window's context current
   glfwMakeContextCurrent(window);

   // use glad to load OpenGL function pointers
   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
   {
      printf("Error: Failed to initialize GLAD.\n");
      glfwTerminate();
      return 0;
   }

   glfwSetWindowSize(window, WIDTH, HEIGHT);

   // Setup Dear ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui::StyleColorsDark();

   // Setup Platform/Render backends
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 330");

   // Compile shaders and link program
   GLuint vertex_shader;
   GLCALL(vertex_shader = glCreateShader(GL_VERTEX_SHADER));

   const char* shader_code = vertex_shader_source.c_str();
   GLCALL(glShaderSource(vertex_shader, 1, &shader_code, nullptr));
   GLCALL(glCompileShader(vertex_shader));

   // check vertex shader
   GLint result = GL_FALSE;
   int info_log_length;
   GLCALL(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result));
   GLCALL(glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_length));
   if (info_log_length > 0)
   {
      std::vector<char> error_msg(info_log_length+1);
      GLCALL(glGetShaderInfoLog(vertex_shader, info_log_length, NULL, &error_msg[0]));
      printf("Error in vertex shader\n");
      printf("%s\n", &error_msg[0]);
   }

   GLuint fragment_shader;
   GLCALL(fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));

   shader_code = fragment_shader_source.c_str();
   GLCALL(glShaderSource(fragment_shader, 1, &shader_code, nullptr));
   GLCALL(glCompileShader(fragment_shader));

   // check fragment shader
   GLCALL(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result));
   GLCALL(glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_length));
   if (info_log_length > 0)
   {
      std::vector<char> error_msg(info_log_length+1);
      GLCALL(glGetShaderInfoLog(fragment_shader, info_log_length, NULL, &error_msg[0]));
      printf("Error in fragment shader\n");
      printf("%s\n", &error_msg[0]);
   }

   GLCALL(program = glCreateProgram());
   GLCALL(glAttachShader(program, vertex_shader));
   GLCALL(glAttachShader(program, fragment_shader));
   GLCALL(glLinkProgram(program));

   // check the program
   GLCALL(glGetProgramiv(program, GL_LINK_STATUS, &result));
   GLCALL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length));
   if (info_log_length > 0)
   {
      std::vector<char> error_msg(info_log_length+1);
      GLCALL(glGetProgramInfoLog(program, info_log_length, NULL, &error_msg[0]));
      printf("Error linking program\n");
      printf("%s\n", &error_msg[0]);
   }

   GLCALL(glDetachShader(program, vertex_shader));
   GLCALL(glDetachShader(program, fragment_shader));

   GLCALL(glDeleteShader(vertex_shader));
   GLCALL(glDeleteShader(fragment_shader));

   // Make the window visible
   glfwShowWindow(window);

   // Initialize opengl
   GLCALL(glClearColor(0.5, 0.5, 0.5, 1.0));

   // enable blending
   GLCALL(glEnable(GL_BLEND));
   GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

   glfwSetWindowSizeCallback(window, resize);

   // set frame rate to 60 Hz
   using framerate = std::chrono::duration<double, std::ratio<1, 60>>;
   auto frame_time = std::chrono::high_resolution_clock::now() + framerate{1};

   while (window)
   {
      // Poll events
      glfwPollEvents();

      if (glfwWindowShouldClose(window))
      {
         glfwTerminate();
         window = nullptr;
         break;
      }

      GLCALL(glClear(GL_COLOR_BUFFER_BIT));

      render();

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("Debug");
      ImGui::SliderFloat("Corner Radius", &radius, 0.0f, 100.0f);
      ImGui::SliderFloat("Border Thickness", &border_thickness, 0.0f, 100.0f);
      ImGui::ColorEdit4("Upper Left", &color_ul.r);
      ImGui::ColorEdit4("Upper Right", &color_ur.r);
      ImGui::ColorEdit4("Lower Right", &color_lr.r);
      ImGui::ColorEdit4("Lower Left", &color_ll.r);
      ImGui::End();

      // Render ImGui
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);

      // wait until next frame
      std::this_thread::sleep_until(frame_time);
      frame_time += framerate{1};
   }

   return 0;
}
