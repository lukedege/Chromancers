#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D image;
uniform float t0 = 0.1f;
uniform float t1 = 0.5f;

void main()
{             
    vec4 sampled = texture(image, TexCoords);
    vec3 s_color = sampled.rgb;
    float alpha = sampled.a;

	s_color = smoothstep(t0, t1, s_color);
	//alpha = smoothstep(0.1f, .3f, s_color_mag);

    FragColor = vec4(s_color, alpha);
}