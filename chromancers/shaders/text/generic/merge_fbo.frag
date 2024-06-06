#version 430 core

out vec4 FragColor;

uniform sampler2D fbo0_color;
uniform sampler2D fbo0_depth;

uniform sampler2D fbo1_color;
uniform sampler2D fbo1_depth;

in vec2 TexCoords;

// Shader capable to merge two color images together by using their relative
// depth images to decide which color sample is in front of the other
void main()
{
    ivec2 texcoord = ivec2(floor(gl_FragCoord.xy));

	// Fetch depths
    float depth0 = texelFetch(fbo0_depth, texcoord, 0).r;
    float depth1 = texelFetch(fbo1_depth, texcoord, 0).r;

	// Fetch colors
    vec4 color0 = texelFetch(fbo0_color, texcoord, 0);
    vec4 color1 = texelFetch(fbo1_color, texcoord, 0);

    vec4 final_color;
    
	// Blend the colors by using the alpha of the least deep sample 
    if (depth0 <= depth1)
    {
		// color0 has precedence since it has smaller depth
        final_color = color0 * color0.a + color1 * (1 - color0.a);
    }
    else
    {
		// color1 has precedence since it has smaller depth
        final_color = color1 * color1.a + color0 * (1 - color1.a);
    }

    FragColor = final_color;
}