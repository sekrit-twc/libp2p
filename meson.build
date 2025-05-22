project(
    'libp2p',
    'cpp',
    default_options: [
        'cpp_std=c++14',
        'buildtype=debugoptimized',
    ],
    license: 'WTFPL',
    license_files: 'COPYING',
    meson_version: '>= 1.1.0',
)

cpp = meson.get_compiler('cpp')

incs = [include_directories('.')]

install_headers(
    files(
        'p2p_api.h',
        'p2p.h',
    )
)

libp2p_src = files(
    'p2p_api.cpp',
    'v210.cpp',
)

cpp_public_defs = []
cpp_compile_flags = []

subdir('simd')

libp2p = static_library(
    'p2p',
    libp2p_src,
    cpp_args: cpp_public_defs + cpp_compile_flags,
    include_directories: incs,
    install: true,
)

libp2p_dep = declare_dependency(
    link_with: libp2p,
    include_directories: incs,
    compile_args: cpp_public_defs
)

meson.override_dependency('libp2p', libp2p_dep)

libp2p_pkg = import('pkgconfig')
libp2p_pkg.generate(
    name: meson.project_name(),
    description: 'Pack/unpack pixels',
    libraries: libp2p_dep,
)

if get_option('test').enabled()
    subdir('test')
endif
