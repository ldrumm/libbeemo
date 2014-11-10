import sys
import os
import multiprocessing

from SCons.Errors import BuildError
from SCons.SConf import SConfWarning


def _print(*args):
    if GetOption('silent'):
        return
    print ' '.join((str(arg) for arg in args))

class ArgOpts(object):
    def __init__(self, env, args):
        self.args = args
        self.env = env

    def enable_release(self, env):
        self.env.Append(CCFLAGS=['-O2', '-mtune=native', '-ffast-math'])
        self.env.Append(CPPDEFINES=['NDEBUG'])

    def enable_profile(env):
        self.env.Append(CCFLAGS=['-O2', '-ffast-math', '-g'])

    def enable_analyse(env):
        """
        This fetches environment variables set by clang's scan-build
        to allow clang to run the static analyser
        """
        self.env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

    def configure(self):
        for arg in self.args:
            try:
                option = getattr(self, 'enable_' + arg)
                if self.args[arg] in ('1', 'true', 'yes'):
                    option()
            except AttributeError:
                _print("ignoring unknown configure option:'%s" % arg)


def std_switches(env):
    env.Append(CCFLAGS=['-Wall', '-Wextra', '-Werror', '-g3',])
    env["CC"] = os.getenv("CC") or env["CC"]


def mingw(env):
    env.Append(tools=['mingw'])
    
    # MinGW doesn't allow including unistd.h in c99 mode:
    # http://sourceforge.net/p/mingw/bugs/2046/
    try:
        env['CCFLAGS'].remove('-std=c99',)
    except ValueError:
        pass
    env.Append(CCFLAGS = ['-std=gnu99'])


def check_dependencies(env, conf=None):
    conf = conf or Configure(env)
    config = {}
    config['endian'] = sys.byteorder
    config['have_jack'] = conf.CheckLibWithHeader(
        'jack', ['jack/jack.h', 'jack/types.h'], 'C'
    )
    config['have_pthread'] = conf.CheckLibWithHeader(
        'pthread', 'pthread.h', 'C'
    )
    try:
         env.ParseConfig("pkg-config lua5.2 --cflags --libs")
         config['have_lua'] = conf.CheckLibWithHeader(
        'lua5.2', ['lua.h','lauxlib.h'], 'C'
    )
    except OSError:
        _print("***System doesn't have a working pkg-config for Lua5.2 available - fallback config")
        config['have_lua'] = conf.CheckLibWithHeader(
            'lua', ['lua.h','lauxlib.h'], 'C'
        )

    config['have_sndfile'] = conf.CheckLibWithHeader(
        'sndfile', 'sndfile.h', 'C'
    )
    config['have_portaudio'] = conf.CheckLibWithHeader(
        'portaudio', 'portaudio.h', 'C'
    )
    config['have_ladspa'] = conf.CheckHeader('ladspa.h')
    if not config['have_lua']:
        raise BuildError(errstr='Essential dependencies not found; cannot build without Lua')
    return config


def configure_cpp_switches(env, config):
    """
    receives a list of configure options, then mangles the key and value to
    create C preprocessor defines.

    If the value of the key/value pair is a string, then that string is appended
    to the key e.g. a big-endian system defines "-DBMO_ENDIAN_BIG when using
    gcc and /DBMO_ENDIAN_BIG for cl.exe
    """
    defines = []
    for key in config:
        if(type(config[key]) != bool):
            defines.extend(["BMO_" + key.upper()+ "_" + str(config[key]).upper()])
        elif((type(config[key]) == bool) and config[key] == True):
            defines.extend(["BMO_"+ key.upper() ])
    env.Append(CPPDEFINES=defines)



############ Environment / Config ############
env = Environment(ENV=os.environ)
std_switches(env)
if os.name == 'nt':
    # MSVC is the SCons default on windows but MSVC with anything approaching c99 won't work.
    mingw(env)


############ Sources #############
env.Append(source=[
    Glob('src/*.c'), 
    Glob('src/drivers/*.c'),
    Glob('src/dsp/*.c'),
    Glob('src/lua/*.c'),
    Glob('src/memory/*.c'),
])


############ Targets #############
beemo = env.SharedLibrary('beemo', env['source'])

if not env.GetOption('clean'):
    # Only parse these options during a real build because cleans take too long otherwise
    opts = ArgOpts(env, COMMAND_LINE_TARGETS)
    opts.configure()
    conf = Configure(env)
    deps = check_dependencies(env, conf)
    configure_cpp_switches(env, deps)
    env = conf.Finish()

# We need to autogenerate some files for the lua interface
lualibs = env.SConscript('src/lua/SConscript', ['env', '_print'])
# Maybe run some tests
tests = env.SConscript('tests/SConscript', 'env')
# Possibly build a native installer
pkg = env.SConscript("pkg/SConscript", ['env', '_print'])

env.Depends(tests, beemo)
Default(beemo)