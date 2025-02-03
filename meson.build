project('astroterm', 'c',
    version: '1.0.7',
    default_options : ['warning_level=3', 'buildtype=debug'],
    meson_version: '>=1.4.0',
)

is_windows = host_machine.system() == 'windows'

# ------------------------------------------------------------------------------
# Setup Paths
# ------------------------------------------------------------------------------

conf = configuration_data()
fs = import('fs')

project_source_files = []
project_include_dirs = [
    include_directories('include'),
    include_directories('data'),
]

# This is where we install local dependencies (primarily for Windows)
deps_include = 'deps/include'
deps_lib =  'deps/lib'
if fs.exists(deps_include)
    project_include_dirs += include_directories(deps_include)
endif
deps_lib_path = join_paths(meson.current_source_dir(), deps_lib)
deps_include_path = join_paths(meson.current_source_dir(), deps_include)

# Add Homebrew include path
# Need this for argtable3 which:
# - Does not provide a .pc file like argtable2
# - Is sometimes found via cmake, but the argtable3Config.cmake file does not expose include directories
# Reference: https://apple.stackexchange.com/questions/40704/homebrew-installed-libraries-how-do-i-use-them
# Thus, we do this ugliness to make sure we include all Homebrew libs
brew = find_program('brew', required: false)
if brew.found()
    # Get the Homebrew prefix
    brew_prefix = run_command(brew, '--prefix', check: true).stdout().strip()
    message('Homebrew prefix: ' + brew_prefix)

    homebrew_include_path = brew_prefix + '/include'
    if fs.exists(homebrew_include_path)
        project_include_dirs += include_directories(homebrew_include_path)
        message('Adding Homebrew to include paths: ' + homebrew_include_path)
    endif
endif

# ------------------------------------------------------------------------------
# Setup Compiler
# ------------------------------------------------------------------------------

# Setup compiler specific flags
cc = meson.get_compiler('c')
if cc.get_id() == 'msvc'
    # Needed for unicode on Windows
    add_project_arguments('/utf-8', language: 'c')
else
    # Needed to make GCC happy with `strptime` on certain systems
    add_project_arguments('-D_XOPEN_SOURCE', '-D_USE_XOPEN', '-D_GNU_SOURCE', language : 'c')
endif

build_defines = []

# ------------------------------------------------------------------------------
# Dependency: Argtable2/3
# ------------------------------------------------------------------------------

# astroterm's code is compatible both with the argtable2 and 3 APIs. Since
# sometimes only one or the other is available, we do a bit more work to
# support both.
have_argtable3 = false
have_argtable2 = false

if is_windows

    # Search local deps
    if cc.has_header('argtable3.h', args: ['-I' + deps_include_path])
        have_argtable3 = true
    elif cc.has_header('argtable2.h', args: ['-I' + deps_include_path])
        have_argtable2 = true
    endif

    link_arg = have_argtable3 ? '-largtable3' : '-largtable2'
    argtable = declare_dependency(
        include_directories: deps_include,
        link_args: ['-L' + deps_lib_path, link_arg]
    )

else

    # When building on GitHub Actions or locally, the first dependency() call
    # works as expected. However, when building on Homebrew, it fails to locate
    # the library. The second fallback method find_library() is required in that
    # case. Until the root cause is identified, both methods are included to
    # ensure compatibility.
    argtable = dependency('argtable3', required: false)
    if not argtable.found()
        argtable = cc.find_library('argtable3', required: false)
    endif

    if argtable.found()
        have_argtable3 = true
    else
        argtable = dependency('argtable2', required: true)
        have_argtable2 = true
    endif

endif

# Set appropriate include flag
if have_argtable3
    add_project_arguments('-DHAVE_ARGTABLE3', language: 'c')
    message('Using argtable3')
elif have_argtable2
    add_project_arguments('-DHAVE_ARGTABLE2', language: 'c')
    message('Using argtable2')
else
    error('Neither argtable3 nor argtable2 found. Please check your include paths.')
endif

# ------------------------------------------------------------------------------
# Dependency: math lib
# ------------------------------------------------------------------------------

# Skip math library check on Windows
if is_windows
    math = []
else
    math = cc.find_library('m', required : true)
endif

# ------------------------------------------------------------------------------
# Dependency: Curses
# ------------------------------------------------------------------------------

if is_windows
    # Detect PDcurses
    curses = declare_dependency(
        include_directories: deps_include,
        link_args: ['-L' + deps_lib_path, '-lpdcurses']
    )
else
    curses = dependency('curses', required : true)
endif

