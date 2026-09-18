// Generated from Shaders by GenerateShaderSources.ps1. Do not edit directly.
#pragma once
namespace ShaderSources {
struct Entry { const char* path; const char* source; };
static const Entry entries[] = {
{"./Shaders/SolidRect.vs", R"GLSL(#version 330

layout(location=0) in vec3 a_Position;
layout(location=1) in float a_Coverage;
layout(location=2) in vec4 a_Tint;
uniform bool u_Model;
uniform vec2 u_ModelScale;
out vec4 v_Tint;
out float v_Coverage;
uniform vec4 u_Trans;

void main()
{
	vec4 newPosition;
	newPosition.xy = a_Position.xy*u_Trans.w + u_Trans.xy;
    if(u_Model) newPosition.xy=a_Position.xy*u_ModelScale+u_Trans.xy;
    v_Tint=u_Model?a_Tint:vec4(1.0);
	newPosition.z = 0;
	newPosition.w= 1;
	gl_Position = newPosition;
    v_Coverage=a_Coverage;
}
)GLSL"},
{"./Shaders/SolidRect.fs", R"GLSL(#version 330

layout(location=0) out vec4 FragColor;
in float v_Coverage;
in vec4 v_Tint;
uniform vec4 u_Flame;

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
	vec3 color=u_Color.rgb*v_Tint.rgb;
    if(u_Material==7) {
        vec2 uv=(gl_FragCoord.xy-u_Flame.xy)/u_Flame.zw;
        float n=noise(vec2(uv.x*4.0,uv.y*5.0-u_Time*3.8));
        float sway=sin(uv.y*5.0-u_Time*4.0)*.10*uv.y;
        float edge=(1.0-uv.y)*.65-abs(uv.x+sway)+(n-.5)*.23;
        float coverage=smoothstep(-.06,.07,edge)*smoothstep(0.0,.08,uv.y);
        vec3 fire=mix(vec3(1.0,.20,.045),vec3(1.0,.90,.42),clamp(edge*2.5,0.0,1.0));
        FragColor=vec4(fire,coverage*(1.0-smoothstep(.85,1.0,uv.y)));
        return;
    }
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
        color=mix(vec3(.24,.29,.19),color,smoothstep(.20,.64,patches));
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
	FragColor = vec4(color,u_Color.a*v_Tint.a*v_Coverage);
}
)GLSL"},
{"./Shaders/Lake.fs", R"GLSL(#version 330
layout(location=0) out vec4 FragColor;
uniform vec4 u_Color;
uniform vec2 u_Origin;
uniform float u_Time;
uniform float u_RenderScale;
uniform bool u_GenericWater;

float waves(vec2 p) {
    return sin(dot(p,vec2(2.1,1.3))+u_Time*1.3)*.48
         + sin(dot(p,vec2(-3.6,2.4))-u_Time*1.7)*.24
         + sin(dot(p,vec2(6.1,4.3))+u_Time*2.1)*.12;
}
void main() {
    vec2 p=gl_FragCoord.xy/max(u_RenderScale,1.0)+u_Origin;
    vec2 world=vec2(p.x/84.0-p.y/42.0,-p.x/84.0-p.y/42.0);
    float shore=1.0-length((world-vec2(27,24))/vec2(5.8,4.5));
    if(u_GenericWater)shore=.30+.14*sin(world.x*.34+world.y*.27);
    float depth=smoothstep(0.0,.65,shore);
    float h=waves(world);
    vec2 slope=vec2(waves(world+vec2(.03,0))-h,waves(world+vec2(0,.03))-h)/.03;
    vec3 normal=normalize(vec3(-slope*.12,1));
    vec3 light=normalize(vec3(-.4,.6,1.0));
    vec3 view=normalize(vec3(0,-.7,1));
    float spec=pow(max(dot(normal,normalize(light+view)),0.0),96.0);
    float fresnel=.08+.45*pow(1.0-max(dot(normal,view),0.0),4.0);
    vec3 water=mix(vec3(.32,.55,.48),vec3(.055,.25,.32),depth);
    vec3 sky=mix(vec3(.43,.65,.72),vec3(.76,.82,.78),.5+.5*h);
    water=mix(water,sky,fresnel);
    float caustic=pow(.5+.5*sin(world.x*12.0+sin(world.y*9.0+u_Time)+u_Time),10.0);
    water+=vec3(.16,.23,.14)*caustic*(1.0-depth)*.35;
    float foam=(1.0-smoothstep(.01,.11,shore))*(.5+.5*sin(shore*160.0-u_Time*2.0+h));
    water+=vec3(.60,.72,.65)*foam*.34;
    water+=vec3(1,.89,.65)*spec*.65;
    FragColor=vec4(water,u_Color.a);
}
)GLSL"},
{"./Shaders/PostProcess.vs", R"GLSL(#version 330

layout(location=0) in vec3 a_Position;
out vec2 v_Uv;

void main()
{
	v_Uv = a_Position.xy * 0.5 + 0.5;
	gl_Position = vec4(a_Position, 1.0);
}
)GLSL"},
{"./Shaders/PostProcess.fs", R"GLSL(#version 330

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
)GLSL"},
{"./Shaders/Text.fs", R"GLSL(#version 330
layout(location=0) out vec4 FragColor;
uniform sampler2D u_Glyph;
uniform vec4 u_TextRect;
uniform vec4 u_Color;
void main() {
    vec2 uv=(gl_FragCoord.xy-u_TextRect.xy)/u_TextRect.zw;
    float coverage=texture(u_Glyph,vec2(uv.x,1.0-uv.y)).r;
    FragColor=vec4(u_Color.rgb,u_Color.a*coverage);
}
)GLSL"},
};
}
