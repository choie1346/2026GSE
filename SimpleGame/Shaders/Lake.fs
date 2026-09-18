#version 330
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
