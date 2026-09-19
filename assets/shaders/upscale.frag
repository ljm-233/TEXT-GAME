#version 330 core

uniform sampler2D uSource;
uniform vec2 uSourceSize;
uniform vec2 uOutputSize;

out vec4 fragColor;

float catmullRom(float x) {
    x = abs(x);
    if (x < 1.0) return 1.5 * x * x * x - 2.5 * x * x + 1.0;
    if (x < 2.0) return -0.5 * x * x * x + 2.5 * x * x - 4.0 * x + 2.0;
    return 0.0;
}

vec4 sampleBicubic(sampler2D tex, vec2 uv, vec2 texSize) {
    vec2 coord = uv * texSize - 0.5;
    vec2 base = floor(coord);
    vec2 f = coord - base;

    vec4 sum = vec4(0.0);
    float sumWeight = 0.0;

    for (int j = -1; j <= 2; ++j) {
        for (int i = -1; i <= 2; ++i) {
            float wx = catmullRom(float(i) - f.x);
            float wy = catmullRom(float(j) - f.y);
            float w = wx * wy;
            vec2 samplePos = (base + vec2(float(i), float(j)) + 0.5) / texSize;
            sum += texture(tex, samplePos) * w;
            sumWeight += w;
        }
    }
    return sum / sumWeight;
}

void main() {
    // ⭐ gl_FragCoord 原点在左下角，单位像素
    // ⭐ SFML 的 RenderTexture 已内部翻转 Y，直接用 gl_FragCoord 除尺寸
    vec2 uv = gl_FragCoord.xy / uOutputSize;   // ⭐ OpenGL 左下原 → SFML 左上原

    vec4 c = sampleBicubic(uSource, uv, uSourceSize);

    // 轻度锐化
    vec2 texel = 1.0 / uSourceSize;
    vec3 blur = (
        texture(uSource, uv + vec2(-texel.x, 0.0)).rgb +
        texture(uSource, uv + vec2( texel.x, 0.0)).rgb +
        texture(uSource, uv + vec2(0.0, -texel.y)).rgb +
        texture(uSource, uv + vec2(0.0,  texel.y)).rgb
    ) * 0.25;
    c.rgb += (c.rgb - blur) * 0.4;

    fragColor = c;
}