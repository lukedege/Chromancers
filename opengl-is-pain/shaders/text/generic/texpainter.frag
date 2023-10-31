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
uniform layout(binding = 1, rgba32f) image2D paint_map;

// The paint mask
uniform layout(binding = 2, r8) image2D paint_mask;

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
    float was_painted = imageLoad(paint_mask, uv_pixels).r;
    
    
    // Compute perspective divide and normalize it into a [0, 1] range
    vec3 projCoords = fs_in.pwFragPos.xyz / fs_in.pwFragPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Compute if fragment is inside the paint splat mask
    float splatMaskAlpha = texture(splat_mask, projCoords.xy).r;
    float paintAlpha = /*paintBallColor.a */ splatMaskAlpha;
    paintAlpha = clamp(paintAlpha, 0.f, 1.f);

    // TODO -----------------------------------------------------
    // - work towards mask transparency and correct splatting at angles TODO
    // TODO -----------------------------------------------------

    // Computes incidence angle between paint ball direction and face normal
    float incidence = dot(normalize(paintBallDirection), fs_in.wNormal);
    
    // If dot product < 0 then the face got hit by the paint
    if (incidence < 0 && splatMaskAlpha > 0)
    {
        //if(was_painted == 0)
        //    prev_color.rgb = paintBallColor.rgb;
        vec3 final_color = mix(prev_color.rgb, paintBallColor.rgb, paintAlpha);
        float final_alpha = clamp(paintAlpha + prev_color.a, 0.f, 1.f);

        // Store new paint color value
        imageStore(paint_map, uv_pixels, vec4(final_color, final_alpha));
        //vec4 final_color = mix(prev_color, paintBallColor, paintAlpha);

        //imageStore(paint_map, uv_pixels, final_color);
        imageStore(paint_mask, uv_pixels, vec4(1.f));
    }
}