#version 310 es

/* Rectangle position */
layout(location=1) uniform vec2 u_pos;
/* Rectangle size */
layout(location=2) uniform vec2 u_size;
/* Window size */
layout(location=3) uniform vec2 u_win;
layout(location=0) in vec4 v_pos;

void main()
{
  vec2 size = 2.0 * u_size / u_win;
  vec2 pos = vec2 (-1.0 + 2.0 * (u_pos.x / u_win.x),
                    1.0 - 2.0 * (u_pos.y / u_win.y));

  mat4 scale = mat4(size.x, 0.0,    0.0, 0.0,
                    0.0,    size.y, 0.0, 0.0,
                    0.0,    0.0,    1.0, 0.0,
                    0.0,    0.0,    0.0, 1.0);

  mat4 translate = mat4(1.0,   0.0,   0.0, 0.0,
                        0.0,   1.0,   0.0, 0.0,
                        0.0,   0.0,   1.0, 0.0,
                        pos.x, pos.y, 0.0, 1.0);

  gl_Position = translate * scale * v_pos;
}
