uniform float s_StarLightIntensity;

varying vec3 s_PixelColor;
varying float s_StarLight;

void main(void)
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	s_PixelColor = gl_Color.xyz;
	s_StarLight = gl_Color[3] * s_StarLightIntensity;
}
