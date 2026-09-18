#version 330

in vec2 v_Uv;
layout(location=0) out vec4 FragColor;

uniform sampler2D u_Scene;
uniform float u_Time;
uniform vec2 u_Resolution;

void main()
{
	vec2 pixel=1.0/u_Resolution;
    vec3 center=texture(u_Scene,v_Uv).rgb;
    vec3 north=texture(u_Scene,v_Uv+vec2(0,pixel.y)).rgb;
    vec3 south=texture(u_Scene,v_Uv-vec2(0,pixel.y)).rgb;
    vec3 east=texture(u_Scene,v_Uv+vec2(pixel.x,0)).rgb;
    vec3 west=texture(u_Scene,v_Uv-vec2(pixel.x,0)).rgb;
    vec3 weights=vec3(.299,.587,.114);
    float contrast=max(max(dot(north,weights),dot(south,weights)),max(dot(east,weights),dot(west,weights)))
        -min(min(dot(north,weights),dot(south,weights)),min(dot(east,weights),dot(west,weights)));
    vec3 color=mix(center,(north+south+east+west+center*4.0)/8.0,smoothstep(.07,.25,contrast)*.65);
    vec3 bloom=vec3(0);
    for(int i=0;i<8;++i) {
        float a=float(i)*.785398;
        vec3 sampleColor=texture(u_Scene,v_Uv+vec2(cos(a),sin(a))*pixel*5.0).rgb;
        bloom+=max(sampleColor-vec3(.72),vec3(0));
    }
    color+=bloom*.035;
	float edge = distance(v_Uv, vec2(0.5, 0.52));
	float vignette = 1.0 - smoothstep(0.34, 0.82, edge) * 0.22;
	float warmLight = 0.018 * sin(u_Time * 0.32);

	color = pow(max(color, vec3(0.0)), vec3(0.96));
	color += vec3(warmLight, warmLight * 0.72, warmLight * 0.30);
	color *= vignette;
	color = mix(color, color * vec3(1.04, 1.01, 0.97), 0.24);

	FragColor = vec4(color, 1.0);
}
