layout(location = 0) in vec3 vp;

out vec2 UV;

void main(){
	gl_Position =  vec4(vp,1);
	UV = (vp.xy+vec2(1,1))/2.0;
}

