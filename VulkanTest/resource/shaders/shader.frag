#version 450

layout(location = 0) in vec3 v_Color;
layout(location = 1) in vec2 v_Coord;
layout(binding = 1) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(v_Color, 1.0) * texture(texSampler, v_Coord);
}