#version 450 core
//layout(rgba8) uniform restrict image3D tex;
layout(binding = 0, r32ui) uniform volatile coherent uimage3D tex;

in GS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
	vec4 fragPosLightSpace;
	int axis;
} fs_in;

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

uniform vec3 viewPos;
uniform sampler2D gPositionDepth;

const float PI= 3.14159265359;

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewPos, vec3 fragPos);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
uint convVec4ToRGBA8(vec4 val);
vec4 convRGBA8ToVec4(uint val);
void imageAtomicRGBA8Avg(ivec3 coords, vec4 value);

uniform vec3 minPos;
uniform vec3 maxPos;
uniform int Step;
ivec3 posTrans(vec3 pos)
{
	vec3 result = pos-(maxPos+minPos)/2;
	result /=(maxPos-minPos);
	result*=Step;
	result+=vec3(Step/2);
	return ivec3(result);
}

void main(void)
{
    ivec3 write_Pos=posTrans(fs_in.fragPos);
	vec3 result = calcDirLight(dirlight, fs_in.normal, viewPos, fs_in.fragPos);

	imageAtomicRGBA8Avg(write_Pos, vec4(result,1.0));
    //imageStore(tex,write_Pos,vec4(result,1.0)); 
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewPos, vec3 fragPos)
{
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

	return  (1.0 - shadow) * (diffuse + specular);
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

vec4 convRGBA8ToVec4(uint val)
{
    return vec4(float((val & 0x000000FF)), 
    float((val & 0x0000FF00) >> 8U), 
    float((val & 0x00FF0000) >> 16U), 
    float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | 
    (uint(val.z) & 0x000000FF) << 16U | 
    (uint(val.y) & 0x000000FF) << 8U | 
    (uint(val.x) & 0x000000FF);
}

void imageAtomicRGBA8Avg(ivec3 coords, vec4 val)
{
	uint newVal = packUnorm4x8(val);
	uint prevStoredVal = 0;
	uint curStoredVal;
	// Loop as long as destination value gets changed by other threads
	while ((curStoredVal = imageAtomicCompSwap(tex, coords, prevStoredVal, newVal)) != prevStoredVal)
	{
		prevStoredVal = curStoredVal;
		vec4 rval = unpackUnorm4x8(curStoredVal);
		rval.w *= 256.0;
		rval.xyz = (rval.xyz * rval.w); // Denormalize
		vec4 curValF = rval + val; // Add new value
		curValF.xyz /= (curValF.w); // Renormalize
		curValF.w /= 256.0;
		newVal = packUnorm4x8(curValF);
	}
}