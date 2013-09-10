/* [VERT] */ 
#version 330

in vec3 in_Position;
in vec3 in_Normal;

uniform mat4 pvm;
uniform vec4 LightPosition;
uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 ShadowMatrix;

out vec3 ex_normal;
out vec3 ex_position;
out vec4 shadowCoord;
out vec4 ex_color;

void main()
{
    gl_Position = pvm*vec4(in_Position,1.0);
	shadowCoord = ShadowMatrix * vec4(in_Position,1.0);
	ex_normal = in_Normal;
	ex_position = in_Position;
}
