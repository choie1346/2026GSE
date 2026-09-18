#version 330

layout(location=0) out vec4 FragColor;
in float v_Coverage;

uniform vec4 u_Color;
uniform int u_Material;
uniform vec2 u_Origin;
uniform float u_Time;
uniform float u_RenderScale;
uniform vec4 u_Shadow;

float hash(vec2 p) { return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1)),f.x),f.y);
}

void main()
{
	vec2 p=gl_FragCoord.xy/max(u_RenderScale,1.0)+u_Origin;
	vec3 color=u_Color.rgb;
    if(u_Material==6) {
        vec2 offset=gl_FragCoord.xy-u_Shadow.xy;
        vec2 direction=normalize(vec2(.88,-.47));
        float along=dot(offset,direction), across=dot(offset,vec2(-direction.y,direction.x));
        float reach=u_Shadow.w;
        float radius=u_Shadow.z;
        float t=clamp(along/max(reach,1.0),0.0,1.0);
        vec2 q=vec2((along-reach*.45)/(radius+reach*.6),across/(radius*.36+reach*.08));
        float penumbra=mix(.16,.48,t);
        float shadowCoverage=1.0-smoothstep(1.0-penumbra,1.0+penumbra,length(q));
        float contact=exp(-dot(offset/vec2(radius*.7,radius*.25),offset/vec2(radius*.7,radius*.25))*2.0);
        FragColor=vec4(u_Color.rgb,u_Color.a*max(shadowCoverage*.72,contact));
        return;
    }
	if(u_Material==1) {
        vec2 grid=vec2(p.x+floor(p.y/12.0)*13.0,p.y)/vec2(27,12);
        vec2 f=fract(grid);
        float mortar=smoothstep(0.02,0.10,min(f.x,f.y));
        color*=mix(0.70,1.05,mortar)*(0.92+noise(p*.18)*.16);
    } else if(u_Material==2) {
        float patches=noise(p*.025);
        color=mix(vec3(.24,.29,.19),u_Color.rgb,smoothstep(.20,.64,patches));
        color*=.84+noise(p*.18)*.27;
        vec2 blades=vec2(p.x*.8,p.y*.13+sin(p.x*.15+u_Time*.8)*.22);
        float blade=pow(noise(blades),6.0);
        color+=vec3(.22,.28,.10)*blade;
    } else if(u_Material==5) {
        float grain=noise(p*.55);
        color*=.82+noise(p*.05)*.22+grain*.16;
        float stone=smoothstep(.76,.87,noise(p*.23));
        color=mix(color,vec3(.51,.49,.40),stone*.55);
    } else if(u_Material==3) {
        float wave=sin(p.y*.13+sin(p.x*.025+u_Time)*2.0-u_Time*1.7);
        float ripple=sin(p.x*.035+p.y*.09+u_Time*.9);
        color+=vec3(.12,.19,.18)*pow(max(0.0,wave*ripple),8.0);
        color*=.9+noise(p*.013+vec2(u_Time*.02,0))*.25;
    } else if(u_Material==4) {
        vec2 f=fract(vec2(p.x+floor(p.y/8.0)*5.0,p.y)/vec2(12,8));
        color*=.75+.25*smoothstep(.02,.16,min(f.x,f.y));
        color+=noise(p*.3)*.04;
    }
    if(u_Material!=0) {
        float clouds=noise(p*.002+vec2(u_Time*.009,0));
        color*=.92+.11*clouds;
        color*=vec3(1.035,1.01,.965);
    }
	FragColor = vec4(color,u_Color.a*v_Coverage);
}
