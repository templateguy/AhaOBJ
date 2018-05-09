#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include "glfw/glfw3.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "OBJModel.hpp"

#include "trackball.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


int width = 768;
int height = 768;

double prevMouseX, prevMouseY;
bool mouseLeftPressed;
bool mouseMiddlePressed;
bool mouseRightPressed;
float curr_quat[4];
float prev_quat[4];
float eye[3], lookat[3], up[3];

GLFWwindow* window;


static void reshapeFunc(GLFWwindow* window, int w, int h)
{
    int fb_w, fb_h;
    // Get actual framebuffer size.
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    
    glViewport(0, 0, fb_w, fb_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 0.01f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    width = w;
    height = h;
}

static void keyboardFunc(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)window;
    (void)scancode;
    (void)mods;
    if(action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Move camera
        float mv_x = 0, mv_y = 0, mv_z = 0;
        if(key == GLFW_KEY_D)
            mv_x += 1;
        else if(key == GLFW_KEY_A)
            mv_x += -1;
        else if(key == GLFW_KEY_W)
            mv_y += 1;
        else if(key == GLFW_KEY_S)
            mv_y += -1;
        else if(key == GLFW_KEY_Z)
            mv_z += 1;
        else if(key == GLFW_KEY_X)
            mv_z += -1;
        // camera.move(mv_x * 0.05, mv_y * 0.05, mv_z * 0.05);
        // Close window
        if(key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GL_TRUE);
        
        // init_frame = true;
    }
}

static void clickFunc(GLFWwindow* window, int button, int action, int mods)
{
    (void)window;
    (void)mods;
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if(action == GLFW_PRESS)
        {
            mouseLeftPressed = true;
            trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
        }
        else if(action == GLFW_RELEASE)
        {
            mouseLeftPressed = false;
        }
    }
    if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if(action == GLFW_PRESS)
        {
            mouseRightPressed = true;
        }
        else if(action == GLFW_RELEASE)
        {
            mouseRightPressed = false;
        }
    }
    if(button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if(action == GLFW_PRESS)
        {
            mouseMiddlePressed = true;
        }
        else if(action == GLFW_RELEASE)
        {
            mouseMiddlePressed = false;
        }
    }
}

static void motionFunc(GLFWwindow* window, double mouse_x, double mouse_y)
{
    (void)window;
    float rotScale = 1.0f;
    float transScale = 2.0f;
    
    if(mouseLeftPressed)
    {
        trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
                  rotScale * (height - 2.0f * prevMouseY) / (float)height,
                  rotScale * (2.0f * mouse_x - width) / (float)width,
                  rotScale * (height - 2.0f * mouse_y) / (float)height);
        
        add_quats(prev_quat, curr_quat, curr_quat);
    }
    else if(mouseMiddlePressed)
    {
        eye[0] -= transScale * (mouse_x - prevMouseX) / (float)width;
        lookat[0] -= transScale * (mouse_x - prevMouseX) / (float)width;
        eye[1] += transScale * (mouse_y - prevMouseY) / (float)height;
        lookat[1] += transScale * (mouse_y - prevMouseY) / (float)height;
    }
    else if(mouseRightPressed)
    {
        eye[2] += transScale * (mouse_y - prevMouseY) / (float)height;
        lookat[2] += transScale * (mouse_y - prevMouseY) / (float)height;
    }
    
    // Update mouse point
    prevMouseX = mouse_x;
    prevMouseY = mouse_y;
}

static void Init()
{
    eye[0] = 0.0f;
    eye[1] = 0.0f;
    eye[2] = 3.0f;
    
    lookat[0] = 0.0f;
    lookat[1] = 0.0f;
    lookat[2] = 0.0f;
    
    up[0] = 0.0f;
    up[1] = 1.0f;
    up[2] = 0.0f;
}

int main(int argc, char** argv)
{
    Init();
    
    if(!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    window = glfwCreateWindow(width, height, "Obj viewer", NULL, NULL);
    if(window == NULL)
    {
        std::cerr << "Failed to open GLFW window. " << std::endl;
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // Callback
    //glfwSetWindowSizeCallback(window, reshapeFunc);
    //glfwSetKeyCallback(window, keyboardFunc);
    //glfwSetMouseButtonCallback(window, clickFunc);
    //glfwSetCursorPosCallback(window, motionFunc);
    
    //reshapeFunc(window, width, height);
    
    aha::OBJModel objModel;
    objModel.loadModel("pbr/models/PB171_sneaker_hi/PB171_sneaker_hi.obj");
    
    while(glfwWindowShouldClose(window) == GL_FALSE)
    {
        glfwPollEvents();
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        objModel.render();
        
        glfwSwapBuffers(window);
    }
    
    glfwTerminate();
}
