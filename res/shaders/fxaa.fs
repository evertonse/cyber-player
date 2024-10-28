#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec2 resolution;

// Output fragment color
out vec4 finalColor;

void main() {
    // Sample the texture at the current fragment coordinate
    vec2 texCoord = fragTexCoord;
    
    // Calculate one pixel relative to screen size
    vec2 pixelSize = 1.0/resolution;
    
    // Get neighbor samples
    vec3 rgbNW = texture(texture0, texCoord + vec2(-1.0, -1.0)*pixelSize).rgb;
    vec3 rgbNE = texture(texture0, texCoord + vec2(1.0, -1.0)*pixelSize).rgb;
    vec3 rgbSW = texture(texture0, texCoord + vec2(-1.0, 1.0)*pixelSize).rgb;
    vec3 rgbSE = texture(texture0, texCoord + vec2(1.0, 1.0)*pixelSize).rgb;
    vec3 rgbM  = texture(texture0, texCoord).rgb;
    
    // Calculate luma values (perceived brightness)
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM, luma);
    
    // Calculate minimum and maximum luma
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    // Calculate edge contrast
    float lumaRange = lumaMax - lumaMin;
    
    // If contrast is lower than a threshold, no AA is needed
    #define THRESHOLD (0.0312)
    // #define THRESHOLD (0.015)

    if(lumaRange < max(THRESHOLD, lumaMax * 0.125)) {
        finalColor = texture(texture0, texCoord);
        // finalColor = vec4(0.5, 0.5 ,0.9, 1.0) // debug;
        return;
    }
    
    // Calculate blend factor
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * 0.125, 0.0);
    float stepLength = 0.0625;
    float dirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(4.0, 4.0), max(vec2(-4.0, -4.0), dir * dirMin)) * pixelSize;
    
    // Sample in the calculated direction
    vec3 result1 = 0.5 * (
        texture(texture0, texCoord + dir * (1.0/3.0 - 0.5)).rgb +
        texture(texture0, texCoord + dir * (2.0/3.0 - 0.5)).rgb);
    
    vec3 result2 = result1 * 0.5 + 0.25 * (
        texture(texture0, texCoord + dir * -0.5).rgb +
        texture(texture0, texCoord + dir * 0.5).rgb);
    
    float lumaResult = dot(result2, luma);
    
    // Output final color
    if(lumaResult < lumaMin || lumaResult > lumaMax)
        finalColor = vec4(result1, 1.0);
    else
        finalColor = vec4(result2, 1.0);
}
