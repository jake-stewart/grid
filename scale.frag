#version 130

uniform sampler2D tex;
uniform float scale;
uniform vec2 geometry;

out vec4 frag_color;


void main() {
    vec2 pixel = gl_TexCoord[0].xy * geometry;

    vec2 uv = floor(pixel) + 0.5;
    uv += (1.0 - clamp((1.0 - fract(pixel)) * scale, 0.0, 1.0));
    
    frag_color = texture(tex, uv / geometry);
}
