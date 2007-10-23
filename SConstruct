# -*- python -*-

import os

version = '0.5'

env = Environment(
    CCFLAGS = [ '-O2', '-Wall' ],
    CPPDEFINES = [ ('VERSION', '\\"%s\\"' % version) ],
    ENV = {'PATH' : os.environ['PATH']}
)

opts = Options('scache.conf')
opts.AddOptions(
    BoolOption('DEBUG', 'debug mode', 0),
    PathOption('PREFIX', 'install prefix', '/usr/local'),
)
opts.Update(env)

if env['DEBUG'] == 1:
    env['CPPDEFINES'] += [ '_DEBUG' ]

env.ParseConfig(
    'pkg-config --cflags --libs jack samplerate sndfile'
)

env.Program('klick', [
    'main.cc',
    'klick.cc',
    'options.cc',
    'audio_interface.cc',
    'audio_chunk.cc',
    'tempomap.cc',
    'metronome.cc',
    'metronome_map.cc',
    'metronome_jack.cc',
    'click_data.cc',
    'util.cc'
])

env['PREFIX_BIN'] = env['PREFIX'] + '/bin'
env.Alias('install', env['PREFIX_BIN'])
env.Install(env['PREFIX_BIN'], ['klick'])

opts.Save('scache.conf', env)
