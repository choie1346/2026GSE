#version 330

layout(location=0) in vec3 a_Position;
out vec2 v_Uv;

void main()
{
	v_Uv = a_Position.xy * 0.5 + 0.5;
	gl_Position = vec4(a_Position, 1.0);
}
