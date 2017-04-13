#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 8) out;

#define FLAG_FLASHING uint(1)
#define FLAG_ITALIC uint(2)
#define FLAG_BOLD uint(4)

#define FLASH_PERIOD float(0.3)

in vec3 gColor[];
in vec3 gBgColor[];
in vec4 gGlyphRect[];
in vec4 gTextureRect[];
in vec4 gBgRect[];
in uint gFlags[];

uniform sampler2D sampler;
uniform mat4 proj;
uniform float timer;

out vec3 fColor;
out vec2 fTextureCoord;

void emitBGVertex(vec2 position, vec3 color) {
  gl_Position = vec4(position, 0, 1);
  fColor = color;
  fTextureCoord = vec2(-1.0, -1.0);
  EmitVertex();
}

void emitGlyphVertex(vec2 position, vec3 color, vec2 texCoord) {
  gl_Position = vec4(position, 0, 1);
  fColor = color;
  fTextureCoord = texCoord;
  EmitVertex();
}

void main() {
  // Flash every 0.3 seconds.
  float periods = timer / FLASH_PERIOD;
  bool flashPeriod = mod(periods, 2) > 1;
  bool flashingCharacter = (gFlags[0] & FLAG_FLASHING) == FLAG_FLASHING;
  bool drawGlyph = !flashingCharacter || (flashPeriod && flashingCharacter);

  // Create then background color rectangle first.
  emitBGVertex(gBgRect[0].xy, gBgColor[0]);
  emitBGVertex(gBgRect[0].zy, gBgColor[0]);
  emitBGVertex(gBgRect[0].xw, gBgColor[0]);
  emitBGVertex(gBgRect[0].zw, gBgColor[0]);
  EndPrimitive();

  // Create the rect for the actual glyph.
  if (drawGlyph) {
    emitGlyphVertex(gGlyphRect[0].xy, gColor[0], gTextureRect[0].xy);
    emitGlyphVertex(gGlyphRect[0].zy, gColor[0], gTextureRect[0].zy);
    emitGlyphVertex(gGlyphRect[0].xw, gColor[0], gTextureRect[0].xw);
    emitGlyphVertex(gGlyphRect[0].zw, gColor[0], gTextureRect[0].zw);
    EndPrimitive();
  }
}
