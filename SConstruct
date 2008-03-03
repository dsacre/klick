# -*- python -*-

import os

version = '0.6.2'

env = Environment(
    CCFLAGS = [ '-O2', '-Wall' ],
    CPPDEFINES = [ ('VERSION', '\\"%s\\"' % version) ],
    CPPPATH = [ '.' ],
    ENV = os.environ,
)

opts = Options('custom.py')
opts.AddOptions(
    BoolOption('DEBUG', 'debug mode', 0),
    PathOption('PREFIX', 'install prefix', '/usr/local'),
)
opts.Update(env)

env['PREFIX_BIN'] = env['PREFIX'] + '/bin'
env['PREFIX_SHARE'] = env['PREFIX'] + '/share/klick'

env['CPPDEFINES'] += [ ('DATA_DIR', '\\"%s\\"' % env['PREFIX_SHARE']) ]

if env['DEBUG'] == 1:
    env['CPPDEFINES'] += [ '_DEBUG' ]
    env['CCFLAGS'] = [ '-g', '-Wall' ]
    env['CCFLAGS'] += [ '-Werror' ]

env.ParseConfig(
    'pkg-config --cflags --libs jack samplerate sndfile'
)

env.Program('src/klick', [
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
])

env.Alias('install', env['PREFIX_BIN'])
env.Install(env['PREFIX_BIN'], ['klick'])
