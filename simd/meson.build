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
    cpp_features += ['-msse4.1']
elif cpp.get_argument_syntax() == 'msvc'
    if cpp.get_id() == 'clang-cl'
        cpp_features += ['/clang:-msse4.1']
    else
        cpp_features += ['/arch:AVX2']
    endif
else
    error('Your compiler, ' + cpp.get_id() + ', does not seem to support SIMD.')
endif

add_project_arguments(
    cpp_features,
    language: 'cpp',
)
