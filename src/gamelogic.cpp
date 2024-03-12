#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;

double ballRadius = 3.0f;

// Mathias
SceneNode* light1;
SceneNode* light2;
SceneNode* light3;

//Mathias2
SceneNode* UIText;

//Mathias
glm::mat4 VP; //Initialize it here so it kan be used in different methods

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

//// A few lines to help you if you've never used c++ structs
// struct LightSource {
//     bool a_placeholder_value;
// };
// LightSource lightSources[/*Put number of light sources you want here*/];

//Mathias2
PNGImage ASCII = loadPNGFile("../res/textures/charmap.png");

//Mathias2
PNGImage diffuse = loadPNGFile("../res/textures/Brick03_col.png");
PNGImage normalMaped = loadPNGFile("../res/textures/Brick03_nrm.png");

//Mathias2 bonus task
PNGImage roughness = loadPNGFile("../res/textures/Brick03_rgh.png");

//Mathias2
int createTextureID(PNGImage image) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    return id;
};



void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    ballNode = createSceneNode();

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);

    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();

    // Mathias 
    light1 = createSceneNode();
    light2 = createSceneNode();
    light3 = createSceneNode();

    // Mathias
    padNode->children.push_back(light1);
    rootNode->children.push_back(light2);
    rootNode->children.push_back(light3);

    //Mathias
    light1->position = glm::vec3(0.0, 50.0, 0.0);
    light2->position = glm::vec3(0.0, 0.0, -70.0);
    light3->position = glm::vec3(-20.0, 0.0, -70.0);

    //Mathias
    light1->ID = 1;
    light2->ID = 2;
    light3->ID = 3;

    //Mathias2
    int textureGen = createTextureID(ASCII);
    std::string word = "Hello there";
    Mesh genTextBuffer = generateTextGeometryBuffer(word, float(39.0/29.0), float(word.length()) * 29.0);

    // Mathias2
    UIText = createSceneNode();
    UIText->nodeType = TWOD_GEOMETRY;

    // Mathias2
    rootNode->children.push_back(UIText);

    //Mathias2
    UIText->textureID = textureGen;

    //Mathias2
    unsigned int UIVAO = generateBuffer(genTextBuffer);
    UIText->vertexArrayObjectID = UIVAO;
    UIText->VAOIndexCount = genTextBuffer.indices.size();

    //Mathias2
    boxNode->nodeType = NORM_MAP_GEO;

    //Mathias2
    boxNode->textureID = createTextureID(diffuse);
    boxNode->normMapTextureID = createTextureID(normalMaped);

    //Mathias bonus task
    boxNode->roughnessID = createTextureID(roughness);
    
    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ
                ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                }
            }
        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    //Mathias
    glUniform3fv(10, 1, glm::value_ptr(cameraPosition));

    //Mathias2
    glm::mat4 orthographic = glm::ortho(0.0, 1366.0, 0.0, 768.0, -0.1, 350.0);
    glUniformMatrix4fv(13, 1, GL_FALSE, glm::value_ptr(orthographic));

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    //Mathias
    //glm::mat4 VP = projection * cameraTransform; //original

    //Mathias
    VP = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    padNode->position  = {
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    //Mathias
    //updateNodeTransformations(rootNode, VP); //original

    //Mathias
    glm::mat4 identityMatrix = glm::mat4(1.0f); //Exchange the VP for an identity matrix so that only the Model transforms will be kept in the updateNodeTransformations()
    updateNodeTransformations(rootNode, identityMatrix);

}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

    // Mathias
    glUniform4fv(6, 1, glm::value_ptr(light1->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
    glUniform4fv(7, 1, glm::value_ptr(light2->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
    glUniform4fv(8, 1, glm::value_ptr(light3->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));

    GLint colorLocation = shader->getUniformFromName("anArrayOfStructs[0].lightColor");
    GLint lighLlocation = shader->getUniformFromName("anArrayOfStructs[0].lightPos");
    glUniform3fv(colorLocation, 1, glm::value_ptr(glm::vec3(0.99, 0.99, 0.99)));
    glUniform3fv(lighLlocation, 1, glm::value_ptr(glm::vec3(light1->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))));

    GLint colorLocation2 = shader->getUniformFromName("anArrayOfStructs[1].lightColor");
    GLint lighLlocation2 = shader->getUniformFromName("anArrayOfStructs[1].lightPos");
    glUniform3fv(colorLocation2, 1, glm::value_ptr(glm::vec3(0.0, 0.99, 0.0)));
    glUniform3fv(lighLlocation2, 1, glm::value_ptr(glm::vec3(light2->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))));

    GLint colorLocation3 = shader->getUniformFromName("anArrayOfStructs[2].lightColor");
    GLint lighLlocation3 = shader->getUniformFromName("anArrayOfStructs[2].lightPos");
    glUniform3fv(colorLocation3, 1, glm::value_ptr(glm::vec3(0.0, 0.0, 0.99)));
    glUniform3fv(lighLlocation3, 1, glm::value_ptr(glm::vec3(light3->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))));

    GLint colorLocationText = shader->getUniformFromName("textStruct[0].textColor");
    GLint textPosLocation = shader->getUniformFromName("textStruct[0].textPos");
    glUniform3fv(colorLocationText, 1, glm::value_ptr(glm::vec3(1.0, 0.0, 0.0)));
    glUniform3fv(textPosLocation, 1, glm::value_ptr(glm::vec3(UIText->currentTransformationMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))));
    

    switch(node->nodeType) {
        case GEOMETRY: break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

void renderNode(SceneNode* node) {
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));

    //Mathias
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(VP)); //View Projection
    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix)); //Model
    //Mathias
    glUniformMatrix3fv(9, 1, GL_FALSE, glm::value_ptr(glm::mat3(transpose(inverse(node->currentTransformationMatrix))))); //transposeInverseModelMatrix

    //Mathias
    glUniform3fv(11, 1, glm::value_ptr(ballPosition));
    //printf("%f,%f,%f\n", ballPosition.x, ballPosition.y, ballPosition.z); //for help during the assignment


    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glUniform1i(14, 0);
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
        //Mathias2
        case TWOD_GEOMETRY:
            if (node->vertexArrayObjectID != -1) {
                glUniform1i(14, 1);
                glBindVertexArray(node->vertexArrayObjectID);
                glBindTextureUnit(0, node->textureID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        //Mathias2
        case NORM_MAP_GEO:
            if (node->vertexArrayObjectID != -1) {
                glUniform1i(14, 2);
                glBindVertexArray(node->vertexArrayObjectID);
                glBindTextureUnit(1, node->textureID);
                glBindTextureUnit(2, node->normMapTextureID);
                glBindTextureUnit(3, node->roughnessID); //bonus task
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    renderNode(rootNode);
}
