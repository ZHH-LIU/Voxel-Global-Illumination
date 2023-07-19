#version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D texColorBuffer;

uniform float exposure;

void main()
{
	vec3 hdrColor = vec3(texture(texColorBuffer, texCoord).rgb);
	//Reinhard
	//vec3 mapped = hdrColor / (hdrColor + vec3(1.0f));
	//exposure
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
	//gamma
	float gamma = 2.2;
	mapped = pow(mapped, vec3(1.0 / gamma));

	fragColor = vec4(mapped, 1.0);
}