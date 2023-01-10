project(
    'Expressive Means',
    'c', 'cpp',
    version: '0.1',
    license: 'GPL-2.0-or-later',
    default_options: [
        'cpp_std=c++17',
        'b_lundef=true',
        'b_asneeded=true',
        'warning_level=3',
        'buildtype=release',
    ],
    meson_version: '>= 0.60.0'
)

conf = configuration_data()
conf.set('version', meson.project_version())
configure_file(input: 'version.h.in', output: 'version.h', configuration: conf)

system = host_machine.system()
architecture = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')

plugin_sources = [
  'articulation/Articulation.cpp',
  'libmain.cpp',
]

vamp_dir = 'ext/vamp-plugin-sdk'
vamp_sources = [
  vamp_dir / 'src/vamp-sdk/PluginAdapter.cpp',
  vamp_dir / 'src/vamp-sdk/RealTime.cpp',
  vamp_dir / 'src/vamp-sdk/FFT.cpp',
]

#qmdsp_dir = 'ext/qm-dsp'
#qmdsp_sources = [
#  qmdsp_dir / 'dsp/signalconditioning/Filter.cpp',
#  qmdsp_dir / 'dsp/signalconditioning/FiltFilt.cpp',
#]

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

if system == 'darwin'
  vamp_symbol_args = [
    '-exported_symbols_list', meson.current_source_dir() / 'vamp-plugin.list'
  ]
elif system == 'windows'
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
#  qmdsp_sources,
  pyin_sources,
  include_directories: [ vamp_dir ],
  link_args: [ vamp_symbol_args ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / 'vamp'
)

install_data(
  'expressive-means.cat',
  install_dir: get_option('libdir') / 'vamp',
)
