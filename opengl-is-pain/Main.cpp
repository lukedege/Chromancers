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
#include "utils/window.h"

namespace ugo = utils::graphics::opengl;

// callback function for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
void process_input();

// index of the current shader subroutine (= 0 in the beginning)
GLuint current_subroutine = 0;
// a vector for all the shader subroutines names used and swapped in the application
std::vector<std::string> shader_subroutines;

// print on console the name of current shader subroutine
void PrintCurrentShader(int subroutine);

GLfloat lastX, lastY;
bool firstMouse = true;

bool keys[1024];

ugo::Camera camera(glm::vec3(0, 0, 7), GL_TRUE);

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

ugo::PointLight* currentLight;

/////////////////// MAIN function ///////////////////////
int main()
{
	ugo::window wdw
	{
		{
			.title			  { "Prova" },
			.gl_version_major { 4 },
			.gl_version_minor { 3 },
			.window_width	  { 1200 },
			.window_height	  { 900 },
			.resizable		  { false },
		}
	};
	
	GLFWwindow* glfw_window = wdw.get();

	//// we put in relation the window and the callbacks
	glfwSetKeyCallback(glfw_window, key_callback);
	glfwSetCursorPosCallback(glfw_window, mouse_pos_callback);
	//
	//// we disable the mouse cursor
	glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   
    // we create the Shader Program used for the plane
    std::vector<const GLchar*> utils_shaders { "shaders/types.glsl", "shaders/constants.glsl" };

	ugo::Shader lightShader{ "shaders/procedural_base.vert", "shaders/illumination_models.frag", utils_shaders, 4, 3};

	shader_subroutines = {lightShader.findSubroutines(GL_FRAGMENT_SHADER)};
    PrintCurrentShader(current_subroutine);

    // Projection matrix: FOV angle, aspect ratio, near and far planes
	auto window_size = wdw.get_size();
    glm::mat4 projection = glm::perspective(45.0f, (float)window_size.first / (float)window_size.second, 0.1f, 10000.0f);
    // View matrix (=camera): position, view direction, camera "up" vector
    glm::mat4 view = glm::mat4(1);

    // Setup objects
    ugo::Object plane{"models/plane.obj"}, sphere{"models/sphere.obj"}, cube{"models/cube.obj"}, bunny{"models/bunny.obj"};

    // Setup lights
    glm::vec3 ambient{ 0.1f, 0.1f, 0.1f }, diffuse{ 1.0f, 0.0f, 0.0f }, specular{ 1.0f, 1.0f, 1.0f };
    //GLfloat kA, kD, kS;
    ugo::LightAttributes la{ ambient, diffuse, specular, Ka, Kd, Ks };
    ugo::PointLight pl1{ glm::vec3{ 20.f, 10.f, 10.f}, la };
    ugo::PointLight pl2{ glm::vec3{-20.f, 10.f, 10.f}, la };

    std::vector<ugo::PointLight> pointLights{};
    pointLights.push_back(pl1);
    pointLights.push_back(pl2);
	currentLight = &pointLights[0];

	GLuint currentSubroutineIndex;
    // Rendering loop: this code is executed at each frame
    while (!glfwWindowShouldClose(glfw_window))
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
        lightShader.use();

        lightShader.setUint("nPointLights", pointLights.size());
        for (size_t i = 0; i < pointLights.size(); i++)
        {
            pointLights[i].setup(lightShader, i);
        }

        currentSubroutineIndex = lightShader.getSubroutineIndex(GL_FRAGMENT_SHADER, "Lambert");
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &currentSubroutineIndex);

        // we create the transformation matrix
        plane.translate(glm::vec3(0.0f, -1.0f, 0.0f));
        plane.scale(glm::vec3(10.0f, 1.0f, 10.0f));

        // we render the plane
        plane.draw(lightShader, view);

        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the light_shader Shader Program as part of the current rendering process
        lightShader.use();
        
        if(current_subroutine < shader_subroutines.size())
        { 
            // we search inside the Shader Program the name of the subroutine currently selected, and we get the numerical index
			currentSubroutineIndex = lightShader.getSubroutineIndex(GL_FRAGMENT_SHADER, shader_subroutines[current_subroutine].c_str());
            // we activate the subroutine using the index (this is where shaders swapping happens)
            glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &currentSubroutineIndex);
        }

        lightShader.setFloat("shininess", shininess);
        lightShader.setFloat("alpha", alpha);

        // we pass projection and view matrices to the Shader Program
        lightShader.setMat4("projectionMatrix", projection);
        lightShader.setMat4("viewMatrix", view);

        // SPHERE
        sphere.translate (glm::vec3(-3.0f, 0.0f, 0.0f));
        sphere.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
        sphere.scale     (glm::vec3(0.8f));

        sphere.draw(lightShader, view);

        //CUBE
        cube.translate (glm::vec3(0.0f, 0.0f, 0.0f));
        cube.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
        cube.scale     (glm::vec3(0.8f));	// It's a bit too big for our scene, so scale it down

        cube.draw(lightShader, view);

        //BUNNY
        bunny.translate (glm::vec3(3.0f, 0.0f, 0.0f));
        bunny.rotate_deg(orientationY, glm::vec3(0.0f, 1.0f, 0.0f));
        bunny.scale     (glm::vec3(0.3f));	// It's a bit too big for our scene, so scale it down

        bunny.draw(lightShader, view);

        glfwSwapBuffers(glfw_window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we close and delete the created context
    //glfwTerminate();
    return 0;
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
        camera.ProcessKeyboard(ugo::camdir::FORWARD, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(ugo::camdir::BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(ugo::camdir::LEFT, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(ugo::camdir::RIGHT, deltaTime);

    if (keys[GLFW_KEY_LEFT])
        currentLight->position.x -= mov_light_speed * deltaTime;
    if (keys[GLFW_KEY_RIGHT])
		currentLight->position.x += mov_light_speed * deltaTime;
    if (keys[GLFW_KEY_UP])
		currentLight->position.z -= mov_light_speed * deltaTime;
    if (keys[GLFW_KEY_DOWN])
		currentLight->position.z += mov_light_speed * deltaTime;
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