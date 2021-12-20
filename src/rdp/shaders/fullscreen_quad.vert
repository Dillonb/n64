#version 450
layout(location = 0) in vec2 Position;
layout(location = 0) out highp vec2 vUV;

void main()
{
    gl_Position = vec4(Position, 0.0, 1.0);
    vUV = 0.5 * Position + 0.5;
}