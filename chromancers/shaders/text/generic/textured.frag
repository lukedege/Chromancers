#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D tex;

// Simple fragment shader, sampling a texture for the fragment color given coordinates from the vertex shader
void main()
{ 
    FragColor = texture(tex, TexCoords);
}