# Define some curses preprocessor symbols
if curses.found()
    check_headers = [
        ['ncursesw/menu.h',     'HAVE_NCURSESW_MENU_H'],
        ['ncurses/menu.h',      'HAVE_NCURSES_MENU_H'],
        ['menu.h',              'HAVE_MENU_H'],
        ['ncursesw/curses.h',   'HAVE_NCURSESW_CURSES_H'],
        ['ncursesw.h',          'HAVE_NCURSESW_H'],
        ['ncurses/curses.h',    'HAVE_NCURSES_CURSES_H'],
        ['ncurses.h',           'HAVE_NCURSES_H'],
        ['curses.h',            'HAVE_CURSES_H'],
    ]
    foreach h : check_headers
        if cc.has_header(h.get(0))
            conf.set(h.get(1), 1)
        endif
    endforeach
endif

# Check what curses headers are available
if (conf.get('HAVE_NCURSESW_H', 0) == 1)
    # Have ncurses with wide char support
else
    if (conf.get('HAVE_NCURSES_H', 0) == 1)
        # Have ncurses without wide char support*
        # *on certain systems, the ncurses header includes wide char functions
        warning('ncursesw header not found, which is sometimes needed for ' +
                'unicode support. This may not be an issue as the regular ' +
                'ncurses.h sometimes contains wide character support')
    else
        if (conf.get('HAVE_CURSES_H', 0) == 1)
            # Have curses but not ncurses
            warning('ncurses header not found. Ncurses is required for ' +
                    'certain features such as color support')
        endif
    endif
endif

# ------------------------------------------------------------------------------
# Build submodules
# ------------------------------------------------------------------------------

subdir('src')
subdir('data')

# ------------------------------------------------------------------------------
# Embed data
# ------------------------------------------------------------------------------

if is_windows
    # On Windows, use python script
    embed_command = [
        'cmd', '/c', 'python', '..\\scripts\\embed.py',
        '--source', '@INPUT@',
        '--header', '@OUTPUT@',
    ]
else
    # On Unix-like systems, use the shell script
    embed_command = [
        'sh', '../scripts/embed.sh',
        '--source', '@INPUT0@',
        '--header', '@OUTPUT@',
    ]
endif

bsc5 = custom_target(
    # Use bsc5 path from data/meson.build
    input: ['data/' + bsc5_path],
    output: 'bsc5.h',
    command: [embed_command, '--array-name', 'bsc5']
)
bsc5_constellations = custom_target(
    input: ['data/bsc5_constellations.txt'],
    output: 'bsc5_constellations.h',
    command: [embed_command, '--array-name', 'bsc5_constellations']
)
bsc5_names = custom_target(
    input: ['data/bsc5_names.txt'],
    output: 'bsc5_names.h',
    command: [embed_command, '--array-name', 'bsc5_names']
)
cities = custom_target(
    input: ['data/cities.csv'],
    output: 'cities.h',
    command: [embed_command, '--array-name', 'cities']
)
embedded_files=  [bsc5, bsc5_constellations, bsc5_names, cities]

# ------------------------------------------------------------------------------
# Application library (for reusability)
# ------------------------------------------------------------------------------

lib_project = static_library(
    'lib_astroterm',
    project_source_files + embedded_files,
    link_with           : lib_strptime,
    dependencies        : [curses, math],
    include_directories : project_include_dirs,
)

# ------------------------------------------------------------------------------
# Application executable
# ------------------------------------------------------------------------------

# Get the version from the project
program_version = meson.project_version()
program_name = meson.project_name()

# Add preprocessor definitions
build_defines += [
  f'-DPROJ_NAME="@program_name@"',
  f'-DPROJ_VERSION="@program_version@"'
]

executable(
    meson.project_name(),
    ['src/main.c'] + embedded_files,
    link_with           : lib_project,
    dependencies        : argtable,
    include_directories : project_include_dirs,
    c_args              : build_defines,
    install             : true,
)

# ------------------------------------------------------------------------------
# Testing
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Unity Static Library
# ------------------------------------------------------------------------------

test_files = []
test_include_dirs = []
unity_source_files = []
subdir('test')

unity_lib = static_library(
    'unity',
    unity_source_files,
    include_directories: test_include_dirs,
)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

# Compile test modules separately
foreach test_file : test_files
    filepath = test_file[0].full_path()
    test_name = fs.stem(filepath)
    test_exe = executable(
        test_name,
        test_file + unity_source_files + embedded_files,
        link_with: [lib_project, unity_lib],
        include_directories: project_include_dirs + test_include_dirs,
        c_args: '-DUNITY_INCLUDE_CONFIG_H', # Needed to test doubles
        install: false
    )

    test(test_name, test_exe)
endforeach
