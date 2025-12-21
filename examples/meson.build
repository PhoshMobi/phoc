wayland_egl = dependency('wayland-egl')
wayland_cursor = dependency('wayland-cursor')
egl = dependency('egl', required: false, disabler: true)
glesv2 = dependency('glesv2', required: false, disabler: true)
glslang = find_program('glslang', 'glslangValidator', native: true, required: false, disabler: true)

subdir('ptk')
subdir('shaders')

executable(
  'layer-shell-effects',
  sources: ['layer-shell-effects.c', 'egl-common.c', client_protos_headers, protos_sources],
  dependencies: [egl, glesv2, glib, wayland_client, wayland_cursor, wayland_egl, wlroots],
)

executable(
  'ptk-demo',
  sources: [
    'ptk-demo.c',
    'ptk-demo-subsurface.c',
    'ptk-demo-cutout.c',
    'egl-common.c',
    shader_headers,
  ],
  dependencies: ptk_dep,
)

executable(
  'phosh-private',
  sources: ['phosh-private.c', client_protos_headers, protos_sources],
  dependencies: [glib, wayland_client],
)

executable(
  'device-state',
  sources: ['device-state.c', client_protos_headers, protos_sources],
  dependencies: [glib, wayland_client],
)

executable(
  'idle',
  sources: ['idle.c', client_protos_headers, protos_sources],
  dependencies: [glib, wayland_client],
)
