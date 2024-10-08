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
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(gAlbedoSpecular, 0));
    vec4 albedoSpecular = texture(gAlbedoSpecular, TexCoords);
    vec3 albedo = albedoSpecular.rgb;
    float specular = albedoSpecular.a;
    vec3 position = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    float depth = texture(gDepth, TexCoords).r;

    vec3 color = vec3(0);
    //color = albedo * dot(normal, -normalize(position));
    color = albedo;

    if (depth < 1000000000) {
        vec3 fogColor = vec3(0.6, 0.6, 1.0);
        float fogFactor = 1 - (30000 - depth) / (30000);
        //color = mix(color, fogColor, fogFactor);
        color += fogColor * fogFactor;
    }

    //color = position / 3000;
    if (normal == vec3(0, 0, 0)) {
        color = albedo;
    }

    if (false) { // Vignette
        uv *= 1 - uv.yx;
        float vig = uv.x * uv.y * 15;
        vig = pow(vig, 0.04);
        color *= vig;
        //color = vec3(vig);
    }

    //color = vec3(depth / 500);
    //color = albedo;

    FragColor = vec4(color, 0);
}
