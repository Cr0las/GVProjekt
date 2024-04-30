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

#include <assimp/Importer.hpp>  // C++ importer interface Peder!!!
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // aiScene and aiNode
#include <assimp/material.h>    // aiMaterial
#include <assimp/mesh.h>        // aiMesh
#include <assimp/texture.h>     // aiTexture

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;

glm::mat4 VP; //Initialize it here so it kan be used in different methods

const aiScene* fighter;
const aiScene* bumblebee;
const aiScene* bullet;
const aiScene* hawk;
const aiScene* wall;

aiMesh* mesh;

aiMesh* mesh2;

aiMesh* mesh3;

aiMesh* mesh4;

aiMesh* mesh5;

SceneNode* bulletNode;

SceneNode* blenderObjNode;

const glm::vec3 UP = glm::vec3(0.1, 0.9, 0);
const glm::vec3 DOWN = glm::vec3(-0.1, -0.9, 0);
const glm::vec3 LEFT = glm::vec3(-0.9, 0.1, 0);
const glm::vec3 RIGHT = glm::vec3(0.9, -0.1, 0);
const glm::vec3 FORWARD = glm::vec3(0, 0, 1);
const glm::vec3 BACKWARD = glm::vec3(0, 0, -1);
const glm::vec3 NEUTRAL = glm::vec3(0, 0, 0);

const int amountOfBoids = 4;

//Assimp
// From chat GPT and here: https://nickthecoder.wordpress.com/2013/01/20/mesh-loading-with-assimp/
std::vector<glm::vec3> extractVertices(const aiMesh* mesh) {
    std::vector<glm::vec3> vertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D& vertex = mesh->mVertices[i];
        vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
    }
    return vertices;
}

std::vector<glm::vec2> extractTextureCoordinates(const aiMesh* mesh) {
    std::vector<glm::vec2> texCoords;
    if (mesh->HasTextureCoords(0)) {
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D& texCoord = mesh->mTextureCoords[0][i];
            texCoords.push_back(glm::vec2(texCoord.x, texCoord.y));
        }
    }
    else {
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            texCoords.push_back(glm::vec2(0.0, 0.0));
        }
    }
    return texCoords;
}

std::vector<glm::vec3> extractNormals(const aiMesh* mesh) {
    std::vector<glm::vec3> normals;
    if (mesh->HasNormals()) {
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D& normal = mesh->mNormals[i];
            normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
        }
    }
    return normals;
}

std::vector<unsigned int> extractIndices(const aiMesh* mesh) {
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }
    return indices;
}

//create a struct for boids
struct Boids {
    SceneNode* node;
};
Boids fighterNr[amountOfBoids];

Boids bumbleNr[amountOfBoids];

Boids hawkNr[amountOfBoids];

struct Ammunition {
    SceneNode* node;
};
Ammunition bulletNr[amountOfBoids];

