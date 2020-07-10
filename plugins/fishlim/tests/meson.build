test_sources = [
  'tests.c',
  'fake/keystore.c',
  '../fish.c',
  '../utils.c',
]

e = executable('fishlim_tests', test_sources,
  dependencies: [libgio_dep, libssl_dep],
)

test('Fishlim Tests', e)
