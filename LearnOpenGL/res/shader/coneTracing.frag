#version 450 core
uniform sampler3D tex;
out vec4 fragColor;

uniform vec3 minPos;
uniform vec3 maxPos;
uniform int Step;

in VS_OUT{
	vec3 fragPos;
	vec3 normal;
	vec2 texCoord;
	vec4 fragPosLightSpace;
} fs_in;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	float roughness;
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

vec3 coneDirections[6] = {vec3(0, 0, 1),
                          vec3(0, 0.866025,0.5),
                          vec3(0.823639, 0.267617, 0.5),
                          vec3(0.509037, -0.700629, 0.5),
                          vec3(-0.509037, -0.700629, 0.5),
                          vec3(-0.823639, 0.267617, 0.5)};
float weight[6] =  {1.0/4.0, 
					3.0/20.0,
					3.0/20.0,
					3.0/20.0,
					3.0/20.0,
					3.0/20.0};
const float PI = 3.14159265359;
const float tan30 = tan(PI/6.0);
const float MAX_ALPHA = 1.0;
const float MAX_LENGTH = length(maxPos-minPos);
const float lambda =0.1;
const float stepValue=0.1;
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewPos, vec3 fragPos);
vec3 posTransToNdc(vec3 pos);
vec4 coneTracing(vec3 direction, vec3 N, float tanValue);

void main()
{
	//ambient
	vec3 normal = normalize(fs_in.normal);
	vec3 tangent = normal.z > 0.001 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
	vec3 bitangent = cross(normal, tangent);
	tangent = cross(bitangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	vec4 ambient = vec4(0.0);
	for(int i=0;i!=6;i++)
	{
		ambient+=coneTracing(normalize(TBN * coneDirections[i]),fs_in.normal, tan30)*weight[i];
	}
	ambient.xyz*=(1.0-ambient.w);

	vec3 viewDir=fs_in.fragPos-viewPos;
	vec3 reflectDir = reflect(viewDir,fs_in.normal);
	ambient.xyz +=coneTracing(reflectDir,fs_in.normal, tan(sin(material.roughness*PI/2.0)*PI/2.0)).xyz;

	ambient.xyz*=texture(material.diffuse, fs_in.texCoord).xyz;

	//direct
	vec3 direct = calcDirLight(dirlight, fs_in.normal, viewPos, fs_in.fragPos);
	vec3 result = ambient.xyz+ direct;
	fragColor = vec4(result.xyz,1.0);
}

vec3 posTransToNdc(vec3 pos)
{
	vec3 result = pos-(maxPos+minPos)/2;
	result /=(maxPos-minPos);
	result+=vec3(0.5);
	return result;
}

vec4 coneTracing(vec3 direction, vec3 N, float tanValue)
{
	float voxelSize = (maxPos-minPos).x/Step;
	vec3 start = fs_in.fragPos+N*voxelSize;
	
	vec3 color = vec3(0.0);
	float alpha =0.0;
	float occlusion=0.0;
	float t = voxelSize;
	while(alpha<MAX_ALPHA && t<MAX_LENGTH)
	{
		float d = max(voxelSize, 2.0*t*tanValue);
		float mip = log2(d/voxelSize);
		vec3 texCoord3D= posTransToNdc(start+t*direction);
		vec4 result = textureLod(tex,texCoord3D,mip);
		color += (1.0-alpha)*result.a*result.rgb;
		alpha += (1.0-alpha)*result.a;
		occlusion +=(1.0-occlusion)*result.a/(1.0+lambda*t);
		t+=d*stepValue;
	}
	return vec4(color,occlusion); 
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