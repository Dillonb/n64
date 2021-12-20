#version 450
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D uImage;

layout(push_constant) uniform Screen
{
    vec2 size;
    vec2 offset;
} uScreen;

layout(constant_id = 0) const float Scale = 1.0;

void main()
{
    vec2 uv = (vUV - uScreen.offset) / uScreen.size;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        FragColor = vec4(0, 0, 0, 1);
    } else {
        FragColor = Scale * textureLod(uImage, uv, 0.0);
    }
}
