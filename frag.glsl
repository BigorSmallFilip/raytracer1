#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

// G-Buffer
uniform sampler2D gAlbedoSpecular;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDepth;

uniform sampler2D testTexture;

void main() {
    vec4 albedoSpecular = texture(gAlbedoSpecular, TexCoords);
    vec3 albedo = albedoSpecular.rgb;
    float specular = albedoSpecular.a;
    vec3 position = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    float depth = texture(gDepth, TexCoords).r;

    vec3 color = vec3(0);
    color = albedo * dot(normal, -normalize(position));
    
    if (depth < 1000000000) {
        vec3 fogColor = vec3(0.6, 0.6, 1.0);
        float fogFactor = 1 - (40000 - depth) / (40000);
        color = mix(color, fogColor, fogFactor);
    }

    //color = position / 3000;

    FragColor = vec4(color, 0);
}
