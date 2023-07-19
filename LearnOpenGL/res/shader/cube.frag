#version 450 core
//layout(rgba8) uniform restrict image3D tex;
uniform sampler3D tex;
out vec4 fragColor;

uniform vec3 minPos;
uniform vec3 maxPos;
uniform int Step;
uniform vec3 color;
uniform int mip;
in VS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
} fs_in;

ivec3 posTrans(vec3 pos)
{
	vec3 result = pos-(maxPos+minPos)/2;
	result /=(maxPos-minPos);
	result*=Step;
	result+=vec3(Step/2);
	return ivec3(result);
}

vec3 posTransToNdc(vec3 pos)
{
	vec3 result = pos-(maxPos+minPos)/2;
	result /=(maxPos-minPos);
	result+=vec3(0.5);
	return result;
}

void main()
{
	//ivec3 Pos= posTrans(fs_in.fragPos);
	//vec4 result = imageLoad(tex,Pos);
	vec3 Pos= posTransToNdc(fs_in.fragPos);
	vec4 result = textureLod(tex,Pos,float(mip));
	fragColor = vec4(result.xyz,1.0);
}
