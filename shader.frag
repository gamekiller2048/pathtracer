#version 430 core

uniform sampler2D tex;
in vec2 texUV;

void main()
{
    vec3 color = texture(tex, texUV).rgb;

    // Reinhard tone mapping
    color = color / (color + vec3(1.0));

    // gamma correction
    color = pow(color, vec3(1.0f / 2.2f));

    gl_FragColor = vec4(color, 1.0f);
}