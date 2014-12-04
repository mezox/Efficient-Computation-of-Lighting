uniform sampler2D diff_tex;
uniform sampler2D normal_map;
uniform sampler2D spec_map;

uniform float specExponent;
uniform vec3 Kd;

in vec3 fp;
in vec2 ft;
in vec3 fn;
in vec3 ftan;
in vec3 fbitan;

uniform isamplerBuffer texLightID;

uniform lightGrid
{
	ivec4 countsAndOffsets[TILES_COUNT];
};

uniform lightPosAndRadius
{
	vec4 positionAndRange[MAX_LIGHTS];
};

uniform lightColors
{
  vec4 color[MAX_LIGHTS];
};

out vec4 finalColor;


/*
	Fresnel reflection
*/
vec3 fresnelSchlick(vec3 specular, vec3 E, vec3 H)
{
    return specular + (1.0 - specular) * pow(1.0f - clamp(dot(E, H), 0.0, 1.0), 5.0);
}

/*
	Tiled lighting calculation
*/
vec3 calcLighting(vec3 N, vec3 diffuse, vec3 specular, vec3 V, int lightID)
{
	//get tile light properties
	vec3 lightPos = positionAndRange[lightID].xyz;
	vec3 lightColor = color[lightID].xyz;
	float lightRange = positionAndRange[lightID].w;

	//light direction
	vec3 L = lightPos - fp;

	float dist = length(L);

	L = normalize(L);

	//halfway direction
	vec3 H = normalize(L + V);

	float NdotL = clamp(dot(N, L),0.0,1.0);	//N.L
	float HdotL = clamp(dot(H, L),0.0,1.0);	//H.L
	float NdotH = clamp(dot(N, H),0.0,1.0);	//N.H

	float attenuation = clamp(1.0 - dist*dist/(lightRange*lightRange), 0.0,1.0);
	attenuation *= attenuation;

	//fresnel specular reflection
	vec3 spec = fresnelSchlick(specular, L, H) * ((specExponent + 2.0) / 8.0) * pow((NdotH), specExponent) * NdotL;
		
	return attenuation * NdotL * lightColor * (diffuse + spec);
}

/*
	Calculates bump map normal
*/
vec3 bumpNormal(vec3 normalMap)
{
	//normalize normal, tangent and bitangent from vertex shader
	vec3 normal = normalize(fn);
    vec3 tangent = normalize(ftan);
	vec3 bitangent = normalize(fbitan);

	//get normal in clip space
    vec3 bumpMapNormal = normalMap * vec3(2.0) - vec3(1.0);
    
    return (bumpMapNormal.x * tangent + bumpMapNormal.y * bitangent + bumpMapNormal.z * normal);
}

void main()
{
	vec3 normal = normalize(bumpNormal(texture(normal_map, ft).rgb));
	
	vec3 diffuse = texture(diff_tex, ft).rgb * Kd;
	vec3 specular = texture(spec_map, ft).rgb;
	vec3 ambient = diffuse * 0.05;

	//get tile position
	ivec2 tilePos = ivec2(int(gl_FragCoord.x) / TILE_DIM, int(gl_FragCoord.y) / TILE_DIM);
	
	//view direction
	vec3 V = normalize(-fp);

	int lightCount = countsAndOffsets[tilePos.x + tilePos.y * GRID_X].x;
	int lightOffset = countsAndOffsets[tilePos.x + tilePos.y * GRID_X].y;

	vec3 shadedPixelColor = vec3(0.0, 0.0, 0.0);

	//process tile lights
	for (int i = 0; i < lightCount; ++i)
	{
		int lightID = texelFetch(texLightID, lightOffset + i).x; 
		shadedPixelColor += calcLighting(normal, diffuse, specular, V, lightID);
	}

	finalColor = vec4(shadedPixelColor + ambient, 1.0);
}

