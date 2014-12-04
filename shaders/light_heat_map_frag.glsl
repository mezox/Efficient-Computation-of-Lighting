uniform lightGrid
{
	ivec4 CountAndOffsets[TILES_COUNT];
};

out vec4 finalColor;


void main()
{
	//get current fragment's tile pos
	ivec2 tile = ivec2(int(gl_FragCoord.x) / TILE_DIM, int(gl_FragCoord.y) / TILE_DIM);

	//get light count of tile
	int count = CountAndOffsets[tile.x + tile.y * GRID_X].x;
	
	vec3 color;

	if(count == 0)
		 color = vec3(0.0, 0.0, 0.0);
	else if(count <= 15)
		color = vec3(0.0,0.0,1.0);
	else if(count <= 30)
		color = vec3(0.0,1.0,1.0);
	else if(count <= 45)
		color = vec3(0.0,1.0,0.0);
	else if(count <= 60)
		color = vec3(1.0,1.0,0.0);
	else
		color = vec3(1.0,0.0,0.0);

	finalColor = vec4(color, 1.0);
}

