
#include <stdio.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    layout (location = 1) in vec2 aTexCoord;

    out vec2 tex_coord;
    uniform mat4 mvp;

    void main()
    {
        gl_Position = mvp * vec4(aPos, 1.0);
        tex_coord = aTexCoord;
    }
)";

std::string fragment_shader_source = R"(
    #version 330 core

    in vec2 tex_coord;                                                              
    uniform vec2 rectSize;                                                          

    const vec4 fillColor = vec4(1.0, 0.0, 0.0, 1.0);                                
    const vec4 borderColor = vec4(1.0, 1.0, 0.0, 1.0);                              
    const float borderThickness = 10.0;                                             

    const float radius = 30.0;                                                      
                                                                                    
    float RectSDF(vec2 p, vec2 b, float r)                                          
    {                                                                               
        vec2 d = abs(p) - b + vec2(r);                                              
        return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r;                   
    }                                                                               

    void main()
    {
        vec2 pos = rectSize * tex_coord;                                            
                                                                                 
        float fDist = RectSDF(pos-rectSize/2.0, rectSize/2.0 - borderThickness/2.0-1.0, radius);
        float fBlendAmount = smoothstep(-1.0, 1.0, abs(fDist) - borderThickness / 2.0);
                                                                                 
        vec4 v4FromColor = borderColor;                                             
        vec4 v4ToColor = (fDist < 0.0) ? fillColor : vec4(0.0);                     
        gl_FragColor = mix(v4FromColor, v4ToColor, fBlendAmount);                   
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

void render()
{
   if (initialize_buffers)
   {
      float vertices[4][5] =
      {
           // aPos               // aTexCoord
         {  50.0f,  50.0f, 0.0f, 0.0f, 1.0f },
         { 350.0f,  50.0f, 0.0f, 1.0f, 1.0f },
         { 350.0f, 250.0f, 0.0f, 1.0f, 0.0f },
         {  50.0f, 250.0f, 0.0f, 0.0f, 0.0f },
      };

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
      GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
      GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
      GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));
      GLCALL(glEnableVertexAttribArray(0));
      GLCALL(glVertexAttribPointer(0, 3, GL_FLOAT, false, 5 * sizeof(float), (void*)0));
      GLCALL(glEnableVertexAttribArray(1));
      GLCALL(glVertexAttribPointer(1, 2, GL_FLOAT, false, 5 * sizeof(float), (void*)(3 * sizeof(float))));

      GLCALL(glUseProgram(program));
      int mvp_loc;
      GLCALL(mvp_loc = glGetUniformLocation(program, "mvp"));
      glm::mat4 mvp = glm::ortho(0.0, 640.0, 480.0, 0.0, -1.0, 1.0);
      GLCALL(glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp[0][0]));
      int rect_loc;
      GLCALL(rect_loc = glGetUniformLocation(program, "rectSize"));
      GLCALL(glUniform2f(rect_loc, 300.0, 200.0));

      initialize_buffers = false;
   }

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

   //GLCALL(glDetachShader(program, vertex_shader));
   //GLCALL(glDetachShader(program, fragment_shader));

   //GLCALL(glDeleteShader(vertex_shader));
   //GLCALL(glDeleteShader(fragment_shader));

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

      glfwSwapBuffers(window);

      // wait until next frame
      std::this_thread::sleep_until(frame_time);
      frame_time += framerate{1};
   }

   return 0;
}
