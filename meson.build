project(
    'libp2p',
    'cpp',
    meson_version: '>= 1.1.0',
    default_options : [
        'cpp_std=c++14',
        'buildtype=debugoptimized',
        'default_library=static',
    ]
)

cpp = meson.get_compiler('cpp')

incs = [include_directories('.')]

libp2p_src = files(
    'p2p_api.cpp',
    'v210.cpp',
)

libp2p_headers = files(
    'p2p_api.h',
    'p2p.h',
)

if get_option('simd').enabled()
    cpu_family = host_machine.cpu_family()
    if not cpu_family.startswith('x86')
        error('SIMD support is only available for x86 CPU families.')
    endif
    subdir('simd')
endif

libp2p = library(
    'p2p',
    libp2p_src,
    install: true,
    include_directories: incs,
)

libp2p_dep = declare_dependency(
    link_with: libp2p,
    include_directories: incs,
)

install_headers(libp2p_headers)

if get_option('test').enabled()
    subdir('test')
endif
