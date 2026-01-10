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

GLfloat cameraSpeed = 20.0f;

GLboolean pressedKeys[1024];

glm::vec3 ImperialFleetPositionDefault(0.0f, -10.0f, -100.0f);
glm::vec3 xwingFleetPositionDefault(0.0f, 0.0f, 500.0f);
glm::vec3 cruiserFleetPositionDefault(0.0f, 20.0f, 950.0f);
glm::vec3 awingFleetPositionDefault(0.0f, 0.0f, 880.0f);

glm::vec3 ImperialFleetPosition(0.0f, -10.0f, -100.0f);
GLfloat capitalShipSpeed = 0.1f;

glm::vec3 xwingFleetPosition(0.0f, 0.0f, 500.0f);
GLfloat smallShipSpeed = 0.7f;

glm::vec3 cruiserFleetPosition(0.0f, 20.0f, 950.0f);

glm::vec3 awingFleetPosition(0.0f, 0.0f, 880.0f);

glm::vec3 sunPosition = glm::vec3(3000.0f, 3000.0f, 3000.0f);
glm::vec3 lightPosition = glm::vec3(2500.0f, 2500.0f, 2500.0f);

gps::Model3D laserBeam; // Load your beam object here
float beamProgress = 0.0f; // 0.0 = At Death Star, 1.0 = Hit Cruiser
glm::vec3 dsDishPos(130.0f, 200.0f, -800.0f); // Adjust to match your Death Star model dish

// SHIP DATA STRUCTURE
enum ShipState {
    APPROACHING,
    EVASIVE_TURN,
    EVASIVE_RUN,
    STABILIZE,
    RETURNING,   // Flying back to Rebel Fleet
    RESET_TURN   // Turning around to start a new run
};

struct Ship {
    glm::vec3 position; // World Position
    glm::vec3 rotation; // Euler Angles: x=Pitch, y=Yaw, z=Roll
    float speed;        // Individual speed
    ShipState state = APPROACHING;
    float stateTimer = 0.0f;
};

// Global Fleets
std::vector<Ship> xWings;
std::vector<Ship> aWings;

// Flags
bool shipsInitialized = false;
bool isSpaceHeld = false;
double lastFrameTime = 0.0;

// models
gps::Model3D sun;
//Empire
gps::Model3D isd1;
gps::Model3D deathStar;

//Rebel Alliance
gps::Model3D xwing;
gps::Model3D awing;
gps::Model3D cruiser;
gps::Model3D transport;
gps::Model3D explosion;

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

void initFighterFleets() {
    if (shipsInitialized) return; // Only run once

    // --- 1. SETUP X-WINGS (5 Squadrons of 5) ---
    glm::vec3 xWingOffsets[] = {
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-15.0f, 0.0f, 15.0f),
        glm::vec3(15.0f, 0.0f, 15.0f), glm::vec3(-30.0f, 0.0f, 30.0f), glm::vec3(30.0f, 0.0f, 30.0f)
    };
    glm::vec3 xSquadCenters[] = {
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-150.0f, 30.0f, 50.0f),
        glm::vec3(150.0f, 30.0f, 50.0f), glm::vec3(-150.0f, -30.0f, 50.0f), glm::vec3(150.0f, -30.0f, 50.0f)
    };

    for (int s = 0; s < 5; s++) {
        for (int i = 0; i < 5; i++) {
            Ship sX;
            // Start Position: Default Fleet Pos + Squadron + Offset
            sX.position = glm::vec3(0.0f, 0.0f, 500.0f) + xSquadCenters[s] + xWingOffsets[i];

            // Start Rotation: Face the Empire (-Z direction, which is 180 degrees Yaw)
            sX.rotation = glm::vec3(0.0f, glm::radians(180.0f), 0.0f);

            // Initial Banking for formation look
            if (s == 1 || s == 3) sX.rotation.z = glm::radians(-15.0f);
            if (s == 2 || s == 4) sX.rotation.z = glm::radians(15.0f);

            sX.speed = 4.0f; // Small Ship Speed
            xWings.push_back(sX);
        }
    }

    // --- SETUP A-WINGS (3 Squadrons of 3) ---
    glm::vec3 aOffsets[] = { glm::vec3(-10.0f, 5.0f, -10.0f), glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(10.0f, 5.0f, 10.0f) };
    glm::vec3 aCenters[] = { glm::vec3(-100.0f, 80.0f, 0.0f), glm::vec3(100.0f, 80.0f, 0.0f), glm::vec3(0.0f, 120.0f, 50.0f) };

    for (int s = 0; s < 3; s++) {
        for (int i = 0; i < 3; i++) {
            Ship sA;
            sA.position = glm::vec3(0.0f, 0.0f, 880.0f) + aCenters[s] + aOffsets[i];
            sA.rotation = glm::vec3(0.0f, glm::radians(180.0f), 0.0f);

            if (s == 0) sA.rotation.z = glm::radians(-30.0f);
            if (s == 1) sA.rotation.z = glm::radians(30.0f);

            sA.speed = 5.0f;
            aWings.push_back(sA);
        }
    }
    shipsInitialized = true;
}

