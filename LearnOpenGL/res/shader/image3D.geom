#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices=3)out;

in VS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
	vec4 fragPosLightSpace;
}gs_in[];

out GS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
	vec4 fragPosLightSpace;
	int axis;
}gs_out;

uniform mat4 projectionX;
uniform mat4 projectionY;
uniform mat4 projectionZ;
					
void main()
{
	vec3 edge1 = gl_in[0].gl_Position.xyz-gl_in[1].gl_Position.xyz;
	vec3 edge2 = gl_in[2].gl_Position.xyz-gl_in[1].gl_Position.xyz;
	vec3 N = abs(cross(edge1,edge2));

	if(N.x>N.y && N.x>N.z)
	{
		gs_out.axis = 1;
	}
	else if(N.y>N.x && N.y>N.z)
	{
		gs_out.axis = 2;
	}
	else
	{
		gs_out.axis = 3;
	}
	mat4 Projection = gs_out.axis==1?projectionX:gs_out.axis==2?projectionY:projectionZ;

	for(int i=0;i!=3;i++)
	{
		gl_Position = Projection*gl_in[i].gl_Position;
		gs_out.fragPos = gs_in[i].fragPos;
		gs_out.normal = gs_in[i].normal;
		gs_out.texCoord = gs_in[i].texCoord;
		gs_out.fragPosLightSpace = gs_in[i].fragPosLightSpace;
		EmitVertex();
	}
	EndPrimitive();
}