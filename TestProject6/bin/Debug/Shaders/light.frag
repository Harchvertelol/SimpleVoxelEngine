uniform sampler2D s_TextureLayerID;

uniform vec4 s_StarLightColor;

varying vec3 s_PixelColor;
varying float s_StarLight;

vec4 colorDown(in vec4 color)
{
    float dist = 0.0;
    for(int i = 0; i < 3; i++)
        if(color[i] > 1.0 && color[i] - 1.0 > dist) dist = color[i] - 1.0;
    for(int i = 0; i < 3; i++) color[i] = color[i] - dist;
    return color;
}

vec4 getLight(in vec4 color, in float starlight)
{
    return colorDown(color + vec4(s_StarLightColor.xyz, 0.0) * starlight);
}

void main()
{
    float starlight = s_StarLight;
    vec4 color = texture2D(s_TextureLayerID, vec2(gl_TexCoord[0]));
    gl_FragColor = vec4((color * getLight(vec4(s_PixelColor, 1.0), starlight)).xyz, color.a);
}
