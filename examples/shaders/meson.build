embed = find_program('./embed.sh', native: true)
shader_headers = []

shaders = []

foreach name : shaders
  custom_target(
    'gles2-' + name,
    input: name,
    output: name + '-check',
    command: [glslang, '@INPUT@'],
    build_by_default: true,
  )

  output = name.replace('.', '-') + '-src.h'
  var = name.underscorify() + '_src'
  shader_headers += custom_target(
    output,
    command: [embed, var],
    input: name,
    output: output,
    feed: true,
    capture: true,
  )
endforeach