void updateShips(float deltaTime, bool fightActive) {

    auto updateFleet = [&](std::vector<Ship>& fleet, bool canDoManeuver) {

        // Define boundaries
        float empireLine = ImperialFleetPosition.z + 50.0f; // Where they turn back
        float rebelLine = cruiserFleetPosition.z - 50.0f;     // Where they reset (Assuming Rebel fleet is at +Z)

        for (auto& ship : fleet) {
            ship.stateTimer += deltaTime;

            switch (ship.state) {

                // PHASE 0: The Approach (Flying -Z towards Empire)
            case APPROACHING:
            {
                // Random Jitter (Only if fighting)
                if (fightActive) {
                    float rPitch = ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
                    float rYaw = ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
                    float rRoll = ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
                    ship.rotation.x += rPitch * deltaTime * 2.5f;
                    ship.rotation.y += rYaw * deltaTime * 2.5f;
                    ship.rotation.z += rRoll * deltaTime * 4.0f;
                }

                // Move forward based on nose direction
                glm::vec3 forward;
                forward.x = sin(ship.rotation.y) * cos(ship.rotation.x);
                forward.y = -sin(ship.rotation.x);
                forward.z = cos(ship.rotation.y) * cos(ship.rotation.x);
                forward = glm::normalize(forward);
                ship.position += forward * (ship.speed * 60.0f * deltaTime);

                // Check Arrival at Empire
                if (canDoManeuver && ship.position.z < empireLine) {
                    ship.state = EVASIVE_TURN;
                    ship.stateTimer = 0.0f;
                }
            }
            break;

            // PHASE 1: Turn 180 to face Home
            case EVASIVE_TURN:
            {
                // Rotate Yaw 180 degrees (PI)
                ship.rotation.y += 3.14159f * deltaTime; // 1 second turn
                ship.position.z -= 10.0f * deltaTime;    // Momentum drift

                if (ship.stateTimer >= 1.0f) {
                    ship.state = EVASIVE_RUN;
                    ship.stateTimer = 0.0f;
                }
            }
            break;

            // PHASE 2: Attack Run (Jittery escape)
            case EVASIVE_RUN:
            {
                ship.position.z += (ship.speed * 80.0f) * deltaTime; // Fly +Z

                // Jitter
                ship.rotation.x += sin(glfwGetTime() * 15.0f) * 3.0f * deltaTime;
                ship.rotation.z += cos(glfwGetTime() * 10.0f) * 3.0f * deltaTime;

                if (ship.stateTimer >= 2.0f) {
                    ship.state = STABILIZE;
                    ship.stateTimer = 0.0f;
                }
            }
            break;

            // PHASE 3: Stabilize
            case STABILIZE:
            {
                float lerpSpeed = 2.0f * deltaTime;
                ship.rotation.x = ship.rotation.x * (1.0f - lerpSpeed);
                ship.rotation.z = ship.rotation.z * (1.0f - lerpSpeed);

                ship.position.z += (ship.speed * 80.0f) * deltaTime;

                if (ship.stateTimer >= 1.5f) {
                    ship.state = RETURNING; // Switch to returning mode
                    ship.stateTimer = 0.0f;
                }
            }
            break;

            // PHASE 4: Return to Fleet (Flying +Z smoothly)
            case RETURNING:
            {
                // Just fly straight towards Rebel Fleet (+Z)
                ship.position.z += (ship.speed * 80.0f) * deltaTime;

                // Check Arrival at Rebel Fleet
                // Note: Assuming Rebel fleet is at Positive Z (e.g. 300)
                if (ship.position.z > rebelLine) {
                    ship.state = RESET_TURN;
                    ship.stateTimer = 0.0f;
                }
            }
            break;

            // PHASE 5: Reset Turn (Turn 180 to face Empire again)
            case RESET_TURN:
            {
                // Rotate Yaw 180 degrees (PI) to face -Z again
                ship.rotation.y += 3.14159f * deltaTime;

                // Slight forward momentum so they don't stop dead
                ship.position.z += 10.0f * deltaTime;

                if (ship.stateTimer >= 1.0f) {
                    // RESTART THE LOOP
                    ship.state = APPROACHING;
                    ship.stateTimer = 0.0f;

                    // Optional: Reset Pitch/Roll to perfectly 0 to clean up accumulation errors
                    ship.rotation.x = 0.0f;
                    ship.rotation.z = 0.0f;
                }
            }
            break;
            }
        }
        };

    updateFleet(xWings, true);
    updateFleet(aWings, true);
}

