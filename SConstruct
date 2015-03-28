import os
import SCons.Util

version = '0.13.0-pre'

env = Environment(
    CPPDEFINES = [
        ('VERSION', '\\"%s\\"' % version),
    ],
    CXXFLAGS = ['-std=c++11'],
    ENV = os.environ,
)

if os.environ.has_key('CXX'):
    env.Replace(CXX = os.environ['CXX'])
if os.environ.has_key('CXXFLAGS'):
    env.Append(CXXFLAGS = SCons.Util.CLVar(os.environ['CXXFLAGS']))
if os.environ.has_key('CPPFLAGS'):
    env.Append(CXXFLAGS = SCons.Util.CLVar(os.environ['CPPFLAGS']))
if os.environ.has_key('LDFLAGS'):
    env.Append(LINKFLAGS = SCons.Util.CLVar(os.environ['LDFLAGS']))

# build options
opts = Variables('scache.conf')
opts.AddVariables(
    PathVariable('PREFIX', 'install prefix', '/usr/local'),
    PathVariable('DESTDIR', 'intermediate install prefix', '', PathVariable.PathAccept),
    BoolVariable('DEBUG', 'debug mode', False),
    BoolVariable('OSC', 'OSC support', True),
    BoolVariable('TERMINAL', 'terminal control support', True),
    BoolVariable('RUBBERBAND', 'use Rubber Band for pitch shifting', True),
    BoolVariable('GETOPT_LONG', 'enable long options using getopt_long()', True),
)
opts.Update(env)
opts.Save('scache.conf', env)
Help(opts.GenerateHelpText(env))

if env['DEBUG']:
    env.Append(CCFLAGS = ['-g', '-Wall', '-Wextra'])
else:
    env.Append(CCFLAGS = ['-O2', '-Wall', '-Wextra'])
    env.Prepend(CPPDEFINES = 'NDEBUG')

# install paths
prefix_bin = os.path.join(env['PREFIX'], 'bin')
prefix_share = os.path.join(env['PREFIX'], 'share/klick')

env.Append(CPPDEFINES = ('DATA_DIR', '\\"%s\\"' % prefix_share))

# required libraries
env.ParseConfig(
    'pkg-config --cflags --libs jack samplerate sndfile'
)
if os.system('pkg-config --atleast-version=1.0.18 sndfile') == 0:
    env.Append(CPPDEFINES = ['HAVE_SNDFILE_OGG'])

# source files
sources = [
    'src/main.cc',
    'src/klick.cc',
    'src/options.cc',
    'src/audio_interface.cc',
    'src/audio_interface_jack.cc',
    'src/audio_interface_sndfile.cc',
    'src/audio_chunk.cc',
    'src/tempomap.cc',
    'src/metronome.cc',
    'src/metronome_simple.cc',
    'src/metronome_map.cc',
    'src/metronome_jack.cc',
    'src/position.cc',
]

# audio samples
samples = [
    'samples/square_emphasis.wav',
    'samples/square_normal.wav',
    'samples/sine_emphasis.wav',
    'samples/sine_normal.wav',
    'samples/noise_emphasis.wav',
    'samples/noise_normal.wav',
    'samples/click_emphasis.wav',
    'samples/click_normal.wav',
]

# build options
if env['OSC']:
    env.ParseConfig('pkg-config --cflags --libs liblo')
    env.Append(CPPDEFINES = ['ENABLE_OSC'])
    sources += [
        'src/osc_interface.cc',
        'src/osc_handler.cc',
    ]

if env['TERMINAL']:
    env.Append(CPPDEFINES = ['ENABLE_TERMINAL'])
    sources += [
        'src/terminal_handler.cc',
    ]

if env['RUBBERBAND']:
    env.ParseConfig('pkg-config --cflags --libs rubberband')
    env.Append(CPPDEFINES = ['ENABLE_RUBBERBAND'])

if env['GETOPT_LONG']:
    env.Append(CPPDEFINES = ['ENABLE_GETOPT_LONG'])

env.Program('klick', sources)
Default('klick')

# installation
env.Alias('install', [
    env.Install(env['DESTDIR'] + prefix_bin, 'klick'),
    env.Install(env['DESTDIR'] + os.path.join(prefix_share, 'samples'), samples)
])
