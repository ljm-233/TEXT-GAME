#version 330 core

uniform sampler2D uSource;
uniform sampler2D uBloomTex;
uniform vec2 uOutputSize;

uniform float uSaturation;
uniform float uContrast;
uniform float uBrightness;
uniform float uGamma;
uniform float uVignette;
uniform float uBloomStrength;
uniform float uChromatic;
uniform float uGrain;
uniform float uTime;
uniform float uScanline;
uniform float uDither;

out vec4 fragColor;

const float bayer4x4[16] = float[](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

void main() {
    vec2 uv = gl_FragCoord.xy / uOutputSize;
    vec4 src = texture(uSource, uv);
    vec3 c = src.rgb;

    if (uBloomStrength > 0.001) {
        vec3 bloom = texture(uBloomTex, uv).rgb;
        c += bloom * uBloomStrength;
    }

    c *= uBrightness;
    c = (c - 0.5) * uContrast + 0.5;

    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    c = mix(vec3(luma), c, uSaturation);

    c = pow(max(c, vec3(0.0)), vec3(1.0 / max(uGamma, 0.01)));

    if (uVignette > 0.001) {
        vec2 d = uv - 0.5;
        float r = length(d) * 1.4142;
        float v = smoothstep(1.0, 0.35, r);
        v = mix(1.0, v, uVignette);
        c *= v;
    }

    if (uChromatic > 0.001) {
        vec2 d = uv - 0.5;
        float amt = uChromatic * 0.006;
        vec2 offset = d * amt;
        c.r = texture(uSource, uv + offset).r;
        c.b = texture(uSource, uv - offset).b;
    }

    if (uGrain > 0.001) {
        float n = fract(sin(dot(uv * (uTime + 1.0), vec2(12.9898, 78.233))) * 43758.5453);
        c += (n - 0.5) * uGrain * 0.15;
    }

    if (uScanline > 0.001) {
        float line = mod(gl_FragCoord.y, 2.0);
        c *= mix(1.0, 0.65, line * uScanline);
    }

    if (uDither > 0.001) {
        ivec2 p = ivec2(gl_FragCoord.xy) % 4;
        float bayer = bayer4x4[p.y * 4 + p.x] / 16.0 - 0.5;
        float levels = mix(256.0, 12.0, uDither);
        c = floor(c * levels + bayer) / levels;
    }

    fragColor = vec4(clamp(c, 0.0, 1.0), src.a);
}