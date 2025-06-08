incs += include_directories('.')

libp2p_src += files(
    'cpuinfo_x86.cpp',
    'p2p_simd.cpp',
    'p2p_sse41.cpp',
)

libp2p_headers += files(
    'cpuinfo_x86.h',
    'p2p_simd.h',
)

cpp_features = ['-DP2P_SIMD']

if cpp.get_argument_syntax() == 'gcc'
    cpp_features += [
        '-msse4.1',
    ]
elif cpp.get_argument_syntax() == 'msvc'
    cpp_features += [
        '/arch:AVX2'
    ]
endif

add_project_arguments(
    cpp_features,
    language: 'cpp',
)
