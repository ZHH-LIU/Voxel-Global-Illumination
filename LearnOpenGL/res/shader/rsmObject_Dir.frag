#version 450 core
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec3 worldNorm;

in VS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
	vec4 fragPosLightSpace;
} fs_in;

uniform vec2 samples[256];

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D gFlux;

uniform vec3 viewPos;
uniform bool onePass;
struct Material {
	//vec3 ambient;
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};
uniform Material material;

struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
uniform DirLight dirlight;

const float PI= 3.14159265359;
const float rmax = 0.3; 

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewPos, vec3 fragPos);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

void main()
{
	//direction	
	vec3 result  = calcDirLight(dirlight, fs_in.normal, viewPos, fs_in.fragPos);

	fragColor = vec4(result,1.0);
	worldPos = fs_in.fragPos;
	worldNorm = fs_in.normal;
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewPos, vec3 fragPos)
{
	//ambient
	vec3 projCoord = fs_in.fragPosLightSpace.xyz / fs_in.fragPosLightSpace.w;
	projCoord = projCoord * 0.5 + 0.5;

	vec3 GI=vec3(0.0);
	for(int i=0;i!=256;i++)
	{
		vec2 aSample = rmax * samples[i].x * vec2(sin(2.0f * PI * samples[i].y), cos(2.0f * PI * samples[i].y));
		vec2 samCoord = projCoord.xy+aSample;
		
		vec3 Flux = texture(gFlux, samCoord).rgb;
		vec3 Np = texture(gNormal, samCoord).rgb;
		vec3 Pp = texture(gPositionDepth, samCoord).rgb;

		vec3 lightDir = fs_in.fragPos-Pp;
		float l4 = pow(length(lightDir),4.0);
		
		vec3 Ep = Flux * max(dot(Np,lightDir),0.0) * max(dot(fs_in.normal,-lightDir),0.0) / l4;
		
		GI+=samples[i].x*samples[i].x*Ep;
	}
	//GI /=256.0f;
	vec3 ambient = GI*texture(material.diffuse,fs_in.texCoord).rgb;

	if(!onePass)
		return ambient;

	//diffuse
	vec3 lightDir = normalize(-light.direction);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, fs_in.texCoord));
	//specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 halfwayDir = normalize(viewDir + lightDir);
	float spec = pow(max(dot(halfwayDir, normal), 0.0), material.shininess);
	vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.texCoord));

	float shadow = ShadowCalculation(fs_in.fragPosLightSpace, normal, lightDir);

	return  ambient + (1.0 - shadow) * (diffuse + specular);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
	vec3 projCoord = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoord = projCoord * 0.5 + 0.5;
	float currentDepth = projCoord.z;
	float shadow = 0.0f;

	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

	//PCF
	vec2 texelSize = 1.0 / textureSize(gPositionDepth, 0);
	for (int i = 0; i != 3; i++)
	{
		for (int j = 0; j != 3; j++)
		{
			float pcfDepth = texture(gPositionDepth, projCoord.xy + vec2(i, j) * texelSize).a;

			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;;
		}
	}
	
	shadow /= 9.0;

	//Ô¶´¦
	if (projCoord.z > 1.0f)
		shadow = 0.0f;

	return shadow;
}