void movement(SceneNode* node, double timeDelta, int i) {
    float fighterSpeed = 40;    
    
    //This is similar for all boids
    glm::vec3 boid;
    glm::vec3 cohesion = glm::vec3(0, 0, 0);
    glm::vec3 alignment = glm::vec3(0, 0, 0);
    glm::vec3 seperation = glm::vec3(0, 0, 0);
    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i && fighterNr[n].node->deactivated == false) {
            cohesion += fighterNr[n].node->position; //add all the positions of the active boids, apart from your own
            alignment += fighterNr[n].node->forward; //add the forward direction of all the active boids, apart from your own
            glm::vec3 interboidDistanceVector = node->position - fighterNr[n].node->position; //find the vector from current boid to other boids in flock
            float interboidDistanceLength = glm::length(interboidDistanceVector);             //find the distance from current boid to other boids in flock      
            interboidDistanceVector = normalize(interboidDistanceVector);
            interboidDistanceVector.operator*= (17 / interboidDistanceLength);      //give the vector away from the other boids different weight depending on how far away the other boids are
            seperation += interboidDistanceVector; //add them all together
        }
    }
    
    // count the amount of deactivted boids
    int m = 0;
    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i && fighterNr[n].node->deactivated == true) {
            m++;
        }
    }
    //used to balance seperation and cohesion
    float seperationWeight = glm::length(seperation / glm::max((float)(amountOfBoids - 1 - m), 1.0f));
    float prefixValue = glm::clamp(1.0f - seperationWeight, -1.0f, 1.0f);

    cohesion /= glm::max((amountOfBoids - 1 - m), 1);
    cohesion -= node->position; //find vector towards center of flock
    alignment = normalize(alignment);
    seperation = normalize(seperation);
    cohesion = normalize(cohesion);
                                                                                       
    boid = normalize(alignment + (1.0f - prefixValue) * seperation + (1.0f + prefixValue) * cohesion); //seperation and cohesion can have a fector of 0-2 in front of it, it is to balance it so that the distance between the boids is 17 units and they are all around the cohesion point
    
    node->guide = normalize(boid); //the boid factor of direction of movement
    
    if (m == (amountOfBoids - 1)) { //if there are no other boids, head in the same direction
        node->guide = node->forward;
    }

    //This part is to shove the boids away from the boundary, the closer they are to the boundary the greater the force. It is hoever still posible to hit the boudary in some cases
    float importanceFactor = 0;
    glm::vec3 diro = glm::vec3(0, 0, 0);
    
    if (node->position.x > 100 && node->position.y > -40) {
        
        float divideramount = 120 - node->position.x;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        
        importanceFactor = (1.0f / divideramount);
        diro = normalize(DOWN);
    }
    else if (node->position.x < -100 && node->position.y < 40) {
        
        float divideramount = -120 - node->position.x;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        
        importanceFactor = (1.0f / divideramount);
        diro = normalize(UP);
        
    }
    
    else if (node->position.y < -40 && node->position.x > -100) {
        
        float divideramount = -60 - node->position.y;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        
        importanceFactor = (1.0f / divideramount);
        diro = normalize(LEFT);
    }
    else if (node->position.y > 40 && node->position.x < 100) {
        
        float divideramount = 60 - node->position.y;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        
        importanceFactor = (1.0f / divideramount);
        diro = normalize(RIGHT);
    }

    //Logic to respawn deactivated boids
    if (node->deactivated && !node->reset) {
        node->forward = glm::vec3(0, -1, 0);
        node->guide = node->forward;
        importanceFactor = 0;
        fighterSpeed = 50;
    }

    if (node->deactivated && node->position.y < -200) {
        node->forward = glm::vec3(-1, 0, 0);
        node->guide = node->forward;
        node->reset = true;
        node->position = glm::vec3(160, (-30 + (i * (60.0 / (amountOfBoids - 1)))), -110); 
        fighterSpeed = 80;
    }

    if (node->deactivated && node->reset) {
        node->forward = glm::vec3(-1, 0, 0);
        node->guide = node->forward;
        fighterSpeed = 80;
    }

    if (node->reset && node->position.x < 90) {
        node->reset = false;
        node->deactivated = false;
    }

    //vector to avoid boids from other flocks, and deactivated boids from the same flock. Seperation in the above code handels avoidance within the flock.
    glm::vec3 avoid = glm::vec3(0, 0, 0);
    float amount = 0.0f;
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(bumbleNr[n].node->position - node->position) < 17) {
            amount++;
            
            avoid += -bumbleNr[n].node->forward;
        }
        if (glm::length(hawkNr[n].node->position - node->position) < 17) {
            amount++;
            
            avoid += -hawkNr[n].node->forward;
        }
        if (n != i && fighterNr[n].node->deactivated) {
            if (glm::length(fighterNr[n].node->position - node->position) < 17) {
                amount++;
                
                avoid += -fighterNr[n].node->forward;
            }
        }
    }
    avoid /= glm::max(amount, 1.0f);

    if (node->deactivated) {
        avoid = glm::vec3(0, 0, 0);
    }

    //The actual movement of the boid by putting all the factors together
    glm::vec3 direction = normalize(normalize(node->guide) + (float)15 * normalize(node->forward) + 3.0f*importanceFactor*diro + 2.0f*avoid);
    
    node->position.x += direction.x * fighterSpeed * timeDelta;
    node->position.y += direction.y * fighterSpeed * timeDelta;
    node->position.z += direction.z * fighterSpeed * timeDelta;

    
    node->forward = direction;

    
    //Handling for whan the boid goes out of bounds
    if (node->position.x > 120 || node->position.x < -120) {
        node->deactivated = true;
    }
    if (node->position.y > 60 || node->position.y < -60) {
        node->deactivated = true;
    }

    //time untill it can fire again
    if (node->locked) {
        node->time -= 30.0f * timeDelta;
        
        if (node->time <= 0) {
            node->time = 100;
            node->locked = false;
            node->bull = true;
        }
    }

    //Logic for fireing(angle between target and fighter boid has to be small enough, It has to be able to fire, it can not be deactivate, the target has to be within 80 units, the target can not be deactiveted)
    for (int n = 0; n < amountOfBoids; n++) {
       
        if (glm::length(normalize(bumbleNr[n].node->position - node->position) - direction) < 0.5 && node->bull == true && !node->deactivated && glm::length(bumbleNr[n].node->position - node->position) < 80 && !bumbleNr[n].node->deactivated) {  //0.5 is the strictness of direction
            
            bulletNr[i].node->forward = direction;
            bulletNr[i].node->bull = false;
            bulletNr[i].node->position = node->position;
            node->bull = false;
            node->locked = true;
        }
        else if (glm::length(normalize((hawkNr[n].node->position + glm::vec3(0, 2, 0)) - node->position) - direction) < 0.5 && node->bull == true && !node->deactivated && glm::length((hawkNr[n].node->position + glm::vec3(0, 2, 0)) - node->position) < 80 && !hawkNr[n].node->deactivated) {  //0.5 is the strictness of direction
           
            bulletNr[i].node->forward = direction;
            bulletNr[i].node->bull = false;
            bulletNr[i].node->position = node->position;
            node->bull = false;
            node->locked = true;
        }
    }

    //rotation
    glm::quat matree = glm::normalize(glm::quatLookAt(direction, glm::vec3(0.0, 0.0, 1.0)));
    
    node->rotation.x = glm::abs(glm::roll(matree)) - 1.57079632679489661923132169163975144;
    node->rotation.y = -glm::roll(matree);

    

    //makes it possible for boids in the same flock to crash into each other
    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i) {
            if (glm::length(fighterNr[n].node->position - node->position) < 5) {
                fighterNr[n].node->deactivated = true;
                node->deactivated = true;
            }
        }
    }
    
}

