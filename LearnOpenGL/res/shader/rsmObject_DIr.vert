#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNorm;

out VS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
	vec4 fragPosLightSpace;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vs_out.fragPos = vec3(model * vec4(aPos, 1.0f));
	vs_out.normal = normalize(mat3(transpose(inverse(model))) * aNorm);
	vs_out.texCoord = aTexCoord;
	vs_out.fragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0f);
}