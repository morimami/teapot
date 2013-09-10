/* [FRAG] */ 
#version 330

in vec3 ex_normal;
in vec3 ex_position;
in vec4 shadowCoord;
in vec4 ex_color;

out vec4 fragColor;

uniform vec4 LightPosition;
uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform sampler2DShadow ShadowMap;
uniform int SubroutineIdx;

void shadeWithShadow(){
	vec3 tnorm = normalize(NormalMatrix * ex_normal);
	vec4 eyeCoords = ModelViewMatrix * vec4(ex_position,1.0);
	vec3 s = normalize(vec3(LightPosition - eyeCoords));
	vec3 v = normalize(-eyeCoords.xyz);
	vec3 r = reflect(-s,tnorm);
	float sDotN = max(dot(s,tnorm),0.0);

	vec3 Kd = vec3(0.5);
	vec3 Ld = vec3(1.0);
	vec3 Ka = vec3(0.5);
	vec3 La = vec3(0.4);
	vec3 Ks = vec3(0.7);
	vec3 Ls = vec3(1.0);

	vec3 diffuse = Ld * Kd * sDotN;
	vec3 ambient = La * Ka;
	vec3 spec = vec3(0.0);
	if(sDotN>0.0)
	  spec = Ls * Ks * pow(max(dot(r,v),0.0),100.0);
	float shadow = textureProj(ShadowMap, shadowCoord);
	vec3 LightIntensity = ambient +(diffuse+spec)*shadow;

    fragColor = vec4(LightIntensity,1.0);

}

void recordDepth()
{

}


void main()
{
	if(SubroutineIdx==1){
		shadeWithShadow();
	} else {
		recordDepth();
	}
}
