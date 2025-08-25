#version 450 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Color;

layout(location = 0) out vec4 vertexColor;

layout(std140, location = 0) uniform Transformation {
    mat4 Projection;
    mat4 View;
    mat4 Model;
} transformation;

void main() {
    gl_Position = transformation.Projection * transformation.View * transformation.Model * vec4(Position, 1.0);
    vertexColor = Color;
}
