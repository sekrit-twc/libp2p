test_src = [
  'api_formats_test.cpp',
  'api_test.cpp',
  'v210_test.cpp',
]

if get_option('simd').enabled()
    test_src += ['simd_test.cpp']
endif

gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')
gtest_main_dep = gtest_proj.get_variable('gtest_main_dep')

test_prog = executable(
    'testprog',
    test_src,
    include_directories : incs,
    dependencies : [gtest_dep, gtest_main_dep],
    link_with : libp2p
)    
test('gtest test', test_prog)
