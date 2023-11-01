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

// The texture that represents the splat mask to apply
uniform sampler2D splat_mask; // bound to unit0

// The paint map
uniform layout(binding = 1, rgba8) image2D paint_map;

// The size of the paint map
uniform int paintmap_size;

// The color of the paintball
uniform vec4 paintBallColor;

// The direction of the paintball in world coordinates
uniform vec3 paintBallDirection;

void main()
{
    // Compute the integer coordinates from the interpolated normalized uvs, aka from [0, 1] to [0, paintmap_size] 
    // This is needed for imageStore as the coordinates required are integers
    ivec2 uv_pixels = ivec2(fs_in.interp_UV * paintmap_size);
    vec4 prev_color = imageLoad(paint_map, uv_pixels);

    // Compute perspective divide and normalize fragments projected coordinates into a [0, 1] range
    vec3 projCoords = fs_in.pwFragPos.xyz / fs_in.pwFragPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Given the fragment coordinates, retrieve the corresponding alpha from mask
    float splatMaskAlpha = texture(splat_mask, projCoords.xy).r;

    // Computes incidence angle between paint ball direction and face normal
    float incidence = dot(normalize(paintBallDirection), fs_in.wNormal);
    
    // If dot product < 0 then the face got hit by the paint
    if (incidence < 0 && splatMaskAlpha > 0)
    {
        // Store new paint color value
        vec4 final_color = mix(prev_color, paintBallColor, splatMaskAlpha);
        imageStore(paint_map, uv_pixels, final_color);
    }
}