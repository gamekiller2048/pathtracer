#version 430 core

out vec4 FragColor;
  
in vec2 texUV;

uniform sampler2D u_tex;
uniform bool u_horizontal;
const float u_weight[5] = float[] (0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f);
void main()

{             
    vec2 texOffset = 1.0 / textureSize(u_tex, 0); // gets size of single texel
    vec3 result = texture(u_tex, texUV).rgb * u_weight[0]; // current fragment's contribution
    
    if(u_horizontal) {
        for(int i = 1; i < 5; i++) {
            result += texture(u_tex, texUV + vec2(texOffset.x * i, 0.0)).rgb * u_weight[i];
            result += texture(u_tex, texUV - vec2(texOffset.x * i, 0.0)).rgb * u_weight[i];
        }
    }
    else {
        for(int i = 1; i < 5; i++) {
            result += texture(u_tex, texUV + vec2(0.0, texOffset.y * i)).rgb * u_weight[i];
            result += texture(u_tex, texUV - vec2(0.0, texOffset.y * i)).rgb * u_weight[i];
        }
    }

    FragColor = vec4(result, 1.0);
}