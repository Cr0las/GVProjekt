#version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

in layout(location = 2) vec3 normalM;
in layout(location = 3) vec3 vertex_pos;

in layout(location = 7) vec3 camera_pos;

in layout(location = 10) flat int is_2d;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

layout(binding = 1) uniform sampler2D diffuseTexture;

void main()
{

    color = vec4(0.5 * normal + 0.5, 1.0);
    if (is_2d == 2) {
       color = vec4(texture(diffuseTexture, textureCoordinates).rgb, 1.0);
    }
}