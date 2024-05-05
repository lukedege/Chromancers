#version 430 core

// output variable for the fragment shader
out vec4 colorFrag;
out float gl_FragDepth;

in VS_OUT 
{
	// the output variable for UV coordinates
	vec2 interp_UV;
} fs_in;

// world space data
vec3 wFragPos; 
vec3 wNormal;

vec3 wPointLightDir[MAX_POINT_LIGHTS];
vec3 wDirLightDir  [MAX_DIR_LIGHTS];

// light space data (l = transformed by lightProjMatrix * lightViewMatrix) (needed for shadow calcs)
vec4 lwDirFragPos[MAX_DIR_LIGHTS]; // fragment position in light space for directional lights

uniform vec3 wCameraPos = vec3(0);

//Lights
uniform uint nPointLights;
uniform uint nDirLights;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];

// Textures
uniform sampler2D g_wFragPos;
uniform sampler2D g_wNormal;

uniform sampler2D directional_shadow_maps[MAX_DIR_LIGHTS]; // TexUnit5 Shadow map 0
uniform samplerCube point_shadow_maps[MAX_POINT_LIGHTS]; // TexUnit??

uniform int sample_shadow_map  = 0; // receive shadows or not

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

// Computed stuff
vec3 curr_wLightDir; // current light position
vec3 wViewDir;
vec2 finalTexCoords;
vec3 finalNormal;