void processMovement() {
    if (!shipsInitialized) initFighterFleets();

    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastFrameTime);
    lastFrameTime = currentTime;

    if (pressedKeys[GLFW_KEY_W]) myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_S]) myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    if (pressedKeys[GLFW_KEY_A]) myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_D]) myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    }
    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    }

    if (pressedKeys[GLFW_KEY_SPACE]) {
        isSpaceHeld = true;
        updateShips(deltaTime, true);
        ImperialFleetPosition.z += capitalShipSpeed;
        cruiserFleetPosition.z -= capitalShipSpeed;
        if (beamProgress < 1.0f) {
            beamProgress += 0.3f * deltaTime;
        }
    }
    else {
        isSpaceHeld = false;
    }

    if (pressedKeys[GLFW_KEY_K]) {
        beamProgress = 0.0f;
        ImperialFleetPosition = ImperialFleetPositionDefault;
        cruiserFleetPosition = cruiserFleetPositionDefault;

        // Reset Fleets
        xWings.clear();
        aWings.clear();
        shipsInitialized = false;

        isSpaceHeld = false;
    }

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

void renderSuperlaser(gps::Shader shader) {
    shader.useShaderProgram();

    // 1. Define Start and End Points
    glm::vec3 start = dsDishPos;
    glm::vec3 target = cruiserFleetPosition; // Center Cruiser

    // 2. Calculate the "Tip" of the beam (Current location of the energy)
    // This is Linear Interpolation (Lerp)
    glm::vec3 currentTip = start + (target - start) * beamProgress;

    // 3. SEND LIGHT TO SHADER
    // The light source is at the tip of the beam
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLightPos"), 1, glm::value_ptr(currentTip));

    // Powerful Green Light (RGB: 0, 5, 0) -> Values > 1 make it look glowing/bright
    glm::vec3 greenColor = glm::vec3(0.0f, 5.0f, 0.0f);

    // Only turn the light on if we are firing
    if (beamProgress <= 0.01f) greenColor = glm::vec3(0.0f);
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(greenColor));

    // 4. DRAW THE BEAM MESH
    if (beamProgress > 0.01f) {
        glm::mat4 modelBeam = glm::mat4(1.0f);

        // A. Move to Start Position
        modelBeam = glm::translate(modelBeam, start);

        // B. Rotate to face the Target
        // Calculate the direction vector
        glm::vec3 direction = glm::normalize(target - start);

        // Create a rotation matrix that looks at 'direction' from 'start'
        // We use a helper or glm::lookAt inverse logic.
        // Quick/Dirty way: Use LookAt but invert it because LookAt is for View matrices
        glm::mat4 rotation = glm::inverse(glm::lookAt(start, target, glm::vec3(0, 1, 0)));
        // Extract just the rotation part (remove translation from that matrix)
        rotation[3] = glm::vec4(0, 0, 0, 1);
        modelBeam = modelBeam * rotation;

        // C. Rotate cylinder to align with Z-axis (Models usually point UP (Y), we need them to point Forward (Z))
        // Adjust this depending on your model's default orientation!
        modelBeam = glm::rotate(modelBeam, glm::radians(-90.0f), glm::vec3(1, 0, 0));

        // D. Scale the length
        // Calculate total distance
        //float totalDist = glm::distance(start, target);
        float currentLength = glm::distance(start, target) * beamProgress;
        modelBeam = glm::scale(modelBeam, glm::vec3(5.0f, currentLength / 2.5f, 5.0f));
        modelBeam = glm::translate(modelBeam, glm::vec3(0.0f, 1.0f, 0.0f));
        // Send Uniforms
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelBeam));
        glm::mat3 normalMatrixBeam = glm::mat3(glm::inverseTranspose(view * modelBeam));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixBeam));

        laserBeam.Draw(shader);
    }
}

