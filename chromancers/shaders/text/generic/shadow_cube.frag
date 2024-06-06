#version 330 core
in vec4 lwFragPos;

uniform vec3 lightPos;
uniform float far_plane;

// Shader fragment to compute a depth map
// Part of the pointlight shadow cube computation
void main()
{
    // get distance between fragment and light source (in light space)
    float lightDistance = length(lwFragPos.xyz - lightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / far_plane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}  
