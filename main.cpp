#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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

static void glfw_error_callback(int code, const char* desc){ std::fprintf(stderr,"GLFW error [%d]: %s\n",code,desc?desc:""); }

static const char* kVert = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aCol;
out vec3 vCol;
void main(){ vCol=aCol; gl_Position=vec4(aPos,0.0,1.0); }
)";

static const char* kFrag = R"(
#version 330 core
in vec3 vCol;
out vec4 FragColor;
void main(){ FragColor=vec4(vCol,1.0); }
)";

static GLuint compile(GLenum type,const char* src){
    GLuint s=glCreateShader(type); glShaderSource(s,1,&src,nullptr); glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ GLint len=0; glGetShaderiv(s,GL_INFO_LOG_LENGTH,&len); std::string log(len,'\0'); glGetShaderInfoLog(s,len,nullptr,log.data()); std::fprintf(stderr,"Shader compile error: %s\n",log.c_str()); std::exit(EXIT_FAILURE); }
    return s;
}
static GLuint link(GLuint vs,GLuint fs){
    GLuint p=glCreateProgram(); glAttachShader(p,vs); glAttachShader(p,fs); glLinkProgram(p);
    GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ GLint len=0; glGetProgramiv(p,GL_INFO_LOG_LENGTH,&len); std::string log(len,'\0'); glGetProgramInfoLog(p,len,nullptr,log.data()); std::fprintf(stderr,"Program link error: %s\n",log.c_str()); std::exit(EXIT_FAILURE); }
    return p;
}

struct App {
    GLFWwindow* win{};
    GLuint vao{}, vbo{}, prog{};
    float verts[15] = {
         0.0f,  0.6f, 1.0f, 0.2f, 0.3f,
        -0.6f, -0.6f, 0.2f, 0.8f, 1.0f,
         0.6f, -0.6f, 0.3f, 1.0f, 0.2f
    };
    float orig[9];
    OCIO::ConstConfigRcPtr cfg;
    std::vector<std::string> spaces;
    int sel = -1;
} app;

static void rebuild_colors(){
    for(int i=0;i<3;i++){ app.verts[i*5+2]=app.orig[i*3+0]; app.verts[i*5+3]=app.orig[i*3+1]; app.verts[i*5+4]=app.orig[i*3+2]; }
    if(!app.cfg || app.spaces.empty() || app.sel<0 || app.sel>=(int)app.spaces.size()) return;
    const char* src = nullptr;
    try { src = app.cfg->getRoleColorSpace(OCIO::ROLE_SCENE_LINEAR); } catch(...) {}
    if(!src || !*src){
        if(app.cfg->getNumColorSpaces()>0) src = app.cfg->getColorSpaceNameByIndex(0);
        else src = "raw";
    }
    const char* dst = app.spaces[app.sel].c_str();
    if(std::string(src)==std::string(dst)) goto upload;
    try{
        auto t = OCIO::ColorSpaceTransform::Create();
        t->setSrc(src);
        t->setDst(dst);
        auto proc = app.cfg->getProcessor(t);
        auto cpu = proc->getDefaultCPUProcessor();
        for(int i=0;i<3;i++){
            float rgb[3] = { app.verts[i*5+2], app.verts[i*5+3], app.verts[i*5+4] };
            cpu->applyRGB(rgb);
            app.verts[i*5+2]=rgb[0]; app.verts[i*5+3]=rgb[1]; app.verts[i*5+4]=rgb[2];
        }
    } catch(const std::exception& e){ std::fprintf(stderr,"OCIO error: %s\n", e.what()); }
upload:
    glBindBuffer(GL_ARRAY_BUFFER, app.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(app.verts), app.verts);
}

static void set_title(){
    std::string t = "GLFW Triangle + OCIO";
    if(!app.spaces.empty() && app.sel>=0 && app.sel<(int)app.spaces.size()){
        t += " [" + app.spaces[app.sel] + "]";
    }
    glfwSetWindowTitle(app.win, t.c_str());
}

static void key_cb(GLFWwindow*,int key,int, int action,int){
    if(action!=GLFW_PRESS) return;
    if(key>=GLFW_KEY_1 && key<=GLFW_KEY_9){
        int idx = key - GLFW_KEY_1;
        if(idx<(int)app.spaces.size()){
            app.sel = idx;
            rebuild_colors();
            set_title();
        }
    } else if(key==GLFW_KEY_0){
        app.sel = -1;
        rebuild_colors();
        set_title();
    } else if(key==GLFW_KEY_ESCAPE){
        glfwSetWindowShouldClose(app.win,1);
    }
}

int main(){
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()) return EXIT_FAILURE;

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#endif

    app.win = glfwCreateWindow(800,600,"GLFW Triangle + OCIO",nullptr,nullptr);
    if(!app.win) return EXIT_FAILURE;
    glfwMakeContextCurrent(app.win);
    glfwSwapInterval(1);
    glfwSetKeyCallback(app.win, key_cb);

#ifndef __APPLE__
    if(!gladLoadGL(glfwGetProcAddress)) return EXIT_FAILURE;
#endif

    for(int i=0;i<3;i++){ app.orig[i*3+0]=app.verts[i*5+2]; app.orig[i*3+1]=app.verts[i*5+3]; app.orig[i*3+2]=app.verts[i*5+4]; }

    try { app.cfg = OCIO::GetCurrentConfig(); } catch(...) { app.cfg.reset(); }
    if(!app.cfg || app.cfg->getNumColorSpaces()==0){
        try { app.cfg = OCIO::Config::CreateRaw(); } catch(...) { app.cfg.reset(); }
    }
    if(app.cfg){
        int n = (int)app.cfg->getNumColorSpaces();
        for(int i=0;i<n && i<9;i++){
            const char* nm = app.cfg->getColorSpaceNameByIndex(i);
            if(nm && *nm) app.spaces.emplace_back(nm);
        }
    }
    app.sel = app.spaces.empty() ? -1 : 0;

    glGenVertexArrays(1,&app.vao);
    glGenBuffers(1,&app.vbo);
    glBindVertexArray(app.vao);
    glBindBuffer(GL_ARRAY_BUFFER, app.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(app.verts), app.verts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(2*sizeof(float)));

    GLuint vs=compile(GL_VERTEX_SHADER,kVert);
    GLuint fs=compile(GL_FRAGMENT_SHADER,kFrag);
    app.prog=link(vs,fs);
    glDeleteShader(vs); glDeleteShader(fs);

    glClearColor(0.08f,0.09f,0.11f,1.0f);

    rebuild_colors();
    set_title();

    while(!glfwWindowShouldClose(app.win)){
        int w,h; glfwGetFramebufferSize(app.win,&w,&h);
        glViewport(0,0,w,h);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(app.prog);
        glBindVertexArray(app.vao);
        glDrawArrays(GL_TRIANGLES,0,3);
        glfwSwapBuffers(app.win);
        glfwPollEvents();
    }

    glDeleteProgram(app.prog);
    glDeleteBuffers(1,&app.vbo);
    glDeleteVertexArrays(1,&app.vao);
    glfwDestroyWindow(app.win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
