#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform float time;

out vec4 finalColor;

void main(void) {
    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 n = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(-0.35, 0.85, 0.42));
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);

    float diffuse = max(dot(n, lightDir), 0.0);
    float ambient = 0.30 + 0.08 * sin(time * 0.35);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.8);
    float specular = pow(max(dot(n, halfDir), 0.0), 36.0) * 0.22;
    vec3 aurora = vec3(0.20, 0.72, 0.78) * rim * 0.30;
    vec3 lit = base.rgb * (ambient + diffuse * 0.70) + aurora + vec3(specular);
    finalColor = vec4(max(lit, vec3(0.0)), base.a);
}
