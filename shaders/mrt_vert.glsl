uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 vt;
layout (location = 2) in vec3 vn;
layout (location = 3) in vec3 vtan;
layout (location = 4) in vec3 vbitan;

out vec3 fp;
out vec2 ft;
out vec3 fn;
out vec3 ftan;
out vec3 fbitan;

void main()
{
	// Pass some variables to the fragment shader
	ft = vt;
    fn = normalize((view * vec4 (vn, 0.0)).xyz);
    fp = (view * vec4 (vp, 1.0)).xyz;
	ftan = normalize((view * vec4 (vtan, 0.0)).xyz);
	fbitan = normalize((view * vec4 (vbitan, 0.0)).xyz);

	// Apply all matrix transformations to vert
    gl_Position = projection * view  * model * vec4(vp, 1.0);
}