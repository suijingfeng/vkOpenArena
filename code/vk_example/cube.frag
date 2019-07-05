#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D tex[6];
// layout (binding = 1) uniform int index;

layout (location = 0) in vec4 texcoord;
layout (location = 1) in vec3 frag_pos;
layout (location = 0) out vec4 uFragColor;

const vec3 lightDir= vec3(0.424, 0.566, 0.707);

void main()
{
	int i = 4;
    // dFdx, dFdy, return the partial derivative of an argument with respect to x or y
    // Available only in the fragment shader, these functions return the partial
    // derivative of expression p with respect to the window x coordinate (for dFdx*)
    // and y coordinate (for dFdy*). 
    vec3 dX = dFdx(frag_pos);
    vec3 dY = dFdy(frag_pos);
    vec3 normal = normalize(cross(dX, dY));
    float light = max(1.0, dot(lightDir, normal));
 
	uFragColor = light * texture(tex[i], texcoord.xy);
	
}
