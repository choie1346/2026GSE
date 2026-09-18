#version 330

layout(location=0) in vec3 a_Position;
layout(location=1) in float a_Coverage;
out float v_Coverage;
uniform vec4 u_Trans;

void main()
{
	vec4 newPosition;
	newPosition.xy = a_Position.xy*u_Trans.w + u_Trans.xy;
	newPosition.z = 0;
	newPosition.w= 1;
	gl_Position = newPosition;
    v_Coverage=a_Coverage;
}
