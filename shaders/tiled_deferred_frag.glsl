uniform sampler2D texColor;
uniform sampler2D texNormal;
uniform sampler2D texPos;
uniform sampler2D texSpec;
uniform isamplerBuffer texLightID;

//uniform buffers for grid and light properties
uniform lightGrid
{
	ivec4 countsAndOffsets[TILES_COUNT];
};

uniform lightPosAndRadius
{
  vec4 positionAndRadius[MAX_LIGHTS];
};

uniform lightColors
{
  vec4 color[MAX_LIGHTS];
};

out vec4 finalColor;

//fresnel schlick reflection
vec3 fresnelSchlick(vec3 specular, vec3 E, vec3 H)
{
    return specular + (1.0 - specular) * pow(1.0f - clamp(dot(E, H), 0.0, 1.0), 5.0);
}


vec3 calcLighting(vec3 position, vec3 N, vec3 diffuse, vec3 specular, float shininess, vec3 V, int lightID)
{
	//get light's properties
	vec3 lightPos = positionAndRadius[lightID].xyz;
	vec3 lightColor = color[lightID].xyz;
	float lightRange = positionAndRadius[lightID].w;

	//light direction
	vec3 L = lightPos - position;

	float dist = length(L);

	L = normalize(L);

	//halfway direction
	vec3 H = normalize(L + V);

	float NdotL = clamp(dot(N, L),0.0,1.0);	//N.L
	float HdotL = clamp(dot(H, L),0.0,1.0);	//H.L
	float NdotH = clamp(dot(N, H),0.0,1.0);	//N.H

	//float attenuation =  0.0 + 0.000 * dist + (1.0/(range * range * 0.01)) * dist * dist;

	float attenuation = clamp(1.0 - dist*dist/(lightRange*lightRange), 0.0,1.0);
	attenuation *= attenuation;

	//fresnel specular reflection
	vec3 spec = fresnelSchlick(specular, L, H) * ((shininess + 2.0) / 8.0) * pow((NdotH), shininess) * NdotL;
		
	return (NdotL * lightColor * (diffuse + spec) * attenuation);
}

void main() {

    //get texture coordinates
    vec2 texCoord = gl_FragCoord.xy;

	//extract material and geometry informations from textures
	vec3 diffuse = texelFetch(texColor, ivec2(texCoord), 0).rgb;
    vec3 normal = texelFetch(texNormal, ivec2(texCoord), 0).rgb;
	vec3 position = texelFetch(texPos, ivec2(texCoord), 0).rgb;
	vec3 specular = texelFetch(texSpec, ivec2(texCoord), 0).rgb;
	float shininess = texelFetch(texSpec, ivec2(texCoord), 0).a;

	ivec2 tile = ivec2(int(gl_FragCoord.x) / TILE_DIM, int(gl_FragCoord.y) / TILE_DIM);
	
	vec3 V = normalize(-position);

	int lightCount = countsAndOffsets[tile.x + tile.y * GRID_X].x;
	int lightOffset = countsAndOffsets[tile.x + tile.y * GRID_X].y;

	vec3 color = vec3(0.0, 0.0, 0.0);

	for (int i = 0; i < lightCount; ++i)
	{
		int lightID = texelFetch(texLightID, lightOffset + i).x; 
		color += calcLighting(position, normal, diffuse, specular, shininess, V, lightID);
	}
  
	finalColor = vec4(color, 1.0);
}

