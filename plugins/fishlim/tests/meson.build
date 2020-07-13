subdir('old_version')

fishlim_test_sources = [
  'tests.c',
  'fake/keystore.c',
  '../fish.c',
  '../utils.c',
]

fishlim_tests = executable('fishlim_tests', fishlim_test_sources,
  dependencies: [libgio_dep, libssl_dep],
  link_with : fishlim_old_lib
)

test('Fishlim Tests', fishlim_tests,
  timeout: 90
)
