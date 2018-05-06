#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "glwindow.h"
#include "geometry.h"
#include <shader.hpp>

using namespace std;

const char* glGetErrorString(GLenum error)
{
    switch(error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNRECOGNIZED";
    }
}

void glPrintError(const char* label="Unlabelled Error Checkpoint", bool alwaysPrint=false)
{
    GLenum error = glGetError();
    if(alwaysPrint || (error != GL_NO_ERROR))
    {
        printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
    }
}

GLuint loadShader(const char* shaderFilename, GLenum shaderType)
{
    FILE* shaderFile = fopen(shaderFilename, "r");
    if(!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char* shaderText = new char[shaderSize+1];
    size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
    shaderText[readCount] = '\0';
    fclose(shaderFile);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char**)&shaderText, NULL);
    glCompileShader(shader);

    delete[] shaderText;

    return shader;
}

GLuint loadShaderProgram(const char* vertShaderFilename,
                       const char* fragShaderFilename)
{
    GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
    GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        cout << "Shader load error: " << message << endl;
        return 0;
    }

    return program;
}

OpenGLWindow::OpenGLWindow()
{
    axis = "z";
}

OpenGLWindow::~OpenGLWindow()
{
}


void OpenGLWindow::initGL()
{
    // We need to first specify what type of OpenGL context we need before we can create the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdlWin = SDL_CreateWindow("OpenGL Prac 1",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_OPENGL);
    if(!sdlWin)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
    }
    SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
    SDL_GL_MakeCurrent(sdlWin, glc);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = true;
    GLenum glewInitResult = glewInit();
    glGetError(); // Consume the error erroneously set by glewInit()
    if(glewInitResult != GLEW_OK)
    {
        const GLubyte* errorString = glewGetErrorString(glewInitResult);
        cout << "Unable to initialize glew: " << errorString;
    }

    int glMajorVersion;
    int glMinorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
    cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
    cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
    cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
    cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0,0,0,1);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS); 

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Note - I am using the LoadShaders method from the shaders.cpp file.
    //This file was included with the matrices example provided to us.
    //It was originally from - http://www.opengl-tutorial.org/
    //Original source code available at: https://github.com/opengl-tutorials/ogl
    shader = LoadShaders("simple.vert", "simple.frag");

    MatrixID = glGetUniformLocation(shader, "MVP");

    // Projection matrix : 30° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    Projection = glm::perspective(glm::radians(FOV), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    View = glm::lookAt(
                                glm::vec3(3,3,3), // Camera position
                                glm::vec3(0,0,0), // camera looks towards
                                glm::vec3(0,1,0)  // Head is up
                           );
    Model = glm::mat4(1.0f);//identity matrix - sets the model at the origin
    
    MVP = Projection * View * Model; // the model view projection
    glUseProgram(shader);

    // Load the model that we want to use and buffer the vertex attributes
    geometry.loadFromOBJFile(object_1);
    
    int num_vertices = geometry.vertexCount()*3;
    void* object_data = geometry.vertexData();

    GLfloat color_data[num_vertices];

    //generate random colours
    for (int i = 0; i < num_vertices; ++i)
    {
        float r = static_cast<float>(rand())/static_cast<float>(RAND_MAX);
        color_data[i] = r;
    }

    //for vertices
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, num_vertices*sizeof(float), object_data, GL_STATIC_DRAW);

    //for colours
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, num_vertices*sizeof(float), color_data, GL_STATIC_DRAW);;

    glPrintError("Setup complete", true);
}

void OpenGLWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader);

    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // For vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(
        0,                  // 0 to match number in shader
        3,                  
        GL_FLOAT,           
        GL_FALSE,           
        0,                  
        (void*)0            
    );

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glVertexAttribPointer(
        1,                  // attribute. No particular reason for 1, but must match the layout in the shader.
        3,                  
        GL_FLOAT,           
        GL_FALSE,           
        0,                  
        (void*)0            
    );
    

    glDrawArrays(GL_TRIANGLES, 0, geometry.vertexCount()*3);//vertexCount returns vertices/3

    glDisableVertexAttribArray(0);

    // Swap the front and back buffers on the window, effectively putting what we just "drew"
    // onto the screen (whereas previously it only existed in memory)
    SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{
    // A list of keycode constants is available here: https://wiki.libsdl.org/SDL_Keycode
    // Note that SDL provides both Scancodes (which correspond to physical positions on the keyboard)
    // and Keycodes (which correspond to symbols on the keyboard, and might differ across layouts)
    if(e.type == SDL_KEYDOWN)
    {
        if(e.key.keysym.sym == SDLK_ESCAPE)
        {
            return false;
        }
        //switch to transform mode
        else if(e.key.keysym.sym == SDLK_t)
        {
            if(mode != "translate")
            {
                std::cout << "Translate mode activated" << std::endl;
            }
            mode = "translate";
            if(axis == "x"){axis = "y";}
            else if(axis == "y"){axis = "z";}
            else if(axis == "z"){axis = "x";}
            std::cout<< "Translating on " << axis << " axis" << std::endl;
            return true;
        }
        //switch to scale mode
        else if(e.key.keysym.sym == SDLK_s)
        {
            mode = "scale";
            std::cout << "Scale mode activated\nLeft click to scale up.\nRight click to scale down" << std::endl;
            return true;
        }
        //switch to rotation mode
        else if (e.key.keysym.sym == SDLK_r)
        {
            if(mode != "rotate")
            {
                std::string output = "Rotate mode\n"
                                "Left/Right click to rotate";
                std::cout << output << std::endl;
            }
            mode = "rotate";
            if(axis == "x"){axis = "y";}
            else if(axis == "y"){axis = "z";}
            else if(axis == "z"){axis = "x";}
            std::cout<< "Rotating on " << axis << " axis" << std::endl;
            return true;
        }
        else if (e.key.keysym.sym == SDLK_z)
        {
            mode = "zoom";
            std::cout << "Zoom mode" << std::endl;
            return true;
        }
        //switch to mode to add second object
        else if (e.key.keysym.sym == SDLK_a)
        {
            mode = "add";
            std::cout << "Add second object" << std::endl;
            computeMatrices(mode, e);
            return true;
        }
    }
    //once mouse is clicked, calculate changes to make to MVP
    else if(e.type == SDL_MOUSEBUTTONDOWN)
    {
        computeMatrices(mode, e);
    }
    return true;
}

void OpenGLWindow::cleanup()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
    SDL_DestroyWindow(sdlWin);
}

//Given a transformation type, compute the changes to the MVP
void OpenGLWindow::computeMatrices(std::string & type, SDL_Event e)
{
    if(type == "translate")
    {
        float speed = 1.0f;
        glm::vec3 translateVec;
        
        if(e.button.button == SDL_BUTTON_LEFT)//increase
        {
            speed = 1.0f;
        }
        else if(e.button.button == SDL_BUTTON_RIGHT)//increase
        {
            speed = -1.0f;
        }
        
        if(axis == "x"){translateVec = glm::vec3(speed,0,0);}
        if(axis == "y"){translateVec = glm::vec3(0,speed,0);}
        if(axis == "z"){translateVec = glm::vec3(0,0,speed);}
        Model = glm::translate(Model, translateVec);
        MVP = Projection * View * Model;
    }
    else if(type == "scale")
    {
        float speed;

        if(e.button.button == SDL_BUTTON_LEFT)//increase
        {
            speed = 2.0f;
        }
        else if(e.button.button == SDL_BUTTON_RIGHT)//increase
        {
            speed = 0.5f;
        }

        Model = glm::scale(Model, glm::vec3(speed,speed,speed));
        MVP = Projection * View * Model;
    }
    else if(type == "rotate")
    {
        glm::vec3 axisOfRotation;
        if(axis == "x"){axisOfRotation = glm::vec3(1,0,0);}
        if(axis == "y"){axisOfRotation = glm::vec3(0,1,0);}
        if(axis == "z"){axisOfRotation = glm::vec3(0,0,1);}

        float angle = 30.0f;
        if(e.button.button == SDL_BUTTON_LEFT)
        {
            Model = glm::rotate(Model, glm::radians(angle), axisOfRotation);
        }
        else if(e.button.button == SDL_BUTTON_RIGHT)
        {
            Model = glm::rotate(Model, glm::radians(-1.0f*angle), axisOfRotation);
        }
        MVP = Projection * View * Model;
    }
    else if(type == "zoom")
    {
        if(e.button.button == SDL_BUTTON_LEFT)//increase
        {
            FOV -= 5.0f;
        }
        else if(e.button.button == SDL_BUTTON_RIGHT)//increase
        {
            FOV += 5.0f;
        }

        Projection = glm::perspective(glm::radians(FOV), 4.0f / 3.0f, 0.1f, 100.0f);
        MVP = Projection * View * Model;
    }
    else if(type == "add")
    {
        Model = glm::mat4(1.0f);
        MVP = Projection * View * Model;
        std::cout << "Enter path of second object" << std::endl;
        std::string path;
        std::cin >> path;
        addSecondObject(path);
        mode = "none";
    }
}

//given a path of an object, load it into geometry, generate random colours and reload buffers.
void OpenGLWindow::addSecondObject(std::string & path)
{
    geometry.loadFromOBJFile(path);
    int num_vertices = geometry.vertexCount()*3;
    void* object_data = geometry.vertexData();

    GLfloat color_data[num_vertices];

    for (int i = 0; i < num_vertices; ++i)
    {
        float r = static_cast<float>(rand())/static_cast<float>(RAND_MAX);
        color_data[i] = r;
    }

    //relaod buffers
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, num_vertices*sizeof(float), object_data, GL_STATIC_DRAW);

    //for colours
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, num_vertices*sizeof(float), color_data, GL_STATIC_DRAW);
}