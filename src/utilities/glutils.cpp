#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>

template <class T>
unsigned int generateAttribute(int id, int elementsPerEntry, std::vector<T> data, bool normalize) {
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
    glEnableVertexAttribArray(id);
    return bufferID;
}

unsigned int generateBuffer(Mesh &mesh) {
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    generateAttribute(0, 3, mesh.vertices, false);
    if (mesh.normals.size() > 0) {
        generateAttribute(1, 3, mesh.normals, true);
    }
    if (mesh.textureCoordinates.size() > 0) {
        generateAttribute(2, 2, mesh.textureCoordinates, false);
    }
    //Mathias2 based on guide
    glm::vec3 deltaPos1;
    glm::vec3 deltaPos2;

    glm::vec2 deltaUV1;
    glm::vec2 deltaUV2;

    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

    tangents.resize(mesh.vertices.size());
    bitangents.resize(mesh.vertices.size());
    printf("5");
    printf("%i %i", mesh.vertices.size(), mesh.textureCoordinates.size());
    for (int i = 0; i < mesh.vertices.size()-2; i += 3) {

        // Shortcuts for vertices
        glm::vec3& v0 = mesh.vertices[i + 0];
        glm::vec3& v1 = mesh.vertices[i + 1];
        glm::vec3& v2 = mesh.vertices[i + 2];

        // Shortcuts for UVs
        glm::vec2& uv0 = mesh.textureCoordinates[i + 0];
        glm::vec2& uv1 = mesh.textureCoordinates[i + 1];
        glm::vec2& uv2 = mesh.textureCoordinates[i + 2];

        // Edges of the triangle : position delta
        deltaPos1 = v1 - v0;
        deltaPos2 = v2 - v0;

        // UV delta
        deltaUV1 = uv1 - uv0;
        deltaUV2 = uv2 - uv0;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        tangents.at(i) = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        tangents.at(i+1) = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        tangents.at(i+2) = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        bitangents.at(i) = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        bitangents.at(i+1) = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        bitangents.at(i+2) = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
    }
    printf("6");
    generateAttribute(3, 3, tangents, true);
    generateAttribute(4, 3, bitangents, true);

    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    return vaoID;
}
