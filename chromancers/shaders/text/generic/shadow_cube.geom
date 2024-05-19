#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 lightspace_matrices[6];

out vec4 FragPos; // FragPos from GS (output per emitvertex)

// The geometry shader will take as input 3 triangle vertices and a uniform array of light space transformation matrices. 
// The geometry shader is responsible for transforming these vertices to the light spaces, for a total of 18 vertices (3 vertices per each of the 6 faces)
void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = lightspace_matrices[face] * FragPos; // transform vertex from world space to light space
            EmitVertex();
        }    
        EndPrimitive();
    }
}  