//bullet logic, that checks if it hits its target, and moves the bullet
void activeBullet(int i, double timeDelta) {
    int bulletspeed = 120;
    glm::vec3 direction = bulletNr[i].node->forward;
    bulletNr[i].node->position.x += direction.x * bulletspeed * timeDelta;
    bulletNr[i].node->position.y += direction.y * bulletspeed * timeDelta;
    bulletNr[i].node->position.z += direction.z * bulletspeed * timeDelta;
    glm::vec3 pos = bulletNr[i].node->position;
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(bumbleNr[n].node->position - pos) < 3) {
            bumbleNr[n].node->deactivated = true;
            
        }
        if (glm::length((hawkNr[n].node->position + glm::vec3(0, 2, 0)) - pos) < 3) {
            hawkNr[n].node->deactivated = true;
            
        }
    }
}

void movement2(SceneNode* node, double timeDelta, int i) {
    float fighterSpeed = 60;
    
        glm::vec3 boid;
        glm::vec3 cohesion = glm::vec3(0, 0, 0);
        glm::vec3 alignment = glm::vec3(0, 0, 0);
        glm::vec3 seperation = glm::vec3(0, 0, 0);
        for (int n = 0; n < amountOfBoids; n++) {
            if (n != i && bumbleNr[n].node->deactivated == false) {
                cohesion += bumbleNr[n].node->position;
                alignment += bumbleNr[n].node->forward;
                glm::vec3 interboidDistanceVector = node->position - bumbleNr[n].node->position;
                float interboidDistanceLength = glm::length(interboidDistanceVector); 
                interboidDistanceVector = normalize(interboidDistanceVector);
                interboidDistanceVector.operator*= (9 / interboidDistanceLength); 

                seperation += interboidDistanceVector;
            }
        }

        int m = 0;
        for (int n = 0; n < amountOfBoids; n++) {
            if (n != i && bumbleNr[n].node->deactivated == true) {
                m++;
            }

        }

        float seperationWeight = glm::length(seperation / glm::max((float)(amountOfBoids - 1 - m), 1.0f));
        float prefixValue = glm::clamp(1.0f - seperationWeight, -1.0f, 1.0f);

        cohesion /= glm::max((amountOfBoids - 1 - m), 1);
        cohesion -= node->position;

        alignment = normalize(alignment);
        seperation = normalize(seperation);
        cohesion = normalize(cohesion);
        

        boid = normalize(alignment + (1.0f - prefixValue) * seperation + (1.0f + prefixValue) * cohesion);



        node->guide = normalize(boid);

        if (m == (amountOfBoids - 1)) {
            node->guide = node->forward;
        }

        

        float importanceFactor = 0;
        glm::vec3 diro = glm::vec3(0, 0, 0);
        
        if (node->position.x > 100 && node->position.y > -40) {
            
            float divideramount = 120 - node->position.x;
            divideramount = glm::abs(divideramount);
            if (divideramount < 1) {
                divideramount = 1;
            }
            importanceFactor = (1.0f / divideramount);
            diro = normalize(DOWN);
        }
        else if (node->position.x < -100 && node->position.y < 40) {
            
            float divideramount = -120 - node->position.x;
            divideramount = glm::abs(divideramount);
            if (divideramount < 1) {
                divideramount = 1;
            }
            
            importanceFactor = (1.0f / divideramount);
            diro = normalize(UP);
        }
        else if (node->position.y < -40 && node->position.x > -100) {
            float divideramount = -60 - node->position.y;
            divideramount = glm::abs(divideramount);
            if (divideramount < 1) {
                divideramount = 1;
            }
            importanceFactor = (1.0f / divideramount);
            diro = normalize(LEFT);
        }
        else if (node->position.y > 40 && node->position.x < 100) {
            float divideramount = 60 - node->position.y;
            divideramount = glm::abs(divideramount);
            if (divideramount < 1) {
                divideramount = 1;
            }
            importanceFactor = (1.0f / divideramount);
            diro = normalize(RIGHT);
        }

    //logic for attack, It is loced into one direction until 100 frames has passed
    if (node->locked && node->deactivated == false) {
        node->guide = node->forward;
        fighterSpeed = 120;
        node->time -= 1;
        
        if (node->time <= 0) {
            node->time = 100;
            node->locked = false;
            node->bull = true;
        }
    }

    //checks if it collides with a fighter boid
    glm::vec3 pos = node->position;
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(fighterNr[n].node->position - pos) < 3) {
            fighterNr[n].node->deactivated = true;
            node->deactivated = true;
        }
    }

    if (node->deactivated && !node->reset) {
        node->forward = glm::vec3(0, -1, 0);
        node->guide = node->forward;
        importanceFactor = 0;
        fighterSpeed = 50;
    }

    if (node->deactivated && node->position.y < -200) {
        node->forward = glm::vec3(1, 0, 0);
        node->guide = node->forward;
        node->reset = true;
        node->locked = false;
        node->time = 100;
        node->position = glm::vec3(-160, (-6 - (i * (24.0 / (amountOfBoids - 1)))), -110); 
        fighterSpeed = 80;
    }

    if (node->deactivated && node->reset) {
        node->forward = glm::vec3(1, 0, 0);
        node->guide = node->forward;
        fighterSpeed = 80;
    }
    if (node->reset && node->position.x > -90) {
        node->reset = false;
        node->deactivated = false;
        node->bull = true;
    }

    

    glm::vec3 avoid = glm::vec3(0, 0, 0);
    float amount = 0.0f;
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(hawkNr[n].node->position - node->position) < 17) {
            amount++;
            avoid += -hawkNr[n].node->forward;
        }
        if (n != i && bumbleNr[n].node->deactivated) {
            if (glm::length(bumbleNr[n].node->position - node->position) < 17) {
                amount++;
                avoid += -bumbleNr[n].node->forward;
            }
        }
        if (fighterNr[n].node->deactivated) {
            if (glm::length(fighterNr[n].node->position - node->position) < 17) {
                amount++;
                avoid += -fighterNr[n].node->forward;
            }
        }
    }
    avoid /= glm::max(amount, 1.0f);

    if (node->deactivated || node->locked) {
        avoid = glm::vec3(0, 0, 0);
    }


    glm::vec3 direction = normalize(normalize(node->guide) + (float)15 * normalize(node->forward) + 3.0f * importanceFactor * diro + 2.0f*avoid);

    node->position.x += direction.x * fighterSpeed * timeDelta;
    node->position.y += direction.y * fighterSpeed * timeDelta;
    node->position.z += direction.z * fighterSpeed * timeDelta;


    node->forward = direction;

    if (node->position.x > 120 || node->position.x < -120) {
        node->deactivated = true;
    }
    if (node->position.y > 60 || node->position.y < -60) {
        node->deactivated = true;
    }


    //checks if target is within range (15 units), and orients itself that way
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(fighterNr[n].node->position - node->position) < 15 && node->bull == true) {
            node->bull = false;
            node->forward = normalize(fighterNr[n].node->position - node->position);
            node->locked = true;
        }
    }
    

    glm::quat matree = glm::normalize(glm::quatLookAt(direction, glm::vec3(0.0, 0.0, 1.0)));
    node->rotation.x = glm::abs(glm::roll(matree)) - 1.57079632679489661923132169163975144;
    node->rotation.y = -glm::roll(matree);

    

    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i) {
            if (glm::length(bumbleNr[n].node->position - node->position) < 2.8f) {
                bumbleNr[n].node->deactivated = true;
                node->deactivated = true;
            }
        }
    }
}



