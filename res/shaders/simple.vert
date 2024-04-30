#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 tangents;
in layout(location = 4) vec3 bitangents;

uniform layout(location = 3) mat4 MVP;


uniform layout(location = 4) mat4 VP;
uniform layout(location = 5) mat4 M;
uniform layout(location = 9) mat3 normalM; //transposeInverseModelMatrix


uniform layout(location = 10) vec3 cameraPos;


uniform layout(location = 13 ) mat4 ortho;
uniform layout(location = 14) int is_2d;

out layout(location = 3) vec3 vertex_out;

out layout(location = 7) vec3 cameraPos_out;

out layout(location = 10) flat int is_2d_out;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;

void main()
{
    normal_out = normal_in;
    textureCoordinates_out = textureCoordinates_in;

    
    gl_Position = VP * M * vec4(position, 1.0f);

    vertex_out = vec3(M * vec4(position, 1.0f));

    
    cameraPos_out = cameraPos;

    
    is_2d_out = is_2d;

}
