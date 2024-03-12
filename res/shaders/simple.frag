#version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

//Not all the unnecessary code has been removed as they were part of previous parts of the assignment, 
//and are important to show that the work as been done

//A number has been set behind the variables pertaining to lights 2 and 3, if there is no number it belongs to light 1 or is used by all lights.

//Mathias
in layout(location = 4) vec4 light1_pos;
in layout(location = 5) vec4 light2_pos;
in layout(location = 6) vec4 light3_pos;
in layout(location = 2) vec3 normalM;
in layout(location = 3) vec3 vertex_pos;

//Mathias
in layout(location = 7) vec3 camera_pos;

//Mathias
in layout(location = 8) vec3 ball_pos;
float ballRadius = 3.0f;


//Mathias2
in layout(location = 10) flat int is_2d;

//Mathias2
in layout(location = 11) mat3 TBN;

out vec4 color;

//Mathias
vec3 reject(vec3 from, vec3 onto) {
    return from - onto*dot(from, onto)/dot(onto, onto);
}

//Mathias
struct MyStruct {
    vec3 lightColor;
    vec3 lightPos;
};

//Mathias
uniform MyStruct anArrayOfStructs[3];

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

//Mathias
//Unversal variables (Variables used by all lights ++)
vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);
float shinynessFactor = 32.0;
vec3 from = ball_pos - vertex_pos;
vec3 surface_eyeVector = normalize(camera_pos - vertex_pos);


//Mathias
//Calculating Ls

float distance = length(vec3(anArrayOfStructs[0].lightPos) - vertex_pos);
float L = 1/(0.01 + distance * 0.001 + distance * distance * 0.006);

float distance2 = length(vec3(anArrayOfStructs[1].lightPos) - vertex_pos);
float L2 = 1/(0.01 + distance2 * 0.001 + distance2 * distance2 * 0.006);

float distance3 = length(vec3(anArrayOfStructs[2].lightPos) - vertex_pos);
float L3 = 1/(0.01 + distance3 * 0.001 + distance3 * distance3 * 0.006);


//Mathias2
layout(binding = 0) uniform sampler2D UI;

//Mathias2
layout(binding = 1) uniform sampler2D diffuseTexture;
layout(binding = 2) uniform sampler2D normalMapedGeometry;

//Mathias bonus task
layout(binding = 3) uniform sampler2D roughness; //bonus task

