#version 330 core

in vec3 fColor;
in vec2 fTextureCoord;

uniform sampler2D sampler;
uniform mat4 proj;

out vec4 oColor;

void main() {
  // Get the texture color.
  oColor = vec4(fColor.x, fColor.y, fColor.z, 1.0);
  if (fTextureCoord.x >= 0.0) {
    oColor.w = texture(sampler, fTextureCoord).r;
  }
}
