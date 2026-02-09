#version 310 es
precision mediump float;

layout(location=0) uniform vec4 u_color;
out vec4 frag_color;

void main()
{
  frag_color = u_color;
}
