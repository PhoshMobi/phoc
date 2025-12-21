ptk_inc_dir = include_directories('.')

ptk_sources = [
  'ptk.h',
  'ptk-display.c',
  'ptk-surface.c',
  'ptk-subsurface.c',
  'ptk-toplevel.c',
  'ptk-types.c',
]

ptk_deps = [egl, glesv2, glib, wayland_client, wayland_cursor, wayland_egl]

ptk_lib = static_library(
  'ptk',
  [ptk_sources, client_protos_headers, protos_sources],
  include_directories: protos_inc_dir,
  dependencies: ptk_deps,
  install: false,
)

ptk_dep = declare_dependency(
  link_with: ptk_lib,
  sources: client_protos_headers,
  include_directories: [protos_inc_dir, ptk_inc_dir],
  dependencies: [glib],
)