void movement3(SceneNode* node, double timeDelta, int i) {
    float fighterSpeed = 50;

    glm::vec3 boid;
    glm::vec3 cohesion = glm::vec3(0, 0, 0);
    glm::vec3 alignment = glm::vec3(0, 0, 0);
    glm::vec3 seperation = glm::vec3(0, 0, 0);
    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i && hawkNr[n].node->deactivated == false) {
            cohesion += hawkNr[n].node->position;
            alignment += hawkNr[n].node->forward;
            glm::vec3 interboidDistanceVector = node->position - hawkNr[n].node->position;
            float interboidDistanceLength = glm::length(interboidDistanceVector); 
            interboidDistanceVector = normalize(interboidDistanceVector);
            interboidDistanceVector.operator*= (20 / interboidDistanceLength);  

            seperation += interboidDistanceVector;
        }
    }

    int m = 0;
    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i && hawkNr[n].node->deactivated == true) {
            m++;
        }

    }

    float seperationWeight = glm::length(seperation / glm::max((float)(amountOfBoids - 1 - m), 1.0f));
    float prefixValue = glm::clamp(1.0f - seperationWeight, -1.0f, 1.0f);

    cohesion /= glm::max((amountOfBoids - 1 - m), 1);
    cohesion -= node->position;

    alignment = normalize(alignment);
    seperation = normalize(seperation);
    cohesion = normalize(cohesion);
    boid = normalize(alignment + (1.0f - prefixValue) * seperation + (1.0f + prefixValue) * cohesion);




    node->guide = normalize(boid);
    if (m == (amountOfBoids - 1)) {
        node->guide = node->forward;
    }

    

    float importanceFactor = 0;
    glm::vec3 diro = glm::vec3(0, 0, 0);
    if (node->position.x > 100 && node->position.y > -40) {
        float divideramount = 120 - node->position.x;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        importanceFactor = (1.0f / divideramount);
        diro = normalize(DOWN);
    }
    else if (node->position.x < -100 && node->position.y < 40) {
        
        float divideramount = -120 - node->position.x;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
       
        importanceFactor = (1.0f / divideramount);
        diro = normalize(UP);
    }
    else if (node->position.y < -40 && node->position.x > -100) {
        float divideramount = -60 - node->position.y;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        importanceFactor = (1.0f / divideramount);
        diro = normalize(LEFT);
    }
    else if (node->position.y > 40 && node->position.x < 100) {
        float divideramount = 60 - node->position.y;
        divideramount = glm::abs(divideramount);
        if (divideramount < 1) {
            divideramount = 1;
        }
        importanceFactor = (1.0f / divideramount);
        diro = normalize(RIGHT);
    }

    //attack logic, similar to the previous boid
    if (node->locked && node->deactivated == false) {
        node->guide = node->forward;
        fighterSpeed = 120;
        node->time -= 1;
        
        if (node->time <= 0) {
            node->time = 100;
            node->locked = false;
            node->bull = true;
        }
    }

    //Checks if it hits a fighter boid, the "glm::vec3(0.0f, -2.0f, 0.0f)" is added because if it only collides with that part(its feet), then it does not damage itself
    glm::vec3 pos = node->position + glm::vec3(0.0f, -2.0f, 0.0f);
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(fighterNr[n].node->position - pos) < 3) {
            fighterNr[n].node->deactivated = true;
        }
    }

    

    if (node->deactivated && !node->reset) {
        node->forward = glm::vec3(0, -1, 0);
        node->guide = node->forward;
        importanceFactor = 0;
        fighterSpeed = 50;
    }

    if (node->deactivated && node->position.y < -200) {
        node->forward = glm::vec3(1, 0, 0);
        node->guide = node->forward;
        node->reset = true;
        node->locked = false;
        node->time = 100;
        node->position = glm::vec3(-160, (0 + (i * (30.0 / (amountOfBoids - 1)))), -110);
        fighterSpeed = 80;
    }

    if (node->deactivated && node->reset) {
        node->forward = glm::vec3(1, 0, 0);
        node->guide = node->forward;
        fighterSpeed = 80;
    }

    if (node->reset && node->position.x > -90) {
        node->reset = false;
        node->deactivated = false;
        node->bull = true;
    }

    glm::vec3 avoid = glm::vec3(0, 0, 0);
    float amount = 0.0f;
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(bumbleNr[n].node->position - node->position) < 20) {
            amount++;
            avoid += -bumbleNr[n].node->forward;
        }
        if (n != i && hawkNr[n].node->deactivated) {
            if (glm::length(hawkNr[n].node->position - node->position) < 20) {
                amount++;
                avoid += -hawkNr[n].node->forward;
            }
        }
        if (fighterNr[n].node->deactivated) {
            if (glm::length(fighterNr[n].node->position - node->position) < 20) {
                amount++;
                avoid += -fighterNr[n].node->forward;
            }
        }
    }
    avoid /= glm::max(amount, 1.0f);

    if (node->deactivated) {
        avoid = glm::vec3(0, 0, 0);
    }

   
    glm::vec3 direction = normalize(normalize(node->guide) + (float)15 * normalize(node->forward) + 3.0f * importanceFactor * diro + 2.0f * avoid);


    node->position.x += direction.x * fighterSpeed * timeDelta;
    node->position.y += direction.y * fighterSpeed * timeDelta;
    node->position.z += direction.z * fighterSpeed * timeDelta;


    node->forward = direction;

    if (node->position.x > 120 || node->position.x < -120) {
        node->deactivated = true;
    }
    if (node->position.y > 60 || node->position.y < -60) {
        node->deactivated = true;
    }


    //Atttacks if it is above the fighter boid and behind it (angle between forward vector and vetor from node to fighter boid is acute(90 degrees<))
    for (int n = 0; n < amountOfBoids; n++) {
        if (glm::length(fighterNr[n].node->position - node->position) < 20 && node->bull == true && glm::dot(node->forward, fighterNr[n].node->position - node->position) > 0 && fighterNr[n].node->position.y < node->position.y) {
            node->bull = false;
            node->forward = normalize(fighterNr[n].node->position - (node->position + glm::vec3(0.0f, -2.0f, 0.0f)));
            node->locked = true;
        }
    }

    glm::quat matree = glm::normalize(glm::quatLookAt(direction, glm::vec3(0.0, 0.0, 1.0)));
    node->rotation.y = -glm::roll(matree);

    

    for (int n = 0; n < amountOfBoids; n++) {
        if (n != i) {
            if (glm::length(hawkNr[n].node->position - node->position) < 5.2f) { 
                hawkNr[n].node->deactivated = true;
                node->deactivated = true;
            }
        }
    }

}

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);


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

    

}


