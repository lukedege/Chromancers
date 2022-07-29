#pragma once

#include <string>

// Loader estensions OpenGL
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
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

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// classes developed during lab lectures to manage shaders and to load models
#include "utils/shader.h"
#include "utils/model.h "
#include "utils/camera.h"
#include "utils/object.h"
#include "utils/light.h "

// OpenGL version
GLuint glMajor = 4, glMinor = 3;

// dimensions of application's window
GLuint screenWidth = 1200, screenHeight = 900;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void process_input();

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
std::vector<std::string> shader_subroutines;

// the name of the subroutines are searched in the shaders, and placed in the shaders vector (to allow shaders swapping)
void SetupShader(int shader_program);

// print on console the name of current shader subroutine
void PrintCurrentShader(int subroutine);

GLfloat lastX, lastY;
bool firstMouse = true;

bool keys[1024];

Camera camera(glm::vec3(0, 0, 7), GL_TRUE);

// parameters for time computation
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// rotation angle on Y axis
GLfloat orientationY = 0.0f;
// rotation speed on Y axis
GLfloat spin_speed = 30.0f;
// boolean to start/stop animated rotation on Y angle
GLboolean spinning = GL_TRUE;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// Uniforms to pass to shaders
glm::vec3 lightPos0{ 5.f, 10.f, 10.f }; // Point light
GLfloat mov_light_speed = 30.f;
//for a directional light we need to specify another vec3 for position; 
//for a colored light we need to specify another vec3 for color

GLfloat diffuseColor[] = { 1.0f, 0.0f, 0.0f };
GLfloat specularColor[] = { 1.0f, 1.0f, 1.0f };
GLfloat ambientColor[] = { 0.1f, 0.1f, 0.1f };

// Generally we'd like a normalized sum of these coefficients Kd + Ks + Ka = 1
GLfloat Kd = 0.5f;
GLfloat Ks = 0.4f;
GLfloat Ka = 0.1f;

GLfloat shininess = 25.f;

GLfloat alpha = 0.2f;
GLfloat F0 = 0.9f;

// color to be passed as uniform to the shader of the plane
GLfloat planeColor[] = { 0.0,0.5,0.0 };

// Debug callbacks in a debug context are available from OpenGL 4.3+
inline void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* param) 
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_work04", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
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
    

    // Init debug callbacks
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        //glDebugMessageCallback(debugCallback, NULL);
        //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }
    

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);
   
    // we create the Shader Program used for the plane
    std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

    Shader light_shader{ "shaders/procedural_base.vert", "shaders/illumination_models.frag", utils_shaders, glMajor, glMinor };

    // we create the Shader Program used for objects (which presents different subroutines we can switch)
    // we parse the Shader Program to search for the number and names of the subroutines.
    // the names are placed in the shaders vector
    SetupShader(light_shader.program); //TODO move subroutine setup and handling in the shader class
    // we print on console the name of the first subroutine used
    PrintCurrentShader(current_subroutine);

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);
    // View matrix (=camera): position, view direction, camera "up" vector
    glm::mat4 view = glm::mat4(1);

    // Setup objects
    Object plane{"models/plane.obj"}, sphere{"models/sphere.obj"}, cube{"models/cube.obj"}, bunny{"models/bunny.obj"};

    // Setup lights
    glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 0.0f, 0.0f }, specular{ 1.0f, 1.0f, 1.0f };
    //GLfloat kA, kD, kS;
    LightAttributes la{ ambient, diffuse, specular, Ka, Kd, Ks };
    PointLight pl1{ glm::vec3{ 20.f, 10.f, 10.f}, la };
    PointLight pl2{ glm::vec3{-20.f, 10.f, 10.f}, la };

    std::vector<PointLight> pls{};
    pls.push_back(pl1);
    pls.push_back(pl2);

    // Rendering loop: this code is executed at each frame
    while (!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate the time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();
        process_input();
        view = camera.GetViewMatrix();

        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // if animated rotation is activated, than we increment the rotation angle using delta time and the rotation speed parameter
        if (spinning)
            orientationY += (deltaTime * spin_speed);

        /////////////////// PLANE ////////////////////////////////////////////////
        // We render a plane under the objects. We apply the fullcolor shader to the plane, and we do not apply the rotation applied to the other objects.
        light_shader.use();

        light_shader.setUint("nPointLights", pls.size());
        for (size_t i = 0; i < pls.size(); i++)
        {
            pls[i].setup(light_shader, i);
        }

        GLuint index = glGetSubroutineIndex(light_shader.program, GL_FRAGMENT_SHADER, "Lambert");
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &index);

        // we create the transformation matrix
        plane.translate(glm::vec3(0.0f, -1.0f, 0.0f));
        plane.scale(glm::vec3(10.0f, 1.0f, 10.0f));

        // we render the plane
        plane.draw(light_shader, view);

        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the light_shader Shader Program as part of the current rendering process
        light_shader.use();
        
        if(!shader_subroutines.empty())
        { 
            // we search inside the Shader Program the name of the subroutine currently selected, and we get the numerical index
            index = glGetSubroutineIndex(light_shader.program, GL_FRAGMENT_SHADER, shader_subroutines[current_subroutine].c_str());
            // we activate the subroutine using the index (this is where shaders swapping happens)
            glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &index);
        }

        light_shader.setFloat("shininess", shininess);
        light_shader.setFloat("alpha", alpha);

        // we pass projection and view matrices to the Shader Program
        light_shader.setMat4("projectionMatrix", projection);
        light_shader.setMat4("viewMatrix", view);

        // SPHERE
        sphere.translate (glm::vec3(-3.0f, 0.0f, 0.0f));
        sphere.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
        sphere.scale     (glm::vec3(0.8f));

        sphere.draw(light_shader, view);

        //CUBE
        cube.translate (glm::vec3(0.0f, 0.0f, 0.0f));
        cube.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
        cube.scale     (glm::vec3(0.8f));	// It's a bit too big for our scene, so scale it down

        cube.draw(light_shader, view);

        //BUNNY
        bunny.translate (glm::vec3(3.0f, 0.0f, 0.0f));
        bunny.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
        bunny.scale     (glm::vec3(0.3f));	// It's a bit too big for our scene, so scale it down

        bunny.draw(light_shader, view);

        // light following camera
        //lightPos0 = camera.position();

        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we close and delete the created context
    glfwTerminate();
    return 0;
}


