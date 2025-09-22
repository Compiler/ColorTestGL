#include <cstdio>
#include <cstdlib>
#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
  #include <OpenGL/gl3.h>
#else
  #include <glad/gl.h>
  #define GLAD_GL_IMPLEMENTATION
  #include <glad/gl.h>
#endif

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

static void glfw_error_callback(int code, const char* desc) {
    std::fprintf(stderr, "GLFW error [%d]: %s\n", code, desc ? desc : "");
}

static const char* kVert = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aCol;
out vec3 vCol;
void main() {
    vCol = aCol;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* kFrag = R"(
#version 330 core
in vec3 vCol;
out vec4 FragColor;
void main() {
    FragColor = vec4(vCol, 1.0);
}
)";

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile error: %s\n", log.c_str());
        std::exit(EXIT_FAILURE);
    }
    return s;
}

static GLuint link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "Program link error: %s\n", log.c_str());
        std::exit(EXIT_FAILURE);
    }
    return p;
}

static void ocio_apply(float* verts, int count) {
    auto config = OCIO::Config::CreateRaw();
    auto t = OCIO::ExponentTransform::Create();
    double v[4] = {2.2, 2.2, 2.2, 1.0};
    t->setValue(v);
    auto proc = config->getProcessor(t);
    auto cpu = proc->getDefaultCPUProcessor();
    for (int i = 0; i < count; ++i) {
        float rgb[3] = { verts[i*5+2], verts[i*5+3], verts[i*5+4] };
        cpu->applyRGB(rgb);
        verts[i*5+2] = rgb[0];
        verts[i*5+3] = rgb[1];
        verts[i*5+4] = rgb[2];
    }
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return EXIT_FAILURE;

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    GLFWwindow* win = glfwCreateWindow(800, 600, "GLFW Triangle + OCIO", nullptr, nullptr);
    if (!win) return EXIT_FAILURE;
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

#ifndef __APPLE__
    if (!gladLoadGL(glfwGetProcAddress)) return EXIT_FAILURE;
#endif

    float verts[] = {
         0.0f,  0.6f,   1.0f, 0.2f, 0.3f,
        -0.6f, -0.6f,   0.2f, 0.8f, 1.0f,
         0.6f, -0.6f,   0.3f, 1.0f, 0.2f,
    };

    ocio_apply(verts, 3);

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

    GLuint vs = compile(GL_VERTEX_SHADER,   kVert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFrag);
    GLuint prog = link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);

    while (!glfwWindowShouldClose(win)) {
        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteProgram(prog);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}

