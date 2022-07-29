/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z


// Std. Includes
#include <string>

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY use THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders and to load models
#include "utils/shader.h"
#include "utils/model.h "

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// dimensions of application's window
GLuint screenWidth = 1200, screenHeight = 900;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

GLfloat deltaTime = 0.f;
GLfloat lastFrame = 0.f;

GLfloat orientation_y = 0.f;
GLfloat spin_speed = 30.f;

GLboolean spin = GL_FALSE;
GLboolean wire = GL_FALSE;

glm::vec3 sky_color{0,0,0};
glm::vec3 my_color{0,0,0};
GLfloat weight = 0.2f;
GLfloat speed  = 5.0f;

GLfloat frequency = 10.f;
GLfloat power = 1.f;
GLfloat harmonics = 4.f;

void setup_model(Shader shader, const glm::mat4& view, glm::mat4& model, glm::mat3& norm)
{
    norm = glm::inverseTranspose(glm::mat3(view * model));

    //glUniformMatrix4fv(glGetUniformLocation(shader.program, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
    //glUniformMatrix3fv(glGetUniformLocation(shader.program, "u_norm"), 1, GL_FALSE, glm::value_ptr(view));
    shader.setMat4("u_model", model);
    shader.setMat3("u_norm", norm);
}

/////////////////// MAIN function ///////////////////////
int main()
{
    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 4.1 Core
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Basic deformations", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);

    // we disable the mouse cursor
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    // the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f); // sky
    //glClearColor(0.05f, 0.05f, 0.05f, 1.0f);   // black

    // we create and compile shaders (code of Shader class is in include/utils/shader.h)
    std::vector<const GLchar*> utils_shaders{ "shaders/types.glsl", "shaders/constants.glsl" };
    Shader shader("shaders/deform.vert", "shaders/normals.frag", utils_shaders);

    // we load the model(s) (code of Model class is in include/utils/model.h)
    Model cube  ("models/cube.obj"  );
    Model sphere("models/sphere.obj");
    Model bunny ("models/bunny.obj" );

    // We "install" the Shader program as part of the current rendering process
    // with only one Shader program, we can do this call now, and not inside the main loop:
    // we will use this Shader program for everything is rendered after this call
    shader.use();   // <-- Don't forget this one!

    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 7),
                                 glm::vec3(0, 0, 0),
                                 glm::vec3(0, 1, 0));

    glm::mat4 proj = glm::perspective(45.f, 
        (float) screenWidth / (float)screenHeight, 0.1f, 100.f);

    glm::mat4 cube_model_mat{1} , sphere_model_mat{1}, bunny_model_mat{1};
    glm::mat3 cube_normal_mat{1}, sphere_normal_mat{1}, bunny_normal_mat{1};
    
    cube_model_mat = glm::translate(cube_model_mat, glm::vec3(0, 0, 0));
    cube_model_mat = glm::scale    (cube_model_mat, glm::vec3(0.8f, 0.8f, 0.8f));
    sphere_model_mat = glm::translate(sphere_model_mat, glm::vec3(-3, 0, 0));
    sphere_model_mat = glm::scale    (sphere_model_mat, glm::vec3(0.8f, 0.8f, 0.8f));
    bunny_model_mat = glm::translate(bunny_model_mat, glm::vec3(3, 0, 0));
    bunny_model_mat = glm::scale    (bunny_model_mat, glm::vec3(0.3f, 0.3f, 0.3f));

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        const GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();

        // we "clear" the frame and z  buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setting up uniforms
        shader.setMat4("u_proj", proj);
        shader.setMat4("u_view", view);
        //glUniformMatrix4fv(glGetUniformLocation(shader.program, "u_proj"), 1, GL_FALSE, glm::value_ptr(proj));
        //glUniformMatrix4fv(glGetUniformLocation(shader.program, "u_view"), 1, GL_FALSE, glm::value_ptr(view));

        if(wire)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if(spin)
            orientation_y = deltaTime * glm::radians(spin_speed);
        else
            orientation_y = 0;

        shader.setVec3("u_color_in", my_color);
        shader.setFloat("u_weight", weight);
        shader.setFloat("u_time", currentFrame * speed);
        shader.setFloat("u_freq", frequency);
        shader.setFloat("u_power", power);
        shader.setFloat("u_harmonics", harmonics);
        
        //GLint color_location = glGetUniformLocation(shader.program, "u_color");
        //glUniform3fv(color_location, 1, &my_color[0]);
        //
        //GLint weight_location = glGetUniformLocation(shader.program, "u_weight");
        //glUniform1f(weight_location, weight);
        //
        //GLint timer_location = glGetUniformLocation(shader.program, "u_time");
        //glUniform1f(timer_location, currentFrame * speed);
        //
        //GLint freq_location = glGetUniformLocation(shader.program, "u_freq");
        //glUniform1f(freq_location, frequency);
        //
        //GLint power_location = glGetUniformLocation(shader.program, "u_power");
        //glUniform1f(power_location, power);
        //
        //GLint harm_location = glGetUniformLocation(shader.program, "u_harmonics");
        //glUniform1f(harm_location, harmonics);

        // cube
        cube_model_mat = glm::rotate(cube_model_mat, orientation_y, glm::vec3(0, 1, 0));
        setup_model(shader, view, cube_model_mat, cube_normal_mat);
        cube.draw();
        
        // sphere
        sphere_model_mat = glm::rotate(sphere_model_mat, orientation_y, glm::vec3(0, 1, 0));
        setup_model(shader, view, sphere_model_mat, sphere_normal_mat);
        sphere.draw();

        // bunny
        bunny_model_mat = glm::rotate(bunny_model_mat, orientation_y, glm::vec3(0, 1, 0));
        setup_model(shader, view, bunny_model_mat, bunny_normal_mat);
        bunny.draw();

        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we del the Shader program
    //shader.del();
    // we close and del the created context
    glfwTerminate();
    return 0;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if(key == GLFW_KEY_P && action == GLFW_PRESS)
        spin = !spin;

    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wire = !wire;
}*/
