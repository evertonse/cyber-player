#version 330

//
// Taken from : https://github.com/tsoding/musializer.git
// All rights to tsoding uwu
//

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

uniform float radius;
uniform float power;


out vec4 finalColor; // Output fragment color
void main() {
    float usedPower = power;
    float r = radius;
    vec2 p = fragTexCoord - vec2(0.5);
    if (length(p) <= 0.5) {
        float s = length(p) - r;
        if (s <= 0) {
            finalColor = fragColor * 1.5;
        } else {
            float t = 1 - s / (0.5 - r);
            finalColor =
                mix(vec4(fragColor.xyz, 0), fragColor * 1.5, pow(t, usedPower));
        }
    } else {
        finalColor = vec4(0);
    }
}
