#version 330 core

uniform sampler2D uSource;
uniform vec2 uSourceSize;
uniform vec2 uOutputSize;
uniform float uThreshold;

out vec4 fragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / uOutputSize;
    vec3 c = texture(uSource, uv).rgb;

    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));

    // 软阈值过渡，避免硬边
    float knee = 0.1;
    float soft = clamp(luma - uThreshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-5);
    float contrib = max(soft, luma - uThreshold) / max(luma, 1e-5);

    fragColor = vec4(c * contrib, 1.0);
}