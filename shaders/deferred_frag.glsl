uniform sampler2D texColor;
uniform sampler2D texNormal;
uniform sampler2D texPos;
uniform sampler2D texSpec;

uniform struct Light
{
   vec4 positionRadius;
   vec3 color;
} light;

out vec4 finalColor;

vec3 fresnelSchlick(vec3 specular, vec3 E, vec3 H)
{
    return specular + (1.0 - specular) * pow(1.0f - clamp(dot(E, H), 0.0, 1.0), 5.0);
}

void main() {

    //calculates texture coordinates
    vec2 texCoord = gl_FragCoord.xy;

    vec3 normal = texelFetch(texNormal, ivec2(texCoord), 0).rgb;
	vec3 position = texelFetch(texPos, ivec2(texCoord), 0).rgb;
	vec3 diffuse = texelFetch(texColor, ivec2(texCoord), 0).rgb;
	vec3 specular = texelFetch(texSpec, ivec2(texCoord), 0).rgb;
	float shininess = texelFetch(texSpec, ivec2(texCoord), 0).a;

    //normalize normals
    normal = normalize(normal);

	//transform light and camera positions to view space
	vec3 lightPos = light.positionRadius.xyz;

    vec3 surfaceToLight = normalize(lightPos - position);
    vec3 surfaceToCamera = normalize(-position);

    //diffuse
	float diffuseCoefficient = clamp(dot(normal, surfaceToLight),0.0,1.0);

    //specular

		vec3 halfwayDirection = normalize(surfaceToLight + surfaceToCamera);
		vec3 fresnelSpec = specular + (vec3(1.0) - specular) * pow(max(0.0,1.0 - dot(surfaceToLight, halfwayDirection)), 5.0);
	
		float normalizationFactor = ((shininess + 2.0) / 8.0);
		vec3 spec = fresnelSpec * pow(max(0.0, dot(normal, halfwayDirection)), shininess) * normalizationFactor * dot(normal, surfaceToLight);

	// attenuation (fade out to sphere edges)
    float dist = length(lightPos - position);
	float atten_factor = max(1.0 - max(0.0, (dist / light.positionRadius.a)), 0.0);

	//linear color (color before gamma correction)
    vec3 linearColor = atten_factor * diffuseCoefficient * light.color * (diffuse + spec);

    //final color (after gamma correction)
    //vec3 gamma = vec3(1.0/2.2);
    //finalColor = vec4(pow(linearColor, gamma), 1.0);
    finalColor = vec4(linearColor, 1.0);
}
