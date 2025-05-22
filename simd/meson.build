incs += include_directories('.')

install_headers(
    files(
        'cpuinfo_x86.h',
        'p2p_simd.h',
    ), 
    subdir: 'simd'
)

libp2p_src += files(
    'cpuinfo_x86.cpp',
    'p2p_simd.cpp',
    'p2p_sse41.cpp',
)

if get_option('simd').enabled()
    cpu_family = host_machine.cpu_family()
    if not cpu_family.startswith('x86')
        error('SIMD support is only available for x86 CPU families.')
    endif

    cpp_public_defs += ['-DP2P_SIMD']

    if cpp.get_argument_syntax() == 'gcc'
        cpp_compile_flags += ['-msse4.1']
    elif cpp.get_argument_syntax() == 'msvc'
        if cpp.get_id() == 'clang-cl'
            cpp_compile_flags += ['/clang:-msse4.1']
        else
            cpp_compile_flags += ['/arch:AVX2']
        endif
    else
        error('Your compiler, ' + cpp.get_id() + ', does not seem to support SIMD.')
    endif
endif
