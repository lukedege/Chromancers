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
uniform float far_plane  = 100f;
uniform float depth_offset = 1.f; 
// We use a depth offset to avoid blurring too much paintballs closer to the camera (such as the ones coming out of the gun)

// The equivalent blur kernel is a 10x10 gaussian mask
float weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);


// Since the depth map is not linear, we need to linearize it
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

vec4 rgb_blur(vec2 tex_offset)
{
    vec3 result = texture(image, TexCoords).rgb * weights[0]; // current fragment's contribution

    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weights[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weights[i];
        }
    }

	return vec4(result, 1.0);
}

vec4 rgba_blur(vec2 tex_offset)
{
    vec4 result = texture(image, TexCoords) * weights[0]; // current fragment's contribution

    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)) * weights[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)) * weights[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)) * weights[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)) * weights[i];
        }
    }

	return result;
}

void main()
{	
	float linear_depth = LinearizeDepth(texture(depth_image, TexCoords).r) + depth_offset;
	float depthbased_blur_strength = blur_strength / linear_depth; // sample depthmap, the deeper the fragment, the less it is blurred
	vec2 tex_offset = depthbased_blur_strength / textureSize(image, 0); // gets size of single texel

	vec4 color = ignore_alpha ? rgb_blur(tex_offset) : rgba_blur(tex_offset);

    FragColor = color;
}