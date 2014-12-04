uniform mat4 MVP;

in vec3 vp;

void main(){
    gl_Position = MVP * vec4(vp, 1.0);
}
