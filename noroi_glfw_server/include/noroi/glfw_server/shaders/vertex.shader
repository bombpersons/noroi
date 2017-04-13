#version 330 core

in vec3 vColor;
in vec3 vBgColor;
in vec4 vGlyphRect;
in vec4 vTextureRect;
in vec4 vBgRect;
in uint vFlags;

out vec3 gColor;
out vec3 gBgColor;
out vec4 gGlyphRect;
out vec4 gTextureRect;
out vec4 gBgRect;
out uint gFlags;

uniform sampler2D sampler;
uniform mat4 proj;

void main() {
  // Pass colors through
  gColor = vColor;
  gBgColor = vBgColor;

  // Pass flags through too.
  gFlags = vFlags;

  // Pass texture rect straight through.
  gTextureRect = vTextureRect;

  // Transform input rectangles by our projection matrix.
  gGlyphRect.xy = (proj * vec4(vGlyphRect.x, vGlyphRect.y, 0.0, 1.0)).xy;
  gGlyphRect.zw = (proj * vec4(vGlyphRect.z, vGlyphRect.w, 0.0, 1.0)).xy;
  gBgRect.xy = (proj * vec4(vBgRect.x, vBgRect.y, 0.0, 1.0)).xy;
  gBgRect.zw = (proj * vec4(vBgRect.z, vBgRect.w, 0.0, 1.0)).xy;
}
