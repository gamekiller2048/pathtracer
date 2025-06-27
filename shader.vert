#version 430 core

layout(location = 0) in vec2 v_pos;
layout(location = 1) in vec2 v_texUV;

out vec2 texUV;

void main()
{
    gl_Position = vec4(v_pos, 0, 1);
    texUV = v_texUV;
}