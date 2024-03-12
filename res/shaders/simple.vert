#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;
in layout(location = 3) vec3 tangents;
in layout(location = 4) vec3 bitangents;

uniform layout(location = 3) mat4 MVP;

//Most of these variables have self explanatory names and have thus not been commented

//Mathias
uniform layout(location = 4) mat4 VP;
uniform layout(location = 5) mat4 M;
uniform layout(location = 6) vec4 light1Pos;
uniform layout(location = 7) vec4 light2Pos;
uniform layout(location = 8) vec4 light3Pos;
uniform layout(location = 9) mat3 normalM; //transposeInverseModelMatrix

//Mathias
uniform layout(location = 10) vec3 cameraPos;

//Mathias
uniform layout(location = 11) vec3 ballPos;

//Mathias2
uniform layout(location = 13 ) mat4 ortho;
uniform layout(location = 14) int is_2d;

//Mathias
out layout(location = 2) vec3 normalM_out;
out layout(location = 3) vec3 vertex_out;
out layout(location = 4) vec4 light1Pos_out;
out layout(location = 5) vec4 light2Pos_out;
out layout(location = 6) vec4 light3Pos_out;

//Mathias
out layout(location = 7) vec3 cameraPos_out;

//Mathias
out layout(location = 8) vec3 ballPos_out;

//Mathias2
out layout(location = 10) flat int is_2d_out;

//Mathias2
out layout(location = 11) mat3 TBN_out;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;

void main()
{
    normal_out = normal_in;
    textureCoordinates_out = textureCoordinates_in;
    //gl_Position = MVP * vec4(position, 1.0f); //original

    //Mathais
    //gl_Position = VP * M * vec4(position, 1.0f);

    //Mathias
    normalM_out = normalize(normalM * normal_in);
    vertex_out = vec3(M * vec4(position, 1.0f));
    light1Pos_out = light1Pos;
    light2Pos_out = light2Pos;
    light3Pos_out = light3Pos;

    //Mathias
    cameraPos_out = cameraPos;

    //Mathias
    ballPos_out = ballPos;

    if (is_2d == 1) {
        gl_Position = ortho * M * vec4(position , 1.0f); 
    } else {
        gl_Position = VP * M * vec4(position, 1.0f); //gl_Position = MVP * vec4(position, 1.0f);
    }

    //Mathias2
    is_2d_out = is_2d; //Has to do with textures

    //Mathias2
    TBN_out = mat3(normalize(normalM*tangents), normalize(normalM*bitangents), normalM_out);
}
