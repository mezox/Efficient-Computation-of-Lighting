uniform sampler2D depthTex;

uniform mat4 inverseProjectionMatrix;

out vec4 resultMinMax;

//converts clip space depth to view space
float convertToSS(float depth)
{
	vec4 pt = inverseProjectionMatrix * vec4(0.0, 0.0, 2.0 * depth - 1.0, 1.0);
	return pt.z/pt.w;
}

void main()
{
	ivec2 resolution = ivec2(WIDTH, HEIGHT);

	vec2 minMax = vec2(1.0f, -1.0f);

	//calc offset and range of tile
	ivec2 offset = ivec2(gl_FragCoord.xy) * ivec2(TILE_DIM, TILE_DIM);
	ivec2 end = min(resolution, offset + ivec2(TILE_DIM, TILE_DIM));

	for (int y = offset.y; y < end.y; y++)
	{
		for (int x = offset.x; x < end.x; x++)
		{
			float d = texelFetch(depthTex, ivec2(x, y), 0).x;

			if (d < 1.0)
			{
				minMax.x = min(minMax.x, d);
				minMax.y = max(minMax.y, d);
			}
		}
	}

	vec2 result = minMax;

	minMax = vec2(convertToSS(minMax.x), convertToSS(minMax.y));
	resultMinMax = vec4(minMax,result);
}
