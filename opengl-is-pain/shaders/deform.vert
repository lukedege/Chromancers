// vertex position in world coordinates
// the number used for the location in the layout qualifier is the position of the vertex attribute
// as defined in the Mesh class
// #version 410 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 UV;

uniform mat4 u_model, u_view, u_proj;
uniform mat3 u_norm; 
// for normals we care only about the mat3 rotational submatrix of the model
// such that we ignore the translation component (vertical vector to the right)
// and the scale component (normals are versors, so they're always normalized)

uniform float u_weight;
uniform float u_time;
uniform float u_freq;
uniform float u_power;
uniform float u_harmonics;

out vec3 interp_N;
out vec2 interp_UV;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    //vec3 flattened = position;
    //flattened.z = 0;

    //float disp = u_weight * sin(u_time) + u_weight;
    //vec3 displaced = position + normal*disp;
    
    vec3 final_pos = position;

    gl_Position = u_proj * u_view * u_model * vec4(final_pos, 1.0f);
    interp_N = normalize(u_norm * normal);
    interp_UV = UV;
}
