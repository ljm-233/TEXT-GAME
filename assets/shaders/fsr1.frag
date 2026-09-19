#version 330 core

uniform sampler2D uSource;
uniform vec2 uSourceSize;
uniform vec2 uOutputSize;

out vec4 fragColor;

// ============================================================
// EASU (Edge Adaptive Spatial Upsampling) - FSR 1.0
// ============================================================

#define FSR_EPS 0.0001
#define FSR_PI  3.14159265358979323846

// Lanczos2 核
float fsrLanczos2(float x) {
    if (x > 2.0) return 0.0;
    if (x < FSR_EPS) return 1.0;
    float pix = FSR_PI * x;
    return (sin(pix) / pix) * (sin(pix * 0.5) / (pix * 0.5));
}

// 12 个采样点位置（相对中心）
const vec2 FSR_SAMPLES[12] = vec2[12](
    vec2( 1.0,  0.0), vec2(-1.0,  0.0), vec2( 0.0,  1.0), vec2( 0.0, -1.0),
    vec2( 1.0,  1.0), vec2(-1.0,  1.0), vec2( 1.0, -1.0), vec2(-1.0, -1.0),
    vec2( 2.0,  0.0), vec2(-2.0,  0.0), vec2( 0.0,  2.0), vec2( 0.0, -2.0)
);

vec3 fsrEasu(sampler2D tex, vec2 uv, vec2 texSize, vec2 outputSize) {
    vec2 texel = 1.0 / texSize;

    // 中心像素
    vec3 c = texture(tex, uv).rgb;

    // 用邻域估算局部对比度（边缘强度）
    vec3 n = texture(tex, uv + vec2(0.0, -texel.y)).rgb;
    vec3 s = texture(tex, uv + vec2(0.0,  texel.y)).rgb;
    vec3 e = texture(tex, uv + vec2( texel.x, 0.0)).rgb;
    vec3 w = texture(tex, uv + vec2(-texel.x, 0.0)).rgb;
    float edge = length(n + s + e + w - c * 4.0);

    // 边缘强度高 → 更锐利的核（抑制边缘模糊）
    float edgeFactor = clamp(edge * 2.0, 0.0, 1.0);

    // 12 采样加权
    vec3 sum = c * 4.0;
    float wsum = 4.0;
    for (int i = 0; i < 12; ++i) {
        vec2 offset = FSR_SAMPLES[i];
        // 沿边缘方向的采样权重降低（保持边缘）
        float dist = length(offset);
        float w2 = fsrLanczos2(dist);
        // 边缘越强，十字方向权重越低
        float axisAlign = 1.0 - edgeFactor * max(0.0, 1.0 - abs(offset.x * offset.y) * 2.0);
        w2 *= axisAlign;

        vec3 s2 = texture(tex, uv + offset * texel).rgb;
        sum  += s2 * w2;
        wsum += w2;
    }
    return sum / wsum;
}

// ============================================================
// RCAS (Robust Contrast Adaptive Sharpening) - FSR 1.0
// ============================================================

vec3 fsrRcas(sampler2D tex, vec2 uv, vec2 texSize, float sharpness) {
    vec2 texel = 1.0 / texSize;
    vec3 c = texture(tex, uv).rgb;

    // 8 邻域 + 中心，取 min/max 估算局部对比度
    vec3 mn = c, mx = c;
    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            if (i == 0 && j == 0) continue;
            vec3 s = texture(tex, uv + vec2(float(i), float(j)) * texel).rgb;
            mn = min(mn, s);
            mx = max(mx, s);
        }
    }
    vec3 contrast = mx - mn;
    // 高频区域少锐化，低频区域多锐化
    float peak = -1.0 / mix(8.0, 3.0, sharpness);
    vec3 wgt = clamp(contrast * peak, 0.0, 1.0);

    // 4 方向锐化
    vec3 n = texture(tex, uv + vec2(0.0, -texel.y)).rgb;
    vec3 s = texture(tex, uv + vec2(0.0,  texel.y)).rgb;
    vec3 e = texture(tex, uv + vec2( texel.x, 0.0)).rgb;
    vec3 w = texture(tex, uv + vec2(-texel.x, 0.0)).rgb;
    vec3 sharpened = (c + (n + s + e + w) * wgt * 0.25) / (1.0 + wgt);

    // 保持亮度
    return mix(c, sharpened, 0.9);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uOutputSize;
    vec3 col = fsrEasu(uSource, uv, uSourceSize, uOutputSize);
    fragColor = vec4(col, texture(uSource, uv).a);
}