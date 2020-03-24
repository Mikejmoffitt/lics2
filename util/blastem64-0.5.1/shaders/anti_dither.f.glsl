#version 110

uniform sampler2D textures[2];

varying vec2 texcoord;

void main()
{
	float sharpY = (floor(texcoord.y * 512.0) + 0.5)/512.0;
	float sharpX = (floor(texcoord.x * 512.0) + 0.5)/512.0;
	vec2 coordCur = vec2(sharpX, sharpY);
	vec2 coordP1 = vec2(sharpX + 1.0/512.0, sharpY);
	vec2 coordP2 = vec2(sharpX + 2.0/512.0, sharpY);
	vec2 coordN1 = vec2(sharpX - 1.0/512.0, sharpY);
	vec2 coordN2 = vec2(sharpX - 2.0/512.0, sharpY);
	vec2 coordRegular = vec2(texcoord.x, sharpY);
	vec4 cur0 = texture2D(textures[0], coordCur);
	vec4 p20 = texture2D(textures[0], coordP2);
	vec4 cur1 = texture2D(textures[1], coordCur);
	vec4 p21 = texture2D(textures[1], coordP2);
	vec4 color0;
	if (cur0 == p20) {
		color0 = mix(cur0, texture2D(textures[0], coordP1), 0.5);
	} else {
		vec4 pN20 = texture2D(textures[0], coordN2);
		if (cur0 == pN20) {
			color0 = mix(cur0, texture2D(textures[0], coordN1), 0.5);
		} else {
			vec4 n10 = texture2D(textures[0], coordN1);
			if (n10 == texture2D(textures[0], coordP1)) {
				color0 = mix(n10, cur0, 0.5);
			} else {
				color0 = texture2D(textures[0], coordRegular);
			}
		}
	}
	
	vec4 color1;
	if (cur1 == p21) {
		color1 = mix(cur1, texture2D(textures[1], coordP1), 0.5);
	} else {
		vec4 pN21 = texture2D(textures[1], coordN2);
		if (cur1 == pN21) {
			color1 = mix(cur1, texture2D(textures[1], coordN1), 0.5);
		} else {
			vec4 n11 = texture2D(textures[1], coordN1);
			if (n11 == texture2D(textures[1], coordP1)) {
				color1 = mix(n11, cur0, 0.5);
			} else {
				color1 = texture2D(textures[1], coordRegular);
			}
		}
	}
	
	gl_FragColor = mix(
		color1,
		color0,
		(sin(texcoord.y * 1024.0 * 3.14159265359) + 1.0) * 0.5
	);
}
