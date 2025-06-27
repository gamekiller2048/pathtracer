#version 430 core

uniform sampler2D u_tex;
uniform sampler2D u_bloom;
in vec2 texUV;

void main()
{
    vec3 color = texture(u_tex, texUV).rgb;

    vec3 bloomColor = texture(u_bloom, texUV).rgb;
    color += bloomColor;

    // Reinhard tone mapping
    //color = color / (color + vec3(1.0));

    // exposure tone mapping
    color = vec3(1.0) - exp(-color * 0.1f);
    
    // gamma correction
    color = pow(color, vec3(1.0f / 2.2f));

    gl_FragColor = vec4(color, 1.0f);
}