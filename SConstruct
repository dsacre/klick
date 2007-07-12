# -*- python -*-

version = '0.4'

env = Environment(
    CCFLAGS = [ '-O2', '-Wall' ],
    CPPDEFINES = 'VERSION=\\"%s\\"' % version
)

env.ParseConfig(
    'pkg-config --cflags --libs jack samplerate sndfile'
)

env.Program('klick', [
    'main.cc',
    'klick.cc',
    'audio.cc',
    'tempomap.cc',
    'metronome.cc',
    'click_data.cc',
    'util.cc'
])

env['PREFIX'] = ARGUMENTS.get('PREFIX', '/usr/local')
env['PREFIX_BIN'] = env['PREFIX'] + '/bin'

env.Alias('install', env['PREFIX_BIN'])

env.Install(env['PREFIX_BIN'], ['klick'])
