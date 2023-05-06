project(
    'Expressive Means',
    'c', 'cpp',
    version: '0.1.1',
    license: 'GPL-2.0-or-later',
    default_options: [
        'cpp_std=c++17',
        'b_lundef=true',
        'b_asneeded=true',
        'warning_level=3',
        'buildtype=release',
    ],
    meson_version: '>= 0.53.0'
)

conf = configuration_data()
conf.set('version', meson.project_version())
configure_file(input: 'version.h.in', output: 'version.h', configuration: conf)

system = host_machine.system()
architecture = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')

boost_dep = dependency('boost', modules: [], version: '>= 1.69', required: true, static: true)

boost_unit_test_dep = dependency('boost', modules: ['unit_test_framework'], version: '>= 1.73', required: get_option('tests'), static: true)
have_boost_unit_test = boost_unit_test_dep.found()

plugin_sources = [
  'src/Articulation.cpp',
  'src/CoreFeatures.cpp',
  'src/Glide.cpp',
  'src/Onsets.cpp',
  'src/PitchVibrato.cpp',
  'src/Portamento.cpp',
  'src/SemanticArticulation.cpp',
  'src/SemanticOnsets.cpp',
  'src/SemanticPitchVibrato.cpp',
  'src/SemanticPortamento.cpp',
  'src/libmain.cpp',
]

unit_test_sources = [
  'test/TestArticulation.cpp',
  'test/TestGlide.cpp',
  'test/TestPitchVibrato.cpp',
  'test/tests.cpp',
]

vamp_dir = 'ext/vamp-plugin-sdk'
vamp_sources = [
  vamp_dir / 'src/vamp-sdk/PluginAdapter.cpp',
  vamp_dir / 'src/vamp-sdk/RealTime.cpp',
  vamp_dir / 'src/vamp-sdk/FFT.cpp',
]

qmdsp_dir = 'ext/qm-dsp'
qmdsp_sources = [
  qmdsp_dir / 'maths/MathUtilities.cpp',
#  qmdsp_dir / 'dsp/signalconditioning/FiltFilt.cpp',
]

pyin_dir = 'ext/pyin'
pyin_sources = [
  pyin_dir / 'PYinVamp.cpp',
  pyin_dir / 'YinVamp.cpp',
  pyin_dir / 'LocalCandidatePYIN.cpp',
  pyin_dir / 'Yin.cpp',
  pyin_dir / 'YinUtil.cpp',
  pyin_dir / 'MonoNote.cpp',
  pyin_dir / 'MonoNoteParameters.cpp',
  pyin_dir / 'SparseHMM.cpp',
  pyin_dir / 'MonoNoteHMM.cpp',
  pyin_dir / 'MonoPitchHMM.cpp',
]

feature_defines = []

if system == 'darwin'
  vamp_symbol_args = [
    '-exported_symbols_list', meson.current_source_dir() / 'vamp-plugin.list'
  ]
elif system == 'windows'
  feature_defines += ['-D_WIN32', '-DNOMINMAX', '-D_USE_MATH_DEFINES']
  vamp_symbol_args = [
    '-EXPORT:vampGetPluginDescriptor'
  ]
else
  vamp_symbol_args = [
    '-Wl,--version-script=' + meson.current_source_dir() / 'vamp-plugin.map'
  ]
endif

expressive_means = shared_library(
  'expressive-means',
  plugin_sources,
  vamp_sources,
  qmdsp_sources,
  pyin_sources,
  include_directories: [ vamp_dir ],
  cpp_args: [ feature_defines ],
  link_args: [ vamp_symbol_args ],
  dependencies: [ boost_dep ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / 'vamp'
)

if have_boost_unit_test
  message('Building unit tests: use "meson test -C <builddir>" to run them')
  unit_tests = executable(
    'tests',
    unit_test_sources,
    plugin_sources,
    vamp_sources,
    qmdsp_sources,
    pyin_sources,
    include_directories: [ vamp_dir ],
      cpp_args: [ feature_defines ],
    dependencies: [ boost_unit_test_dep ],
    install: false,
    build_by_default: true
  )
  general_test_args = [ '--log_level=message' ]
  test('Articulation',
       unit_tests, args: [ '--run_test=TestArticulation', general_test_args ])
else
  message('Not building unit tests: boost_unit_test_framework dependency not found')
endif

install_data(
  'expressive-means.cat',
  install_dir: get_option('libdir') / 'vamp',
)