//////////////////////////////////////////
// The function parses the content of the Shader Program, searches for the Subroutine type names,
// the subroutines implemented for each type, print the names of the subroutines on the terminal, and add the names of
// the subroutines to the shaders vector, which is used for the shaders swapping
void SetupShader(int program)
{
    int maxSub = 0, maxSubU = 0, countActiveSU = 0;
    GLchar name[256];
    int len = 0, numCompS = 0;

    // global parameters about the Subroutines parameters of the system
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSub);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubU);
    std::cout << "Max Subroutines:" << maxSub << " - Max Subroutine Uniforms:" << maxSubU << std::endl;

    // get the number of Subroutine uniforms (only for the Fragment shader, due to the nature of the exercise)
    // it is possible to add similar calls also for the Vertex shader
    glGetProgramStageiv(program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);

    // print info for every Subroutine uniform
    for (int i = 0; i < countActiveSU; i++) {

        // get the name of the Subroutine uniform (in this example, we have only one)
        glGetActiveSubroutineUniformName(program, GL_FRAGMENT_SHADER, i, 256, &len, name);
        // print index and name of the Subroutine uniform
        std::cout << "Subroutine Uniform: " << i << " - name: " << name << std::endl;

        // get the number of subroutines
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);

        // get the indices of the active subroutines info and write into the array s
        int* s = new int[numCompS];
        glGetActiveSubroutineUniformiv(program, GL_FRAGMENT_SHADER, i, GL_COMPATIBLE_SUBROUTINES, s);
        std::cout << "Compatible Subroutines:" << std::endl;

        // for each index, get the name of the subroutines, print info, and save the name in the shaders vector
        for (int j = 0; j < numCompS; ++j) {
            glGetActiveSubroutineName(program, GL_FRAGMENT_SHADER, s[j], 256, &len, name);
            std::cout << "\t" << s[j] << " - " << name << "\n";
            shader_subroutines.push_back(name);
        }
        std::cout << std::endl;

        delete[] s;
    }
}

//////////////////////////////////////////
// we print on console the name of the currently used shader subroutine
void PrintCurrentShader(int subroutine)
{
    if(!shader_subroutines.empty())
        std::cout << "Current shader subroutine: " << shader_subroutines[subroutine] << std::endl;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    GLuint new_subroutine;

    // if ESC is pressed, we close the application
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if P is pressed, we start/stop the animated rotation of models
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        spinning = !spinning;

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe = !wireframe;

    // pressing a key number, we change the shader applied to the models
    // if the key is between 1 and 9, we proceed and check if the pressed key corresponds to
    // a valid subroutine
    if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_9) && action == GLFW_PRESS)
    {
        // "1" to "9" -> ASCII codes from 49 to 59
        // we subtract 48 (= ASCII CODE of "0") to have integers from 1 to 9
        // we subtract 1 to have indices from 0 to 8
        new_subroutine = (key - '0' - 1);
        // if the new index is valid ( = there is a subroutine with that index in the shaders vector),
        // we change the value of the current_subroutine variable
        // NB: we can just check if the new index is in the range between 0 and the size of the shaders vector,
        // avoiding to use the std::find function on the vector
        if (new_subroutine < shader_subroutines.size())
        {
            current_subroutine = new_subroutine;
            PrintCurrentShader(current_subroutine);
        }
    }

    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;



}

void process_input()
{
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(camdir::FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(camdir::BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(camdir::LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(camdir::RIGHT, deltaTime);

    if (keys[GLFW_KEY_LEFT])
        lightPos0.x -= mov_light_speed * deltaTime;
    if (keys[GLFW_KEY_RIGHT])
        lightPos0.x += mov_light_speed * deltaTime;
    if (keys[GLFW_KEY_UP])
        lightPos0.z -= mov_light_speed * deltaTime;
    if (keys[GLFW_KEY_DOWN])
        lightPos0.z += mov_light_speed * deltaTime;
}

void mouse_pos_callback(GLFWwindow* window, double x_pos, double y_pos)
{
    if (firstMouse)
    {
        lastX = x_pos;
        lastY = y_pos;
        firstMouse = false;
    }

    GLfloat x_offset = x_pos - lastX;
    GLfloat y_offset = lastY - y_pos;

    lastX = x_pos;
    lastY = y_pos;

    camera.ProcessMouseMovement(x_offset, y_offset);
}