void main()
{
    vec3 diffuseMaterial = vec3(1.0);
    vec3 newNormal;

    //makes sure the textures are applied properly

    if (is_2d == 2) {
       newNormal = TBN * ((texture(normalMapedGeometry, textureCoordinates).xyz * 2)-1);
    }
    else {
       newNormal = normalM;
    }

    //makes sure the lighting becomes correct
    //Mathias2
    if (is_2d == 2) {
       diffuseMaterial = texture(diffuseTexture, textureCoordinates).rgb;
       shinynessFactor = 5/(pow(texture(roughness, textureCoordinates).a, 2)); //bonus task
    }


    //Mathias
    //Calculating diffuse lighting

    vec3 diffuseComponent = normalize(vec3(anArrayOfStructs[0].lightPos) - vertex_pos);
    float diffuseIntensity = L * max(dot(diffuseComponent, normalize(newNormal)), 0.0);
    vec4 diffuse = vec4(anArrayOfStructs[0].lightColor * diffuseMaterial, 1.0) * diffuseIntensity;

    vec3 diffuseComponent2 = normalize(vec3(anArrayOfStructs[1].lightPos) - vertex_pos);
    float diffuseIntensity2 = L2 * max(dot(diffuseComponent2, normalize(newNormal)), 0.0);
    vec4 diffuse2 = vec4(anArrayOfStructs[1].lightColor * diffuseMaterial, 1.0) * diffuseIntensity2;

    vec3 diffuseComponent3 = normalize(vec3(anArrayOfStructs[2].lightPos) - vertex_pos);
    float diffuseIntensity3 = L3 * max(dot(diffuseComponent3, normalize(newNormal)), 0.0);
    vec4 diffuse3 = vec4(anArrayOfStructs[2].lightColor * diffuseMaterial, 1.0) * diffuseIntensity3;


    //Mathias
    //Calculating specular lighting

    vec3 specularReflect = reflect(-diffuseComponent, normal);
    float specularIntensity = L * max(pow(dot(surface_eyeVector, specularReflect), shinynessFactor), 0.0);
    vec4 specular = specularIntensity * vec4(anArrayOfStructs[0].lightColor, 1.0);//vec4(0.7, 0.7, 0.7, 1.0);

    vec3 specularReflect2 = reflect(-diffuseComponent2, normal);
    float specularIntensity2 = L2 * max(pow(dot(surface_eyeVector, specularReflect2), shinynessFactor), 0.0);
    vec4 specular2 = specularIntensity2 * vec4(anArrayOfStructs[1].lightColor, 1.0);//vec4(0.7, 0.7, 0.7, 1.0);

    vec3 specularReflect3 = reflect(-diffuseComponent3, normal);
    float specularIntensity3 = L3 * max(pow(dot(surface_eyeVector, specularReflect3), shinynessFactor), 0.0);
    vec4 specular3 = specularIntensity3 * vec4(anArrayOfStructs[2].lightColor, 1.0);//vec4(0.7, 0.7, 0.7, 1.0);


    //Mathias
    //light1
    vec3 onto = vec3(anArrayOfStructs[0].lightPos) - vertex_pos;

    //Mathias
    //light2
    vec3 onto2 = vec3(anArrayOfStructs[1].lightPos) - vertex_pos;

    //Mathias
    //light3
    vec3 onto3 = vec3(anArrayOfStructs[2].lightPos) - vertex_pos;

    //color = vec4(0.5 * normal + 0.5, 1.0); //Original

    //Mathias
    //Here the shadow and soft shadows are calculated

    if (!(length(onto) < length(from)) && !(dot(from, onto) < 0)) {
        if(length(reject(from, onto)) < ballRadius + 2.0f) {
            diffuse *= vec4(length(reject(from, onto))/5.0f, length(reject(from, onto))/5.0f, length(reject(from, onto))/5.0f, 1.0);
            specular *= vec4(length(reject(from, onto))/5.0f, length(reject(from, onto))/5.0f, length(reject(from, onto))/5.0f, 1.0);
            if(length(reject(from, onto)) < ballRadius) {
                diffuse = vec4(0.0, 0.0, 0.0, 1.0);
                specular = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
    }

    if (!(length(onto2) < length(from)) && !(dot(from, onto2) < 0)) {
        if(length(reject(from, onto2)) < ballRadius + 2.0f) {
            diffuse2 *= vec4(0.0, length(reject(from, onto2))/5.0f, 0.0, 1.0);
            specular2 *= vec4(0.0, length(reject(from, onto2))/5.0f, 0.0, 1.0);
            if(length(reject(from, onto2)) < ballRadius) {
                diffuse2 = vec4(0.0, 0.0, 0.0, 1.0);
                specular2 = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
    }

    if (!(length(onto3) < length(from)) && !(dot(from, onto3) < 0)) {
        if(length(reject(from, onto3)) < ballRadius + 2.0f) {
            diffuse3 *= vec4(0.0, 0.0, length(reject(from, onto3))/5.0f, 1.0);
            specular3 *= vec4(0.0, 0.0, length(reject(from, onto3))/5.0f, 1.0);
            if(length(reject(from, onto3)) < ballRadius) {
                diffuse3 = vec4(0.0, 0.0, 0.0, 1.0);
                specular3 = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
    }

    //Mathias
    //color = ambient + (diffuse + diffuse2 + diffuse3) + specular + specular2 + specular3 + dither(textureCoordinates);

    //Mathias2
    color = ambient + diffuse + specular + dither(textureCoordinates); //only one light

    //Mathias2 test
    //color = vec4(newNormal, 1.0);

    //Mathias2
    if (is_2d == 1) {
       vec4 onScreenText = texture(UI, textureCoordinates); //displaying text
       color = onScreenText;
    }

}