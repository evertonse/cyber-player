#version 330
// FXAA shader, GLSL code adapted from:
// http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA
// Whitepaper describing the technique:
// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
float R_fxaaSpanMax = 8.0f;
float R_fxaaReduceMin = 1.0f/128.0f;
float R_fxaaReduceMul = 1.0f/8.0f;

// Output fragment color
out vec4 finalColor;


void main() {
    vec2 texCoordOffset =  1.0/resolution;

    vec2 texCoord = fragTexCoord;

    vec3 luma    = vec3(0.299, 0.587, 0.114);
    float lumaTL = dot(luma, texture2D(texture0, texCoord.xy + (vec2(-1.0, -1.0) * texCoordOffset)).xyz);
    float lumaTR = dot(luma, texture2D(texture0, texCoord.xy + (vec2(1.0, -1.0) * texCoordOffset)).xyz);
    float lumaBL = dot(luma, texture2D(texture0, texCoord.xy + (vec2(-1.0, 1.0) * texCoordOffset)).xyz);
    float lumaBR = dot(luma, texture2D(texture0, texCoord.xy + (vec2(1.0, 1.0) * texCoordOffset)).xyz);
    float lumaM  = dot(luma, texture2D(texture0, texCoord.xy).xyz);

    vec2 dir;
    dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
    dir.y = ((lumaTL  + lumaBL) - (lumaTR + lumaBR));

    float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (R_fxaaReduceMul * 0.25), R_fxaaReduceMin);
    float inverseDirAdjustment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(R_fxaaSpanMax, R_fxaaSpanMax),
        max(vec2(-R_fxaaSpanMax, -R_fxaaSpanMax), dir * inverseDirAdjustment)) * texCoordOffset;

    vec3 result1 = (1.0/2.0) * (
        texture2D(texture0, texCoord.xy + (dir * vec2(1.0/3.0 - 0.5))).xyz +
        texture2D(texture0, texCoord.xy + (dir * vec2(2.0/3.0 - 0.5))).xyz);

    vec3 result2 = result1 * (1.0/2.0) + (1.0/4.0) * (
        texture2D(texture0, texCoord.xy + (dir * vec2(0.0/3.0 - 0.5))).xyz +
        texture2D(texture0, texCoord.xy + (dir * vec2(3.0/3.0 - 0.5))).xyz);

    float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
    float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));
    float lumaResult2 = dot(luma, result2);

    if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
        finalColor = vec4(result1, 1.0);
    else
        finalColor = vec4(result2, 1.0);
}
