from __future__ import print_function

import sys
import os
import multiprocessing

LIB_VERSION = (0, 0, 1)

from SCons.Errors import BuildError

def _print(*args):
    if GetOption('silent'):
        return
    print(*args)


class ArgOpts(object):
    def __init__(self, env, enable, disable):
        self.env = env
        self.enable = set([] if not enable else enable.split(','))
        self.disable = set([] if not disable else disable.split(','))
        intersection = self.enable.intersection(self.disable)
        if intersection:
            raise BuildError(
                errstr="Mutually exclusive options: "
                "enable/disable %s" % ','.join(intersection)
            )

    def enable_release(self):
        self.env.Append(CCFLAGS=['-O2', '-mtune=native', '-ffast-math'])
        self.env.Append(CPPDEFINES=['NDEBUG'])
        for debug in ('-g', '-ggdb'):
            for x in ('', '2', '3'):
                try:
                    self.env['CCFLAGS'].remove("%s%s" % (debug, x))
                except ValueError:
                    pass

    def enable_profile(self):
        self.env.Append(CCFLAGS=['-O2', '-ffast-math', '-g'])

    def enable_gprof(self):
        self.env.Append(CCFLAGS=['-pg'])
        self.env.Append(LDFLAGS=['-pg'])
        self.env.Append(LINKFLAGS=['-pg'])

    def enable_lto(self):
        self.env.Append(CCFLAGS=['-flto'])
        self.env.Append(LINKFLAGS=['-flto'])

    def enable_asan(self):
        _print("enabling adress sanitizer")
        asan_symbolizer = os.environ.get('ASAN_SYMBOLIZER_PATH')
        if asan_symbolizer:
            self.env["ENV"].update(ASAN_SYMBOLIZER_PATH=asan_symbolizer)

        self.env.Append(LDFLAGS=['-fsanitize=address'])
        self.env.Append(CCFLAGS=['-fsanitize=address'])
        self.env.Append(LIBS=['asan'])

    def enable_ubsan(self):
        self.env.Append(CCFLAGS=['-fsanitize=undefined'])
        self.env.Append(LDFLAGS=['-fsanitize=undefined'])
        self.env.Append(LIBS=['ubsan'])

    def enable_analyse(self):
        """
        This fetches environment variables set by clang's scan-build
        to allow clang to run the static analyser
        """
        self.env["ENV"].update(
                (x, y)
                for x, y in os.environ.items() if x.startswith("CCC_")
        )

    def configure(self):
        for arg in self.enable:
            try:
                getattr(self, 'enable_' + arg)()
            except AttributeError:
                raise BuildError(errstr="Unknown configure option:'%s" % arg)


def std_switches(env):
    env.Append(CCFLAGS=['-std=c99', '-Wall', '-Wextra', '-g3'])
    # enable color output from tools.
    if 'TERM' in os.environ:
        env['ENV']['TERM'] = os.environ['TERM']
    env["CC"] = os.getenv("CC", env["CC"])


def mingw(env):
    # MinGW doesn't allow including unistd.h in c99 mode:
    # http://sourceforge.net/p/mingw/bugs/2046/
    try:
        env['CCFLAGS'].remove('-std=c99',)
    except ValueError:
        pass

    # MinGW is missing a bunch of prototypes which cause build failures,
    # even though the MinGW CRT includes the definitions
    try:
        env['CCFLAGS'].remove('-Werror',)
    except ValueError:
        pass
    env.Append(CCFLAGS=['-std=gnu99'])


def check_dependencies(env, conf=None, opts=None):
    opts = opts or list()
    conf = conf or Configure(env)
    config = {}
    config['endian'] = sys.byteorder

    optional_headers = {
        'ladspa': 'ladspa.h'
    }
    optional_libraries = {
        'jack': ['jack/jack.h', 'jack/types.h'],
        'pthread': 'pthread.h',
        'sndfile': 'sndfile.h',
        'portaudio': 'portaudio.h',
    }
    # First check the essentials
    try:
        env.ParseConfig("pkg-config lua5.2 --cflags --libs")
        config['have_lua'] = conf.CheckLibWithHeader(
            'lua5.2', ['lua.h', 'lauxlib.h'], 'C'
        )
    except OSError:
        _print("Can't find a working pkg-config for Lua5.2 - fallback config")
        config['have_lua'] = conf.CheckLibWithHeader(
            'lua', ['lua.h', 'lauxlib.h'], 'C'
        )

    if not config['have_lua']:
        raise BuildError(errstr='Cannot build without Lua')

    # Now process optional dependencies
    for name, header in optional_headers.items():
        config['have_%s' % name] = conf.CheckHeader(header)

    for name, headers in optional_libraries.items():
        config['have_%s' % name] = conf.CheckLibWithHeader(name, headers, 'C')

    # Any explicitly disabled options are removed
    for disabled in opts.disable:
        try:
            # skip if not a valid get
            _print("disabling %s" % disabled)
            if config['have_' + disabled]:
                config['have_' + disabled] = False
        except KeyError:
            raise BuildError(errstr="invalid disable option: %r" % disabled)

    return config


def configure_cpp_switches(env, config):
    """
    receives a list of configure options, then mangles the key and value to
    create C preprocessor defines.

    If the value of the key/value pair is a string, then that string is
    appended to the key e.g. a big-endian system defines "-DBMO_ENDIAN_BIG
    when using gcc and /DBMO_ENDIAN_BIG for cl.exe
    """
    anonymous_defines = [
        "BMO_{}".format(key.upper())
        for key in filter(lambda x: config[x] == True, config)
    ]
    named_defines = [
        "BMO_{}_{}".format(key.upper(), val.upper())
        for key, val in config.items() if isinstance(val, str)
    ]
    env.Append(CPPDEFINES=anonymous_defines + named_defines)


############ Environment / Config ############
SetOption('num_jobs', multiprocessing.cpu_count())
AddOption('--disable', type='string')
AddOption('--enable', type='string')
VariantDir('build', 'src', duplicate=0)
env = Environment(ENV=os.environ)
std_switches(env)

if sys.platform.startswith('win32'):
    # MSVC is the SCons default on windows but MSVC with anything approaching c99 won't work.
    env = Environment(ENV=os.environ, tools=['mingw'])
    std_switches(env)
    mingw(env)


############ Sources #############
env.Append(source=[
    Glob('build/*.c'),
    Glob('build/drivers/*.c'),
    Glob('build/dsp/*.c'),
    Glob('build/lua/*.c'),
    Glob('build/memory/*.c'),
])


############ Targets #############
beemo = env.SharedLibrary(
    'beemo',
    env['source'],
    SHLIBVERSION='%s.%s.%s' % LIB_VERSION
)

if not env.GetOption('clean'):
    # Only parse options during a real build because cleans should be fast
    opts = ArgOpts(env, env.GetOption('enable'), env.GetOption('disable'))
    opts.configure()
    conf = Configure(env)
    deps = check_dependencies(env, conf, opts)
    configure_cpp_switches(env, deps)
    env = conf.Finish()

# We need to autogenerate some files for the lua interface
lualibs = env.SConscript('src/lua/SConscript', 'env')
# Maybe run some tests
tests = env.SConscript('tests/SConscript', 'env')
# Possibly build a native installer
pkg = env.SConscript('pkg/SConscript', 'env')

env.Depends(tests, beemo)
Default(beemo)

static = env.Library('beemo', env['source'])
Default(static)
