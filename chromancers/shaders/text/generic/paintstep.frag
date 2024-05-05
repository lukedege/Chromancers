#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D image;

uniform vec4 color;

void main()
{             
    vec4 sampled = texture(image, TexCoords);
    vec3 s_color = sampled.rgb;
    float alpha = sampled.a;

	vec3 p_color = color.rgb;

	p_color = smoothstep(0.1f, 0.3f, s_color);

	float s_color_mag = length(s_color);
	float p_color_mag = length(p_color);

	s_color = smoothstep(0.1f, .3f, s_color);
	alpha = smoothstep(0.1f, .3f, s_color_mag);

	// thresholds
    float th0 = 0.5f, th1 = 0.4f, th2 = 0.8f;
    if(s_color_mag < th0)
    {
        alpha = 0;
    }

    FragColor = vec4(s_color, alpha);
}