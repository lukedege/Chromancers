#version 430 core

// output variable for the fragment shader
out vec4 colorFrag;

in VS_OUT 
{
	// world space data
	vec3 wFragPos;

	// view space data
	vec3 vwFragPos;

	// tangent space data
	vec3 twPointLightDir[MAX_POINT_LIGHTS];
	vec3 twDirLightDir  [MAX_DIR_LIGHTS];
	vec3 twFragPos; // Local -> World (w) -> Tangent (t) position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twCameraPos; 
	vec3 twNormal;

	// light space data
	vec4 lwFragPos;

	// the output variable for UV coordinates
	vec2 interp_UV;
} fs_in;

//Lights
uniform uint nPointLights;
uniform uint nDirLights;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];

// Textures
// texture samplers
uniform sampler2D diffuse_map     ; // TexUnit0 Main material color
uniform sampler2D normal_map      ; // TexUnit1 Normals for detail and light computation
uniform sampler2D displacement_map; // TexUnit2 Emulated vertex displacement (also known as height/depth map)
uniform usampler2D detail_map      ; // TexUnit3 Secondary material color

uniform sampler2D shadow_map;       // TexUnit4 Shadow map

uniform int sample_diffuse_map      = 0;
uniform int sample_normal_map       = 0;
uniform int sample_displacement_map = 0;
uniform int sample_detail_map       = 0;

uniform float uv_repeat = 1; // texture repetitions

// Material-light attributes
uniform vec4 ambient_color ;
uniform vec4 diffuse_color ;
uniform vec4 specular_color;

// Ambient, diffuse, specular coefficients
uniform float kA;
uniform float kD;
uniform float kS;

// shininess coefficients (passed from the application)
uniform float shininess;

// uniforms for GGX model
uniform float alpha; // rugosity - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence

// uniform for parallax map
uniform float parallax_heightscale;

// Current light position
vec3 curr_twLightDir;