void renderSun(gps::Shader shader) {
    shader.useShaderProgram();

    glm::mat4 modelSun = glm::mat4(1.0f);

    // Position: Far away along the light direction vector
    modelSun = glm::translate(modelSun, sunPosition);

    // Scale: Make it huge (Adjust 100.0f if your model is too small/big)
    modelSun = glm::scale(modelSun, glm::vec3(100.0f));

    // Rotation 
    // modelSun = glm::rotate(modelSun, glm::radians(angle * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSun));

    glm::mat3 normalMatrixSun = glm::mat3(glm::inverseTranspose(view * modelSun));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixSun));

    sun.Draw(shader);
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
    faces.push_back("skybox/space_right1.tga");
    faces.push_back("skybox/space_left2.tga");
    faces.push_back("skybox/space_top3.tga");
    faces.push_back("skybox/space_bottom4.tga");
    faces.push_back("skybox/space_front5.tga");
    faces.push_back("skybox/space_back6.tga");
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

    //sun.LoadModel("models/sun/sun.obj");
    //Empire
    isd1.LoadModel("models/isd2/Imperial-Class-StarDestroyer.obj");
    deathStar.LoadModel("models/ds4/death_star_ii.obj");

    //Rebel Aliance
    xwing.LoadModel("models/xwing4/x-wing.obj");
    awing.LoadModel("models/awing/A-Wing.obj");
    cruiser.LoadModel("models/cruiser/cruiser.obj");
    laserBeam.LoadModel("models/beam2/beam.obj");
    explosion.LoadModel("models/explosion/source/explosion.obj");
    //transport.LoadModel("models/transport/transport.obj");
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
                               0.1f, 10000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
    lightDir = glm::normalize(lightPosition);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void renderXWings(gps::Shader shader) {
    shader.useShaderProgram();

    for (const auto& ship : xWings) {
        glm::mat4 modelX = glm::mat4(1.0f);

        // Position
        modelX = glm::translate(modelX, ship.position);

        // Rotation (Order: Yaw -> Pitch -> Roll)
        modelX = glm::rotate(modelX, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        modelX = glm::rotate(modelX, ship.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        modelX = glm::rotate(modelX, ship.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        modelX = glm::rotate(modelX, ship.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        // Scale
        // modelX = glm::scale(modelX, glm::vec3(0.6f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelX));
        glm::mat3 normalMatrixX = glm::mat3(glm::inverseTranspose(view * modelX));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixX));
        xwing.Draw(shader);
    }
}

void renderAWings(gps::Shader shader) {
    shader.useShaderProgram();

    for (const auto& ship : aWings) {
        glm::mat4 modelA = glm::mat4(1.0f);

        modelA = glm::translate(modelA, ship.position);
        modelA = glm::rotate(modelA, ship.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        modelA = glm::rotate(modelA, ship.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        modelA = glm::rotate(modelA, ship.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        modelA = glm::scale(modelA, glm::vec3(0.4f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelA));
        glm::mat3 normalMatrixA = glm::mat3(glm::inverseTranspose(view * modelA));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixA));
        awing.Draw(shader);
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
        glm::mat4 modelM = glm::mat4(1.0f);
        
        // position updates
        modelM = glm::translate(modelM, cruiserFleetPosition + cruisersOffsets[i]);

        modelM = glm::scale(modelM, glm::vec3(0.05f));
        modelM = glm::rotate(modelM, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelM));
        glm::mat3 normalMatrixM = glm::mat3(glm::inverseTranspose(view * modelM));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixM));cruiser.Draw(shader);
    }
}

