#version 430 core

in VS_OUT 
{
    // the output variable for UV coordinates
	vec2 interp_UV;

    // vertex normal
    vec3 wNormal;

    // Fragment position in paint space
    vec4 pwFragPos;
} fs_in;

// The texture that represents the stain.
uniform sampler2D stainTex; // bound to unit0

// The size of the paint map.
uniform int paintmap_size;

// The previous state of the paint map.
uniform layout(binding = 3, rgba8ui) uimage2D previous_paint_map;

// The maximum unsigned byte (used for normalization)
const uint max_ubyte = 255;

// The color of the paintball
uniform vec4 paintBallColor;

// The direction of the paintball in world coordinates
uniform vec3 paintBallDirection;

void main()
{
    // Retrieves color on the previous version of the paint map.
    ivec2 uv_pixels = ivec2(fs_in.interp_UV * paintmap_size);
    uv_pixels.x = clamp(uv_pixels.x, 0, paintmap_size - 1);
    uv_pixels.y = clamp(uv_pixels.y, 0, paintmap_size - 1);
    //uint previous_paint_color = imageLoad(previous_paint_map, uv_pixels).r;
    
    // Retrieves paint color added to the fragment with the current splat.
    vec3 projCoords = fs_in.pwFragPos.xyz / fs_in.pwFragPos.w;
    projCoords = projCoords * 0.5 + 0.5;
    uint addedColor = uint(max_ubyte * gl_FragDepth); // the closer the surface -> the smaller the glfragdepth value -> the darker the color applied to mask
    
    // Computes incidence angle between paint ball direction and face normal.
    float paintLevel = 1.0 - texture2D(stainTex, projCoords.xy).r;
    float incidence = dot(normalize(paintBallDirection), fs_in.wNormal);
    
    // If dot product < 0 then the face got hit by the paint.
    if (incidence < 0 && paintLevel > 0)
    {
        // Computes new paint color value.
        uvec4 paint_color = uvec4(paintBallColor) * max_ubyte;
    
        imageStore(previous_paint_map, uv_pixels, paint_color);
    }
}