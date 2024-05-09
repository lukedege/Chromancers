#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform float blur_strength = 1.f;

float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
// the equivalent blur kernel is a 10x10 mask

vec4 rgb_blur()
{
	vec2 tex_offset = blur_strength / textureSize(image, 0); // gets size of single texel
    vec3 result = texture(image, TexCoords).rgb * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }

	return vec4(result, 1.0);
}

vec4 rgba_blur()
{
	vec2 tex_offset = blur_strength / textureSize(image, 0); // gets size of single texel
    vec4 result = texture(image, TexCoords) * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)) * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)) * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)) * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)) * weight[i];
        }
    }

	return result;
}

void main()
{             
    FragColor = rgba_blur();
}