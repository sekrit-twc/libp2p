test_src = [
  'api_formats_test.cpp',
  'api_test.cpp',
  'v210_test.cpp',
]

if get_option('simd').enabled()
    test_src += ['simd_test.cpp']
endif

gtest_main_dep = dependency('gtest_main')

test_prog = executable(
    'testprog',
    test_src,
    dependencies : [gtest_main_dep, libp2p_dep],
)    
test('gtest test', test_prog)
