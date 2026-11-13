precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
varying vec3 fragPosition;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform float time;

void main(void) {
    vec4 base = texture2D(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 n = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(-0.35, 0.85, 0.42));
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);
    float diffuse = max(dot(n, lightDir), 0.0);
    float ambient = 0.34 + 0.06 * sin(time * 0.35);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.5);
    float specular = pow(max(dot(n, halfDir), 0.0), 28.0) * 0.16;
    vec3 aurora = vec3(0.20, 0.72, 0.78) * rim * 0.26;
    vec3 lit = base.rgb * (ambient + diffuse * 0.64) + aurora + vec3(specular);
    gl_FragColor = vec4(max(lit, vec3(0.0)), base.a);
}
