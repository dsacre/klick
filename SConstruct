# -*- python -*-

import os

version = '0.7.0'

env = Environment(
    CPPDEFINES = [('VERSION', '\\"%s\\"' % version)],
    CPPPATH = ['.'],
    ENV = os.environ,
)

# build options
opts = Options('scache.conf')
opts.AddOptions(
    BoolOption('DEBUG', 'debug mode', False),
    PathOption('PREFIX', 'install prefix', '/usr/local'),
)
opts.Update(env)
opts.Save('scache.conf', env)
Help(opts.GenerateHelpText(env))

if env['DEBUG']:
    env.Append(CCFLAGS = ['-g', '-W', '-Wall'])
    env.Append(CCFLAGS = '-Werror')
else:
    env.Append(CCFLAGS = ['-O2', '-W', '-Wall'])
    env.Prepend(CPPDEFINES = 'NDEBUG')

# install paths
env['PREFIX_BIN'] = os.path.join(env['PREFIX'], 'bin')
env['PREFIX_SHARE'] = os.path.join(env['PREFIX'], 'share/klick')

env.Append(CPPDEFINES = ('DATA_DIR', '\\"%s\\"' % env['PREFIX_SHARE']))

# required libraries
env.ParseConfig(
    'pkg-config --cflags --libs jack samplerate sndfile'
)

# source files
sources = [
    'src/main.cc',
    'src/klick.cc',
    'src/options.cc',
    'src/audio_interface.cc',
    'src/audio_chunk.cc',
    'src/tempomap.cc',
    'src/metronome.cc',
    'src/metronome_map.cc',
    'src/metronome_jack.cc',
    'src/position.cc',
    'util/util.cc'
]

env.Program('klick', sources)

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

# installation
env.Alias('install', [env['PREFIX_BIN'], env['PREFIX_SHARE']])
env.Install(env['PREFIX_BIN'], 'klick')
env.Install(os.path.join(env['PREFIX_SHARE'], 'samples'), samples)