PNGImage diffuse = loadPNGFile("../res/textures/pastelFighter.png");
PNGImage spacePNG = loadPNGFile("../res/textures/Space.png");
PNGImage bumblePNG = loadPNGFile("../res/textures/circles.png");
PNGImage hawkPNG = loadPNGFile("../res/textures/animalPalette.png");

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

    

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    
    unsigned int boxVAO  = generateBuffer(box);
   
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    

    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    
    Assimp::Importer importer;
    Assimp::Importer importer2;
    Assimp::Importer importer3;
    Assimp::Importer importer4;
    Assimp::Importer importer5;

    bumblebee = importer.ReadFile("..\\res\\TFinalHoneybee.obj", aiProcessPreset_TargetRealtime_Fast); 

    fighter = importer2.ReadFile("..\\res\\TTTFinalMillitaryFighter.obj", aiProcessPreset_TargetRealtime_Fast); 

    bullet = importer3.ReadFile("..\\res\\TBullet.obj", aiProcessPreset_TargetRealtime_Fast);

    hawk = importer4.ReadFile("..\\res\\TFinalHawk.obj", aiProcessPreset_TargetRealtime_Fast);

    wall = importer5.ReadFile("..\\res\\TTSkyWall.obj", aiProcessPreset_TargetRealtime_Fast);

    mesh = fighter->mMeshes[0]; 

    mesh2 = bumblebee->mMeshes[0];

    mesh3 = bullet->mMeshes[0];

    mesh4 = hawk ->mMeshes[0];

    mesh5 = wall->mMeshes[0];

    Mesh blenderObjMesh;
    blenderObjMesh.vertices = extractVertices(mesh);
    blenderObjMesh.textureCoordinates = extractTextureCoordinates(mesh);
    blenderObjMesh.normals = extractNormals(mesh);
    blenderObjMesh.indices = extractIndices(mesh);

    Mesh bumbleMesh;
    bumbleMesh.vertices = extractVertices(mesh2);
    bumbleMesh.textureCoordinates = extractTextureCoordinates(mesh2);
    bumbleMesh.normals = extractNormals(mesh2); 
    bumbleMesh.indices = extractIndices(mesh2); 

    Mesh bulletMesh; 
    bulletMesh.vertices = extractVertices(mesh3); 
    bulletMesh.textureCoordinates = extractTextureCoordinates(mesh3); 
    bulletMesh.normals = extractNormals(mesh3); 
    bulletMesh.indices = extractIndices(mesh3); 

    Mesh hawkMesh; 
    hawkMesh.vertices = extractVertices(mesh4); 
    hawkMesh.textureCoordinates = extractTextureCoordinates(mesh4); 
    hawkMesh.normals = extractNormals(mesh4); 
    hawkMesh.indices = extractIndices(mesh4); 

    Mesh wallMesh; 
    wallMesh.vertices = extractVertices(mesh5); 
    wallMesh.textureCoordinates = extractTextureCoordinates(mesh5); 
    wallMesh.normals = extractNormals(mesh5); 
    wallMesh.indices = extractIndices(mesh5); 

    SceneNode* wallNode = createSceneNode();
    rootNode->children.push_back(wallNode);
    unsigned int wallNodeVAO = generateBuffer(wallMesh);
    wallNode->vertexArrayObjectID = wallNodeVAO;
    wallNode->VAOIndexCount = wallMesh.indices.size();
    wallNode->nodeType = NORM_MAP_GEO;
    int spaceID = createTextureID(spacePNG);
    wallNode->textureID = spaceID;
    wallNode->position = glm::vec3(0, 0, -250);

    int purpleID = createTextureID(diffuse);
    int bumbleID = createTextureID(bumblePNG);
    int hawkID = createTextureID(hawkPNG);

    for (int i = 0; i < amountOfBoids; i++) {
        
        bulletNr[i].node = createSceneNode();
        rootNode->children.push_back(bulletNr[i].node);
        unsigned int bulletNrVAO = generateBuffer(bulletMesh);
        bulletNr[i].node->vertexArrayObjectID = bulletNrVAO;
        bulletNr[i].node->VAOIndexCount = bulletMesh.indices.size();
        bulletNr[i].node->nodeType = NORM_MAP_GEO;

        
        bulletNr[i].node->textureID = hawkID;
        
    }

    

    printf("3");
    unsigned int fighterVAO = generateBuffer(blenderObjMesh);
    printf("4");
    blenderObjNode = createSceneNode();

    

    blenderObjNode->vertexArrayObjectID = fighterVAO;
    blenderObjNode->VAOIndexCount = blenderObjMesh.indices.size();

    blenderObjNode->position = glm::vec3(0,0,-110);

     for (int i = 0; i < amountOfBoids; i++) {
         
         fighterNr[i].node = createSceneNode();
         rootNode->children.push_back(fighterNr[i].node);
         unsigned int fighterNrVAO = generateBuffer(blenderObjMesh);
         fighterNr[i].node->vertexArrayObjectID = fighterNrVAO;
         fighterNr[i].node->VAOIndexCount = blenderObjMesh.indices.size();

         fighterNr[i].node->position = glm::vec3(i*-5, i*5, -110);    
         
         if (i == 0) {
             fighterNr[i].node->position = glm::vec3(0, 5, -110);
         }
         else if (i == 1) {
             fighterNr[i].node->position = glm::vec3(10, 0, -110);
         }
         else if (i == 2) {
             fighterNr[i].node->position = glm::vec3(0, -5, -110);
         }
         else if (i == 3) {
             fighterNr[i].node->position = glm::vec3(-10, 0, -110);
         }
         fighterNr[i].node->nodeType = NORM_MAP_GEO;

         fighterNr[i].node->textureID = purpleID;

     }

     for (int i = 0; i < amountOfBoids; i++) {
         bumbleNr[i].node = createSceneNode();
         rootNode->children.push_back(bumbleNr[i].node);
         unsigned int bumbleNrVAO = generateBuffer(bumbleMesh);
         bumbleNr[i].node->vertexArrayObjectID = bumbleNrVAO;
         bumbleNr[i].node->VAOIndexCount = bumbleMesh.indices.size();
         bumbleNr[i].node->forward = glm::vec3(1, 0, 0);
         bumbleNr[i].node->position = glm::vec3(i * -20, i * -5, -110); 
         bumbleNr[i].node->nodeType = NORM_MAP_GEO;

         bumbleNr[i].node->textureID = bumbleID;

     }

     for (int i = 0; i < amountOfBoids; i++) {
         hawkNr[i].node = createSceneNode();
         rootNode->children.push_back(hawkNr[i].node);
         unsigned int hawkNrVAO = generateBuffer(hawkMesh);
         hawkNr[i].node->vertexArrayObjectID = hawkNrVAO;
         hawkNr[i].node->VAOIndexCount = hawkMesh.indices.size();
         hawkNr[i].node->forward = glm::vec3(1, 0, 0);
         hawkNr[i].node->position = glm::vec3(i * 10, i * 10, -110); 
         hawkNr[i].node->nodeType = NORM_MAP_GEO;

         hawkNr[i].node->textureID = hawkID;
     }
     
    
    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

