varying vec3 s_PixelColor;
varying float s_StarLight;

void main(void)
{
    vec4 color = vec4(1.0, 0.0, 0.0, 1.0);
    if(gl_TexCoord[0].x > 0.125 && gl_TexCoord[0].x < 0.85)
        if(gl_TexCoord[0].y > 0.125 && gl_TexCoord[0].y < 0.85)
            color = vec4(s_PixelColor, 0.2);
    gl_FragColor = color;
}
