uniform sampler2D diff_tex;
uniform sampler2D normal_map;
uniform sampler2D spec_map;

uniform float specExponent;
uniform vec3 Kd;

in vec3 fp;
in vec2 ft;
in vec3 fn;
in vec3 ftan;
in vec3	fbitan;

layout (location = 0) out vec4 mrt_diffuse;
layout (location = 1) out vec4 mrt_normal;
layout (location = 2) out vec4 mrt_pos;
layout (location = 3) out vec4 mrt_spec;
layout (location = 4) out vec4 mrt_ambient;

/*
	Calculates bump map normal using TBN matrix
	[Tx		Ty		Tz]		//tangent
	[BTx	BTy		BTz]	//bitangent
	[Nx		Ny		Nz]		//normal
*/
/*
	Calculates bump map normal using TBN matrix
*/
vec3 bumpNormal(vec3 normalMap)
{
	//normalize normal, tangent and bitangent from vertex shader
	vec3 normal = normalize(fn);
    vec3 tangent = normalize(ftan);
	vec3 bitangent = normalize(fbitan);

    vec3 bumpMapNormal = normalMap * vec3(2.0) - vec3(1.0);
    
    return (bumpMapNormal.x * tangent + bumpMapNormal.y * bitangent + bumpMapNormal.z * normal);
}

void main(){

  vec3 norm = normalize(bumpNormal(texture(normal_map, ft).rgb));
  vec3 diff = texture(diff_tex, ft).rgb;
  vec3 spec = texture(spec_map, ft).rgb;

  vec3 ambient = diff * 0.05;

  mrt_diffuse = vec4(diff, Kd);
  mrt_pos = vec4(fp,1.0);
  mrt_normal = vec4(norm, 1.0);
  mrt_spec = vec4(spec, specExponent);
  mrt_ambient = vec4(ambient,1.0);
}