void renderISD1(gps::Shader shader) {
    shader.useShaderProgram();

    // Spacing offsets
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

        isdModel = glm::translate(isdModel, ImperialFleetPosition + formationOffsets[i]);

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

    // Position
    modelDS = glm::translate(modelDS, glm::vec3(0.0f, 50.0f, -800.0f));

    // Rotation
    modelDS = glm::rotate(modelDS, glm::radians(-70.0f + (angle / 10.0f)), glm::vec3(0.0f, 1.0f, 0.0f));

    // Send uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDS));

    glm::mat3 normalMatrixDS = glm::mat3(glm::inverseTranspose(view * modelDS));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixDS));

    deathStar.Draw(shader);
}

void renderDebugDeathStar(gps::Shader shader) {
    shader.useShaderProgram();

    glm::mat4 modelDDS = glm::mat4(1.0f);

    // Position: Exactly halfway between the Sun (3000) and the Scene (0)
    modelDDS = glm::translate(modelDDS, glm::vec3(1500.0f, 1500.0f, 1500.0f));

    // Scale: Reasonable size to see it clearly
    //modelDDS = glm::scale(modelDDS, glm::vec3(5.0f));

    // Send Uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDDS));

    glm::mat3 normalMatrixDDS = glm::mat3(glm::inverseTranspose(view * modelDDS));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixDDS));

    // Reuse the existing deathStar model
    deathStar.Draw(shader);
}

void renderExplosion(gps::Shader shader) {
    shader.useShaderProgram();

    glm::mat4 modelDDS = glm::mat4(1.0f);

    modelDDS = glm::translate(modelDDS, glm::vec3(0.0f, 0.0f, 0.0f));

    // Scale: Reasonable size to see it clearly
    modelDDS = glm::scale(modelDDS, glm::vec3(5.0f));

    // Send Uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDDS));

    glm::mat3 normalMatrixDDS = glm::mat3(glm::inverseTranspose(view * modelDDS));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixDDS));

    // Reuse the existing deathStar model
    explosion.Draw(shader);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene
    //renderSun(myBasicShader);
	// render the skybox
    mySkyBox.Draw(skyboxShader, view, projection);
    //Empire
    renderISD1(myBasicShader);
    renderDeathStar(myBasicShader);
    //renderDebugDeathStar(myBasicShader);
    //Rebel Alliance
    renderXWings(myBasicShader);
    renderCruisers(myBasicShader);
    renderAWings(myBasicShader);
    renderSuperlaser(myBasicShader);
    renderExplosion(myBasicShader);
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
