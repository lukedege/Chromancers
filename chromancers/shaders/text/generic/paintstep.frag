#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D image;

void main()
{             
    vec4 sampled = texture(image, TexCoords);
    vec3 color = sampled.rgb;
    float alpha = sampled.a;
    
    float th = 0.2f;
    if(color.r < th && color.g < th && color.b < th)
    {
        alpha = 0f;
    }
    else
    {
        //color *= 1.5;
    }
    FragColor = vec4(color, alpha);
}