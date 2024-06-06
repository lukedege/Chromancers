#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

// We're using the depth image from the paintball rendering step to determine how 
// much we should blur a certain "fragment": the closer it is to camera, the more
// it should be blurred.
// With this, farther paintballs will not just disappear and closer paintballs
// will not appear as singular elements anymore, but parts of a stream

uniform sampler2D image;
uniform sampler2D depth_image;

uniform bool horizontal = false;
uniform float blur_strength = 1.f;
uniform bool ignore_alpha = false;

uniform float near_plane = 0.1f;
uniform float far_plane  = 100.f;
uniform float depth_offset = 1.f; 
// We use a depth offset to avoid blurring too much paintballs closer to the camera (such as the ones coming out of the gun)

// The equivalent blur kernel is a 9x9 gaussian mask
// We can take advantage of bilinear texture filtering to get information
// about 2 pixels instead of one, halving the texture fetches
const uint gauss_sampleCount = 3;
float weights[gauss_sampleCount] = float[](0.2270270270, 0.3162162162, 0.0702702703);
float offsets[gauss_sampleCount] = float[](0.0, 1.3846153846, 3.2307692308);
/*
const uint gauss_sampleCount = 9;
float weights[gauss_sampleCount] = float[]
(
6.745572658e-2,
1.302552143e-1,
1.131940344e-1,
8.791722474e-2,
6.103025286e-2,
3.786477257e-2,
2.099627278e-2,
1.040549874e-2,
4.608866282e-3
);

float offsets[gauss_sampleCount] = float[]
(
0.000000000	,
1.489397239	,
3.475276694	,
5.461195572	,
7.447176104	,
9.433240132	,
11.419408973,
13.405703291,
15.392142983
);*/


// Since the depth map is not linear, we need to linearize it
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

vec4 rgba_blur(vec2 tex_offset)
{
    vec4 result = texture(image, TexCoords) * weights[0]; // current fragment's contribution

    if(horizontal)
    {
        for(int i = 1; i < gauss_sampleCount; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * offsets[i], 0.0)) * weights[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * offsets[i], 0.0)) * weights[i];
        }
    }
    else
    {
        for(int i = 1; i < gauss_sampleCount; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * offsets[i])) * weights[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * offsets[i])) * weights[i];
        }
    }

	return result;
}


vec4 rgb_blur(vec2 tex_offset)
{
    vec4 result = rgba_blur(tex_offset);

	return vec4(result.rgb, 1.0);
}

void main()
{	
	float linear_depth = LinearizeDepth(texture(depth_image, TexCoords).r) + depth_offset; // linearize depth value to use depth as a way to tell distance
	float depthbased_blur_strength = blur_strength / linear_depth; // sample depthmap, the deeper the fragment, the less it is blurred
	vec2 tex_offset = depthbased_blur_strength / textureSize(image, 0); // gets size of single texel

	vec4 color = ignore_alpha ? rgb_blur(tex_offset) : rgba_blur(tex_offset);

    FragColor = color;
}