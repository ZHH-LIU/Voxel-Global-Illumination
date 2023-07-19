#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNorm;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
	gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0f);
	fragPos = vec3(model * vec4(aPos, 1.0f));
	mat3 normModel = transpose(inverse(mat3(model)));
	normal = normalize(normModel * aNorm);
	texCoord = aTexCoord;
}