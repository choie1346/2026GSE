#version 330
layout(location=0) out vec4 FragColor;
uniform sampler2D u_Glyph;
uniform vec4 u_TextRect;
uniform vec4 u_Color;
void main() {
    vec2 uv=(gl_FragCoord.xy-u_TextRect.xy)/u_TextRect.zw;
    float coverage=texture(u_Glyph,vec2(uv.x,1.0-uv.y)).r;
    FragColor=vec4(u_Color.rgb,u_Color.a*coverage);
}
