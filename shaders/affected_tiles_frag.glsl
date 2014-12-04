uniform lightGrid
{
	ivec4 countAndOffsets[TILES_COUNT];
};

uniform lightColors
{
  vec4 colors[MAX_LIGHTS];
};

uniform isamplerBuffer texLightID;

out vec4 finalColor;


void main() 
{
	//set not affected tiles color to black
	vec3 color = vec3(0.0, 0.0, 0.0);

	//get processed fragment's tile
	ivec2 tile = ivec2(int(gl_FragCoord.x) / TILE_DIM, int(gl_FragCoord.y) / TILE_DIM);

	//get light count and offfset to light list for this tile
	int count = countAndOffsets[tile.x + tile.y * GRID_X].x;
	int offset = countAndOffsets[tile.x + tile.y * GRID_X].y;

	//compute final color of tile
	for (int i = 0; i < count; i++)
	{
		int lightID = texelFetch(texLightID, offset + i).x; 

		color += (colors[lightID].xyz);
	}

	finalColor = vec4(color/count, 1.0);
}

