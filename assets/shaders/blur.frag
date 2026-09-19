#version 330 core

uniform sampler2D uSource;
uniform vec2 uSourceSize;
uniform vec2 uOutputSize;
uniform vec2 uDirection;

out vec4 fragColor;

const float weights[5] = float[](
    0.2270270270,
    0.1945945946,
    0.1216216216,
    0.0540540541,
    0.0162162162
);

void main() {
    vec2 uv = gl_FragCoord.xy / uOutputSize;
    vec2 texel = uDirection / uSourceSize;

    vec3 result = texture(uSource, uv).rgb * weights[0];

    for (int i = 1; i < 5; ++i) {
        vec2 offset = texel * float(i);
        result += texture(uSource, uv + offset).rgb * weights[i];
        result += texture(uSource, uv - offset).rgb * weights[i];
    }

    fragColor = vec4(result, 1.0);
}