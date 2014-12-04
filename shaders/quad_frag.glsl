in vec2 UV;
out vec4 color;

uniform sampler2D in_tex;

void main(){
	color = texture(in_tex, UV) ;
}

