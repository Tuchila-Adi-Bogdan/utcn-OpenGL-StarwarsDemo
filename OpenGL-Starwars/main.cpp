#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include "SkyBox.hpp"

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 10.0f;

GLboolean pressedKeys[1024];

glm::vec3 fleetPosition(0.0f, -10.0f, -100.0f); // Starting position
GLfloat fleetSpeed = 0.5f; // How fast they move

// models
//Empire
gps::Model3D teapot;
gps::Model3D isd1;
gps::Model3D deathStar;

//Rebel Alliance
gps::Model3D xwing;
gps::Model3D awing;
gps::Model3D cruiser;
gps::Model3D transport;

GLfloat angle;

// shaders
gps::Shader myBasicShader;

//skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

bool firstMouse = false;
double lastX, lastY;
float pitch = 0.0f, yaw = -90.0f;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    /*if (yaw > 89.0f)
        yaw = 89.0f;
    if (yaw < -89.0f)
        yaw = -89.0f;*/

    myCamera.rotate(pitch, yaw);

}

void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    }

    if (pressedKeys[GLFW_KEY_SPACE]) {
        fleetPosition.z += fleetSpeed;
    }

    // Update the view matrix every frame
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Update normal matrix (because it depends on View)
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Starwars Project");
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initSkybox()
{
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");
    mySkyBox.Load(faces);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {

    //Empire
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    isd1.LoadModel("models/isd2/Imperial-Class-StarDestroyer.obj");
    deathStar.LoadModel("models/ds4/death_star_ii.obj");

    //Rebel Aliance
    xwing.LoadModel("models/xwing4/x-wing.obj");
    awing.LoadModel("models/awing/A-Wing.obj");
    cruiser.LoadModel("models/cruiser/cruiser.obj");
    //transport.LoadModel("models/falcon/transport.obj");
    //Skybox
    initSkybox();
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 5000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void renderTeapot(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    teapot.Draw(shader);
}

void renderXWings(gps::Shader shader) {
    shader.useShaderProgram();

    // 1. Define the 5-ship "V" shape offsets (Local formation)
    glm::vec3 localFormation[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),    // Leader
        glm::vec3(-15.0f, 0.0f, 15.0f),   // Left Wingman 1
        glm::vec3(15.0f, 0.0f, 15.0f),    // Right Wingman 1
        glm::vec3(-30.0f, 0.0f, 30.0f),   // Left Wingman 2
        glm::vec3(30.0f, 0.0f, 30.0f)     // Right Wingman 2
    };

    // 2. Define the center positions for the 5 different Squadrons
    glm::vec3 squadronCenters[] = {
        glm::vec3(0.0f,   0.0f,   0.0f),    // Alpha Squadron (Center)
        glm::vec3(-150.0f,  30.0f,  50.0f), // Bravo Squadron (High Left)
        glm::vec3(150.0f,  30.0f,  50.0f),  // Charlie Squadron (High Right)
        glm::vec3(-150.0f, -30.0f,  50.0f), // Delta Squadron (Low Left)
        glm::vec3(150.0f, -30.0f,  50.0f)   // Echo Squadron (Low Right)
    };

    // 3. Global base position
    // Moved to 0.0f to create more distance from ISDs (at -100.0f) but still in view
    glm::vec3 rebelFleetBase = glm::vec3(0.0f, 0.0f, 500.0f);

    for (int s = 0; s < 5; s++) {
        for (int i = 0; i < 5; i++) {
            glm::mat4 modelX = glm::mat4(1.0f);

            glm::vec3 finalPos = rebelFleetBase + squadronCenters[s] + localFormation[i];
            modelX = glm::translate(modelX, finalPos);

            // 4. Scale down by 40% (Target size = 0.6)
            modelX = glm::scale(modelX, glm::vec3(0.6f));

            // Banking logic for effect (Optional, keeps them looking dynamic)
            if (s == 1 || s == 3)
                modelX = glm::rotate(modelX, glm::radians(-15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            if (s == 2 || s == 4)
                modelX = glm::rotate(modelX, glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelX));

            glm::mat3 normalMatrixX = glm::mat3(glm::inverseTranspose(view * modelX));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixX));

            xwing.Draw(shader);
        }
    }
}