vec2 CheapParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
	// Sample the heightmap
    float height = texture(displacement_map, texCoords).r; 
	// Calculate vector towards approximate height
	vec2 p = viewDir.xy * (height * parallax_heightscale);
	// Return displacement
    return texCoords - p; 
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * parallax_heightscale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(displacement_map, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(displacement_map, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(displacement_map, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec2 calculateTexCoords(vec2 texCoords, vec3 viewDir)
{
	//Repeated UV coords
	vec2 computedTexCoords = mod(texCoords * uv_repeat, 1.0);

	// branchless condition to avoid divergence
	computedTexCoords = ((1 - sample_displacement_map) * computedTexCoords) + // no displacement map
	                    ((sample_displacement_map)     * CheapParallaxMapping(computedTexCoords,  viewDir));  // use displacement map
	
	return computedTexCoords;
}

vec4 calculateSurfaceColor(vec2 texCoords)
{
	// branchless condition to avoid divergence
	vec4 surface_color = ((1 - sample_diffuse_map) * diffuse_color) + // use albedo (diffuse color)
	                     ((sample_diffuse_map)     * texture(diffuse_map, texCoords)); // use diffuse map

	return surface_color;
}

vec4 calculateDetailColor(vec2 texCoords)
{
	// branchless condition to avoid divergence
	vec4 detail_color = ((1 - sample_detail_map) * 0) + // don't consider detail
	                    ((sample_detail_map)     * texture(detail_map, texCoords)); // use detail map

	return detail_color;
}

vec3 calculateNormal(vec2 texCoords)
{
	// branchless condition to avoid divergence
	vec3 normal = ((1 - sample_normal_map) * normalize(fs_in.twNormal)) + // use vertex normal
	              ((sample_normal_map)     * normalize(texture(normal_map, texCoords).rgb * 2.0 - 1.0)); // use normal map

	return normal;
}

float calculateShadow(vec4 lwFragPos, vec3 lightDir, vec3 normal)
{
	// perform perspective divide
    vec3 projCoords = lwFragPos.xyz / lwFragPos.w;

	// normalize coordinates to be within [0,1] instead of NDC range [-1,1]
	projCoords = projCoords * 0.5 + 0.5; 

	// sample the depthmap obtained from the light POV
	float closestDepth = texture(shadow_map, projCoords.xy).r;

	// get fragment depth
	float currentDepth = projCoords.z;  

	// calculate bias to solve shadow acne 
	float shadow_factor = dot(normal, lightDir);
	float bias = mix(0.005, 0.0, shadow_factor);

	// if fragment has greater depth, then something occluded it from light POV, thus is in shadow
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;  

	// don't affect areas outside the shadow_map
	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}


float samplePaintAlpha(usampler2D paintMap, vec2 texCoords)
{
	float texelSize = 1.0 / textureSize(paintMap, 0).x;
	float texelDepth = 255;
	float paintAlpha = float(texture(paintMap, texCoords + texelSize).r) / texelDepth;
	return paintAlpha;
}

float samplePaintAlpha_pcf()
{
	//for (int x = -1; x <= 1; x++)
	//	for (int y = -1; y <= 1; y++)
	//		paint_alpha += float(texture(detail_map, fs_in.interp_UV + texelSize * vec2(x, y)).r)/255; 
	//paint_alpha = (9.0 - paint_alpha) / 9.0;
	return 0.f;
}

vec4 calculatePaintColor(usampler2D paintMap, vec2 texCoords, vec4 paintColor, vec4 surfaceColor)
{
	float paintAlpha = samplePaintAlpha(paintMap, texCoords); 
	vec4 finalColor = (1 - paintAlpha) * paintColor + paintAlpha * surfaceColor;
	return finalColor;
}

vec3 BlinnPhong()
{
	// ambient component 
	vec3 ambient_component = kA * ambient_color.rgb;

	// view direction
	vec3 V = normalize( fs_in.twCameraPos - fs_in.twFragPos );

    vec2 final_texCoords = calculateTexCoords(fs_in.interp_UV, V);

	// obtain normal 
	vec3 N = calculateNormal(final_texCoords);
	
	// normalization of the per-fragment light incidence direction
	vec3 L = normalize(curr_twLightDir);
	
	// Lambert coefficient
	float lambertian = max(dot(L,N), 0.0);
	
	vec3 final_color = ambient_component;

	// if the lambert coefficient is positive, then we calculate the specular component
	if(lambertian > 0.0)
	{
		// calculate surface color
		vec4 surface_color = calculateSurfaceColor(final_texCoords);

		// Temp
		if(sample_detail_map == 1)
		{
			vec4 paint_color = vec4(1,1,0,0.1);
			surface_color = calculatePaintColor(detail_map, fs_in.interp_UV, paint_color, surface_color);
		}

		// calculate diffuse component
		vec3 diffuse_component = kD * lambertian * surface_color.rgb;

		// in the Blinn-Phong model we do not use the reflection vector, but the half vector
		vec3 H = normalize(L + V);
		// we use H to calculate the specular component
		float specAngle = max(dot(H, N), 0.0);
		// shininess application to the specular component
		float spec = pow(specAngle, shininess);

		vec3 specular_component = kS * spec * specular_color.rgb;

		float shadow = calculateShadow(fs_in.lwFragPos, L, N);
		
		// We add diffusive and specular components to the final color
		// N.B. ): in this implementation, the sum of the components can be different than 1
		final_color += (1 - shadow) * (diffuse_component + specular_component);
	}
	return final_color;
}

vec4 calculatePointLights()
{
	vec4 color = vec4(0);
	for(int i = 0; i < nPointLights; i++)
	{
		curr_twLightDir = fs_in.twPointLightDir[i];//maybe you need to check for twfragpos
		float light_distance = length(curr_twLightDir);
		float attenuation = 0.001f + // to avoid division by zero
							pointLights[i].attenuation_constant +
							pointLights[i].attenuation_linear * light_distance +
							pointLights[i].attenuation_quadratic * light_distance * light_distance;

		color += vec4(BlinnPhong(), 1.0f) * pointLights[i].color * pointLights[i].intensity * (1.f/attenuation);
	}
	//color = normalize(color);
	return color;
}

vec4 calculateDirLights()
{
	vec4 color = vec4(0);
	
	for(int i = 0; i < nDirLights; i++)
	{
		curr_twLightDir = fs_in.twDirLightDir[i];
		color += vec4(BlinnPhong(), 1.0f) * directionalLights[i].color * directionalLights[i].intensity;
	}
	//color = normalize(color);
	return color;
}


void main()
{
	vec4 color = vec4(0);
	
	color += calculatePointLights();
	color += calculateDirLights();
	
	//if(sample_detail_map == 1)
	//{
	//	vec4 paint_color = vec4(0.1,0.1,0,0);
	//	vec4 surface_color = vec4(1,1,1,1);
	//	color = calculatePaintColor(detail_map, fs_in.interp_UV, paint_color, surface_color);
	//}

	colorFrag = color;
}