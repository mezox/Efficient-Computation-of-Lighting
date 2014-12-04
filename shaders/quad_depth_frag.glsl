in vec2 UV;
out vec4 color;

uniform sampler2D in_tex;

float linDepth(float depth){

  float n = 1.0; // camera z near
  float f = 5000.0; // camera z far
  float z = depth;

  return (2.0 * n) / (f + n - z * (f - n));	
}

void main(){
	float d = linDepth(texture(in_tex, UV).b);

	color = vec4(d);
}

