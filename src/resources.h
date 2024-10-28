const char * predatorShader =
    "#version 330\n"
    "\n"
    "// Input vertex attributes (from vertex shader)\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "// Input uniform values\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "\n"
    "// Output fragment color\n"
    "out vec4 finalColor;\n"
    "\n"
    "// NOTE: Add here your custom variables\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Texel color fetching from texture sampler\n"
    "    vec3 texelColor = texture(texture0, fragTexCoord).rgb;\n"
    "    vec3 colors[3];\n"
    "    colors[0] = vec3(0.0, 0.0, 1.0);\n"
    "    colors[1] = vec3(1.0, 1.0, 0.0);\n"
    "    colors[2] = vec3(1.0, 0.0, 0.0);\n"
    "\n"
    "    float lum = (texelColor.r + texelColor.g + texelColor.b)/3.0;\n"
    "\n"
    "    int ix = (lum < 0.5)? 0:1;\n"
    "\n"
    "    vec3 tc = mix(colors[ix], colors[ix + 1], (lum - float(ix)*0.5)/0.5);\n"
    "\n"
    "    finalColor = vec4(tc, 1.0);\n"
    "}\n";

const char * grayscaleShader =
    "#version 330\n"
    "\n"
    "// Input vertex attributes (from vertex shader)\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "// Input uniform values\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "\n"
    "// Output fragment color\n"
    "out vec4 finalColor;\n"
    "\n"
    "// NOTE: Add here your custom variables\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Texel color fetching from texture sampler\n"
    "    vec4 texelColor = texture(texture0, fragTexCoord)*colDiffuse*fragColor;\n"
    "\n"
    "    // Convert texel color to grayscale using NTSC conversion weights\n"
    "    float gray = dot(texelColor.rgb, vec3(0.299, 0.587, 0.114));\n"
    "\n"
    "    // Calculate final fragment color\n"
    "    finalColor = vec4(gray, gray, gray, texelColor.a);\n"
    "}\n"
;

const char * fxaaShader =
    "#version 330\n"
    " \n"
    "#define FXAA_REDUCE_MIN (1.0/128.0)\n"
    "#define FXAA_REDUCE_MUL (1.0/8.0)\n"
    "#define FXAA_SPAN_MAX 8.0\n"
    " \n"
    "in vec2 fragTexCoord;\n"
    " \n"
    "out vec4 fragColor;\n"
    " \n"
    "uniform sampler2D texture0;\n"
    "uniform vec2 resolution;\n"
    " \n"
    "void main()\n"
    "{\n"
    "    vec2 inverse_resolution = vec2(1.0/resolution.x,1.0/resolution.y);\n"
    "    \n"
    "    vec3 rgbNW = texture2D(texture0, fragTexCoord.xy + (vec2(-1.0,-1.0)) * inverse_resolution).xyz;\n"
    "    vec3 rgbNE = texture2D(texture0, fragTexCoord.xy + (vec2(1.0,-1.0)) * inverse_resolution).xyz;\n"
    "    vec3 rgbSW = texture2D(texture0, fragTexCoord.xy + (vec2(-1.0,1.0)) * inverse_resolution).xyz;\n"
    "    vec3 rgbSE = texture2D(texture0, fragTexCoord.xy + (vec2(1.0,1.0)) * inverse_resolution).xyz;\n"
    "    \n"
    "    vec3 rgbM  = texture2D(texture0,  fragTexCoord.xy).xyz;\n"
    "    \n"
    "    vec3 luma = vec3(0.299, 0.587, 0.114);\n"
    " \n"
    "    float lumaNW = dot(rgbNW, luma);\n"
    "    float lumaNE = dot(rgbNE, luma);\n"
    "    float lumaSW = dot(rgbSW, luma);\n"
    "    float lumaSE = dot(rgbSE, luma);\n"
    "    float lumaM  = dot(rgbM,  luma);\n"
    "    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
    "    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE))); \n"
    " \n"
    "    vec2 dir;\n"
    "    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
    "    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
    " \n"
    "    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),FXAA_REDUCE_MIN);\n"
    "    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
    " \n"
    "    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),dir * rcpDirMin)) * inverse_resolution;\n"
    " \n"
    "    vec3 rgbA = 0.5 * (texture2D(texture0,   fragTexCoord.xy   + dir * (1.0/3.0 - 0.5)).xyz + texture2D(texture0,   fragTexCoord.xy   + dir * (2.0/3.0 - 0.5)).xyz);\n"
    "    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture2D(texture0,  fragTexCoord.xy   + dir *  - 0.5).xyz + texture2D(texture0,  fragTexCoord.xy   + dir * 0.5).xyz);\n"
    "    \n"
    "    float lumaB = dot(rgbB, luma);\n"
    " \n"
    "    if((lumaB < lumaMin) || (lumaB > lumaMax)) \n"
    "    {\n"
    "        fragColor = vec4(rgbA, 1.0);\n"
    "    } \n"
    "    else \n"
    "    {\n"
    "        fragColor = vec4(rgbB,1.0);\n"
    "    }\n"
    "}\n";
