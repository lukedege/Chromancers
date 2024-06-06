#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D image;
uniform float t0_color = 0.1f;
uniform float t1_color = 0.5f;
uniform float t0_alpha = 0.7f;
uniform float t1_alpha = 0.9f;

// Shader for color level adjustment in screen-space through the usage of the smoothstep function
// Effect is customizable through the previous uniform parameters
void main()
{             
    vec4 sampled = texture(image, TexCoords);
    vec3 s_color = sampled.rgb;
    float alpha = sampled.a;

	s_color = smoothstep(t0_color, t1_color, s_color);
	alpha = smoothstep(t0_alpha, t1_alpha, alpha);

    FragColor = vec4(s_color, alpha);
}