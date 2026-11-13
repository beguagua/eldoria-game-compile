#version 100
precision mediump float;

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
varying vec3 fragPosition;

void main(void) {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize((matNormal * vec4(vertexNormal, 0.0)).xyz);
    fragPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
