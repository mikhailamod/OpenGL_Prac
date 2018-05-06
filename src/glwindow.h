#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "geometry.h"

class OpenGLWindow
{
public:
    std::string object_1;
    std::string mode;
    std::string axis;
    OpenGLWindow();
    ~OpenGLWindow();

    void initGL();
    void render();
    bool handleEvent(SDL_Event e);
    void cleanup();
    void computeMatrices(std::string & type, SDL_Event e);
    void addSecondObject(std::string & path);

    SDL_Window* sdlWin;

    

private:

    GLuint vao;
    GLuint shader;
    GLuint vertexBuffer;
    GLuint colorBuffer;
    GLuint MatrixID;//used for camera
    
    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
    glm::mat4 MVP;
    
    GeometryData geometry;

    glm::vec3 position = glm::vec3(0,0,5);
    float speed = 3.0f;
    float FOV = 30.0f;
};

#endif