void updateFrame(GLFWwindow* window) {


    double timeDelta = getTimeDeltaSeconds();

    
    //Responsible for movement oof the boids, and bullets
    for (int i = 0; i < amountOfBoids; i++) {
        movement(fighterNr[i].node, timeDelta, i);
    }

    for (int i = 0; i < amountOfBoids; i++) {   
        movement2(bumbleNr[i].node, timeDelta, i); 
    } 

    for (int i = 0; i < amountOfBoids; i++) {   
        movement3(hawkNr[i].node, timeDelta, i); 
    } 

    for (int i = 0; i < amountOfBoids; i++) {
        if (bulletNr[i].node->bull == false) {
            
            activeBullet(i, timeDelta);
        }
    }

    //Checks if the fighter and hawk crashes into each other outside of attacks, as well as hawk and honeybee
    for (int i = 0; i < amountOfBoids; i++) {   
        for (int l = 0; l < amountOfBoids; l++) {
            if (glm::length(fighterNr[i].node->position - (hawkNr[l].node->position + glm::vec3(0.0f, 2.0f, 0.0f))) < 8.0f && !(fighterNr[i].node->deactivated && hawkNr[i].node->deactivated)) {
                fighterNr[i].node->deactivated = true;
                hawkNr[i].node->deactivated = true;
            }
        }
        for (int l = 0; l < amountOfBoids; l++) {
            if (glm::length(bumbleNr[i].node->position - (hawkNr[l].node->position + glm::vec3(0.0f, 2.0f, 0.0f))) < 6.0f && !(hawkNr[i].node->deactivated && bumbleNr[i].node->deactivated)) {
                hawkNr[i].node->deactivated = true;
                bumbleNr[i].node->deactivated = true;
            }
        }
    }

    

    

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    

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

        
    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    //Mathias
    glUniform3fv(10, 1, glm::value_ptr(cameraPosition));

    //Mathias2
    glm::mat4 orthographic = glm::ortho(0.0, 1366.0, 0.0, 768.0, -0.1, 350.0);
    glUniformMatrix4fv(13, 1, GL_FALSE, glm::value_ptr(orthographic));

    // Some math to make the camera move in a nice way
    float lookRotation = 0.0;
    glm::mat4 cameraTransform =
                    glm::rotate(0.0f, glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    
    

    VP = projection * cameraTransform;
    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10., -80. };

    

    glm::mat4 identityMatrix = glm::mat4(1.0f);
    updateNodeTransformations(rootNode, identityMatrix);
    std::cout << std::endl << std::endl;

}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar) {
    //std::cout << " " << ((unsigned int)node);
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

    

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

    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(VP)); //View Projection
    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix)); //Model
    glUniformMatrix3fv(9, 1, GL_FALSE, glm::value_ptr(glm::mat3(transpose(inverse(node->currentTransformationMatrix))))); //transposeInverseModelMatrix

    


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
        case TWOD_GEOMETRY:
            if (node->vertexArrayObjectID != -1) {
                glUniform1i(14, 1);
                glBindVertexArray(node->vertexArrayObjectID);
                glBindTextureUnit(0, node->textureID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case NORM_MAP_GEO:
            if (node->vertexArrayObjectID != -1) {
                glUniform1i(14, 2);
                glBindVertexArray(node->vertexArrayObjectID);
                glBindTextureUnit(1, node->textureID);
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
