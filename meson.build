project(
  'zmk-via-stack',
  'c',
  version: '0.1.0',
  default_options: ['c_std=c11', 'warning_level=3'],
)

via_native_tests = executable(
  'via-native-tests',
  'config/src/via_translation.c',
  'tests/via_translation_harness.c',
  include_directories: include_directories('config/include'),
  install: false,
)

test(
  'via-native-tests',
  via_native_tests,
  args: [join_paths(meson.project_source_root(), 'config/src/via_zmk.c')],
  timeout: 30,
)
