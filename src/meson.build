project_source_files += [
    files('astro.c'),
    files('bit.c'),
    files('coord.c'),
    files('core.c'),
    files('core_position.c'),
    files('core_render.c'),
    files('drawing.c'),
    files('parse_BSC5.c'),
    files('stopwatch.c'),
    files('term.c'),
    files('city.c'),
    files('split_lines.c'),
]

# NOTE: We add main.c separately in the root Meson.build file to avoid duplicate "main" functions when compiling tests

# Availability of strptime on non-POSIX systems is not guaranteed
has_strptime = cc.has_function('strptime')

if not has_strptime
  message('strptime is not available, using local implementation.')
  lib_strptime = static_library(
    'lib_strptime',
    'strptime.c'
)
else
  message('strptime is available, no need for local implementation.')
  build_defines += '-DHAS_STRPTIME'
  lib_strptime = []
endif