float calculateShadow(sampler2D shadow_map, vec4 lwFragPos, vec3 wLightDir, vec3 normal)
{
	// perform perspective divide
    vec3 projCoords = lwFragPos.xyz / lwFragPos.w;

	// normalize coordinates to be within [0,1] instead of NDC range [-1,1]
	projCoords = projCoords * 0.5 + 0.5; 

	// get fragment depth
	float currentDepth = projCoords.z;  

	// calculate bias to solve shadow acne 
	float shadow_factor = dot(normal, wLightDir);
	float bias = max(0.01 * (1.0 - shadow_factor), 0.005);

	// sample the depthmap obtained from the light POV
	float closestDepth = texture(shadow_map, projCoords.xy).r;
	// if fragment has greater depth, then something occluded it from light POV, thus is in shadow
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
	
	// PCF-filtered shadow sampling
	shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_map, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
			// sample the depthmap obtained from the light POV through the filtering mask
            float pcfDepth = texture(shadow_map, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
	// Average-out the filtering mask
    shadow /= 9.0;

	// don't affect areas outside the shadow_map
	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}

float calculateShadow(samplerCube shadow_cube, vec3 wFragPos, vec3 wLightPos, float far_plane /* TODO temp*/)
{
    // get vector between fragment position and light position
    vec3 fragToLight = wFragPos - wLightPos;
    // use the light to fragment vector to sample from the depth map    
    float closestDepth = texture(shadow_cube, fragToLight).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // now test for shadows
    float bias = 0.05; 
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

	// PCF-filtered shadow sampling
	shadow  = 0.0;
	bias    = 0.05; 
	float samples = 4.0;
	float offset  = 0.1;
	for(float x = -offset; x < offset; x += offset / (samples * 0.5))
	{
		for(float y = -offset; y < offset; y += offset / (samples * 0.5))
		{
			for(float z = -offset; z < offset; z += offset / (samples * 0.5))
			{
				closestDepth = texture(shadow_cube, fragToLight + vec3(x, y, z)).r; 
				closestDepth *= far_plane;   // undo mapping [0;1]
				if(currentDepth - bias > closestDepth)
					shadow += 1.0;
			}
		}
	}
	shadow /= (samples * samples * samples);

    return shadow;
} 

vec3 BlinnPhong()
{
	float shininess_factor = shininess;

	// ambient component 
	vec3 ambient_component = kA * ambient_color.rgb;

	// view direction
	vec3 V = wViewDir;

    vec2 final_texCoords = finalTexCoords;

	// obtain normal from primary normal map
	vec3 N = finalNormal;
	vec4 surface_color = diffuse_color;
	
	// normalization of the per-fragment light incidence direction
	vec3 L = normalize(curr_wLightDir);
	
	// Lambert coefficient
	N = normalize(N);
	float lambertian = max(dot(L,N), 0.0);
	
	vec3 final_color = ambient_component;

	// if the lambert coefficient is positive, then we calculate the specular component
	if(lambertian > 0.0)
	{
		// in the Blinn-Phong model we do not use the reflection vector, but the half vector
		vec3 H = normalize(L + V);
		// we use H to calculate the specular component
		float specAngle = max(dot(H, N), 0.0);
		// shininess application to the specular component
		float spec = pow(specAngle, shininess_factor);

		// calculate diffuse component
		vec3 diffuse_component = kD * lambertian * surface_color.rgb;

		// calculate specular component
		vec3 specular_component = kS * spec * specular_color.rgb;
		
		// We add diffusive and specular components to the final color
		// N.B. ): in this implementation, the sum of the components can be different than 1
		final_color += (diffuse_component + specular_component);
	}
	return final_color;
}

vec3 calculatePointLights()
{
	vec3 color = vec3(0);

	for(int i = 0; i < nPointLights; i++)
	{
		curr_wLightDir = normalize(wPointLightDir[i]);
		float light_distance = length(wPointLightDir[i]);
		float attenuation = 0.001f + // to avoid division by zero
							pointLights[i].attenuation_constant +
							pointLights[i].attenuation_linear * light_distance +
							pointLights[i].attenuation_quadratic * light_distance * light_distance;

		//float shadow = calculateShadow(point_shadow_maps[i], wFragPos, pointLights[i].position, 25.f) * sample_shadow_map;
		float shadow = 0;
		color += (1 - shadow) * BlinnPhong() * pointLights[i].color.rgb * pointLights[i].intensity * (1.f/attenuation);
	}

	return color;
}

vec3 calculateDirLights()
{
	vec3 color = vec3(0);
	
	for(int i = 0; i < nDirLights; i++)
	{
		curr_wLightDir = normalize(wDirLightDir[i]);

		//float shadow = calculateShadow(directional_shadow_maps[i], lwDirFragPos[i], curr_wLightDir, finalNormal) * sample_shadow_map;
		//float shadow = calculateShadow(directional_shadow_maps[i], lwDirFragPos[i], wDirLightDir[i], finalNormal) * sample_shadow_map;
		float shadow = 0;
		color += (1 - shadow) * BlinnPhong() * directionalLights[i].color.rgb * directionalLights[i].intensity;
	}

	return color;
}

void calculateDirLightDir(uint light_idx)
{
	wDirLightDir[light_idx] = -directionalLights[light_idx].direction;
}

void calculatePointLightDir(uint light_idx)
{
	vec3 curr_wPointLightDir = pointLights[light_idx].position - wFragPos; // Calculate light direction in world coordinates
	wPointLightDir[light_idx] = curr_wPointLightDir;
}

void calculateDirLightSpaceFragPos(uint light_idx)
{
	lwDirFragPos[light_idx] = directionalLights[light_idx].lightspace_matrix * vec4(wFragPos, 1);
}


void main()
{
	vec3 color = vec3(0);

	finalTexCoords = fs_in.interp_UV;

	wFragPos = texture(g_wFragPos, finalTexCoords).rgb;
	wNormal = texture(g_wNormal, finalTexCoords).rgb;

	// calculate light related data
	for(uint i = 0; i < nPointLights; i++)
	{
		calculatePointLightDir(i);
	}

	for(uint i = 0; i < nDirLights; i++)
	{
		calculateDirLightDir(i);
		calculateDirLightSpaceFragPos(i);
	}

	wViewDir = normalize( wCameraPos - wFragPos );
	finalNormal = wNormal;

	//color = vec3(1,1,0);
	
	color += calculatePointLights();
	color += calculateDirLights();
	
	colorFrag = vec4(color, 1.0f);
}