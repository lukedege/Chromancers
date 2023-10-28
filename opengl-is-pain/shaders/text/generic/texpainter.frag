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

// The size of the paint map
uniform int paintmap_size;

// The paint map
uniform layout(binding = 3, rgba32f) image2D paint_map;

// The color of the paintball
uniform vec4 paintBallColor;

// The direction of the paintball in world coordinates
uniform vec3 paintBallDirection;

void main()
{
    // Compute the integer coordinates from the interpolated normalized uvs, aka from [0, 1] to [0, paintmap_size] 
    ivec2 uv_pixels = ivec2(fs_in.interp_UV * paintmap_size);
    
    // Compute perspective divide and normalize it into a [0, 1] range
    vec3 projCoords = fs_in.pwFragPos.xyz / fs_in.pwFragPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Compute if fragment is inside the paint splat mask
    float paintMaskAlpha = texture2D(splat_mask, projCoords.xy).r;
    float paintAlpha = paintBallColor.a * paintMaskAlpha;

    // Computes incidence angle between paint ball direction and face normal
    float incidence = dot(normalize(paintBallDirection), fs_in.wNormal);
    
    // If dot product < 0 then the face got hit by the paint
    if (incidence < 0 && paintMaskAlpha > 0)
    {
        // Store new paint color value
        imageStore(paint_map, uv_pixels, vec4(paintBallColor.rgb, paintAlpha));
    }
}