void renderAWings(gps::Shader shader) {
    shader.useShaderProgram();

    // 1. Define the 3-ship "Slash" pattern (Local formation)
    glm::vec3 slashFormation[] = {
        glm::vec3(-10.0f,  5.0f,  -10.0f), // Wingman 1
        glm::vec3(0.0f, 10.0f,    0.0f), // Leader (Highest)
        glm::vec3(10.0f,  5.0f,   10.0f)  // Wingman 2
    };

    // 2. Define the positions for the 3 separate Squadrons
    glm::vec3 squadronCenters[] = {
        glm::vec3(-100.0f,  80.0f,  0.0f),  // Green Leader (High Left)
        glm::vec3(100.0f,  80.0f,  0.0f),  // Blue Leader (High Right)
        glm::vec3(0.0f, 120.0f, 50.0f)   // Red Leader (Very High Center/Rear)
    };

    // 3. Global Base Position (Moved back to 80.0f)
    glm::vec3 globalBase = glm::vec3(0.0f, 0.0f, 880.0f);

    // Loop through 3 Squadrons
    for (int s = 0; s < 3; s++) {

        // Loop through 3 Ships per Squadron
        for (int i = 0; i < 3; i++) {
            glm::mat4 modelA = glm::mat4(1.0f);

            // Calculate Position
            glm::vec3 finalPos = globalBase + squadronCenters[s] + slashFormation[i];
            modelA = glm::translate(modelA, finalPos);

            // Scale (Tiny interceptors)
            modelA = glm::scale(modelA, glm::vec3(0.4f));

            // Rotate to face enemy (180) + Formation Bank
            modelA = glm::rotate(modelA, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            // Dynamic Banking based on squadron side
            if (s == 0) // Left Squad banks Right
                modelA = glm::rotate(modelA, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            if (s == 1) // Right Squad banks Left
                modelA = glm::rotate(modelA, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            // Send uniforms
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelA));
            glm::mat3 normalMatrixA = glm::mat3(glm::inverseTranspose(view * modelA));
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixA));

            awing.Draw(shader);
        }
    }
}

void renderCruisers(gps::Shader shader) {
    shader.useShaderProgram();

    glm::vec3 cruisersOffsets[] = {
        glm::vec3(-250.0f,  0.0f,  0.0f), // Left Cruiser
        glm::vec3(0.0f,  0.0f,  0.0f), // Center Cruiser
        glm::vec3(250.0f,  0.0f,  0.0f)  // Right Cruiser
    };

    for (int i = 0; i < 3; i++) {
        glm::mat4 modelY = glm::mat4(1.0f);

        glm::vec3 basePos = glm::vec3(0.0f, 20.0f, 950.0f);
        modelY = glm::translate(modelY, basePos + cruisersOffsets[i]);

        // Scale
        modelY = glm::scale(modelY, glm::vec3(0.05f));

        // Rotate: Face the enemy (180 degrees)
        modelY = glm::rotate(modelY, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelY));
        glm::mat3 normalMatrixY = glm::mat3(glm::inverseTranspose(view * modelY));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixY));

        cruiser.Draw(shader);
    }
}

void renderISD1(gps::Shader shader) {
    shader.useShaderProgram();

    // Spacing offsets (Wide formation)
    glm::vec3 formationOffsets[] = {
        glm::vec3(0.0f,  0.0f,   0.0f),
        glm::vec3(-80.0f,  0.0f,  80.0f),
        glm::vec3(80.0f,  0.0f,  80.0f),
        glm::vec3(-160.0f, 0.0f, 160.0f),
        glm::vec3(160.0f, 0.0f, 160.0f),
        glm::vec3(-240.0f, 0.0f, 240.0f),
        glm::vec3(240.0f, 0.0f, 240.0f),
        glm::vec3(0.0f, 40.0f, 120.0f)
    };

    for (int i = 0; i < 8; i++) {
        glm::mat4 isdModel = glm::mat4(1.0f);

        // USE THE GLOBAL VARIABLE HERE:
        // We add the offset to the changing global fleetPosition
        isdModel = glm::translate(isdModel, fleetPosition + formationOffsets[i]);

        isdModel = glm::scale(isdModel, glm::vec3(0.05f));
        isdModel = glm::rotate(isdModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(isdModel));

        glm::mat3 isdNormalMatrix = glm::mat3(glm::inverseTranspose(view * isdModel));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(isdNormalMatrix));

        isd1.Draw(shader);
    }
}

void renderDeathStar(gps::Shader shader) {
    shader.useShaderProgram();

    glm::mat4 modelDS = glm::mat4(1.0f);

    // Position: Far in the background, slightly elevated
    modelDS = glm::translate(modelDS, glm::vec3(0.0f, 50.0f, -800.0f));

    // Scale
    //modelDS = glm::scale(modelDS, glm::vec3(10.0f));

    // Rotation: Slow rotation based on the global 'angle' variable
    // Dividing angle by 10 makes it rotate very slowly and ominously
    modelDS = glm::rotate(modelDS, glm::radians(30.0f + (angle / 10.0f)), glm::vec3(0.0f, 1.0f, 0.0f));

    // Send uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDS));

    glm::mat3 normalMatrixDS = glm::mat3(glm::inverseTranspose(view * modelDS));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixDS));

    deathStar.Draw(shader);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene

	// render the teapot
    mySkyBox.Draw(skyboxShader, view, projection);
	renderTeapot(myBasicShader);
    //Empire
    renderISD1(myBasicShader);
    renderDeathStar(myBasicShader);

    //Rebel Alliance
    renderXWings(myBasicShader);
    renderCruisers(myBasicShader);
    renderAWings(myBasicShader);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) 
{
    bool firstMouse = true;
    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
