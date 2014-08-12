import sys
import os

def release(env):
    env.Append(CCFLAGS=['-O2'])
#    env.Append(CCFLAGS=['-ffast-math'])
    env.Append(CCFLAGS=['-DNDEBUG'])
#    env.Append(CCFLAGS=['-mtune=native'])

def profile(env):
    env.Append(CCFLAGS=['-O2'])
    env.Append(CCFLAGS=['-ffast-math'])
    env.Append(CCFLAGS=['-g'])

def std_switches(env):
    env.Append(CCFLAGS=['-std=c99',])
    env.Append(CCFLAGS=['-Wall',])
    env.Append(CCFLAGS=['-Wextra',])
    env.Append(CCFLAGS=['-Werror',])

def check_dependencies(env, conf):
    config = {}
    config['endian'] = sys.byteorder
    config['have_jack'] = bool(conf.CheckLibWithHeader('jack', ['jack/jack.h', 'jack/types.h'], 'C'))
    config['have_pthread'] = bool(conf.CheckLibWithHeader('pthread', 'pthread.h', 'C' ))
    config['have_lua'] = bool(conf.CheckLibWithHeader('lua5.2', ['lua5.2/lua.h','lua5.2/lauxlib.h'], 'C'))

    #Fallback for systems with non-versioned libs
    if not config['have_lua']:
        config['have_lua'] = bool(conf.CheckLibWithHeader('lua', ['lua5.2/lua.h','lua5.2/lauxlib.h'], 'C'))
    config['have_sndfile'] = bool(conf.CheckLibWithHeader('sndfile', 'sndfile.h', 'C'))
    config['have_portaudio'] = bool(conf.CheckLibWithHeader( 'portaudio', 'portaudio.h', 'C'))
    config['have_ladspa'] = bool(conf.CheckHeader( 'ladspa.h'))
    if not config['have_lua']:
        raise StandardError('Essential dependencies not found; cannot build without Lua')
    return config

def get_preprocessor_switches(env, config):
    """
    receives a list of configure options, then mangles the key and value to create
    C preprocessor defines tailored to the compiler in use.

    e.g. if 'config' contains the key 'have_portaudio', and we have portaudio, then the define emitted
    is "/DBMO_HAVE_PORTAUDIO" on msvc or "-DBMO_HAVE_PORTAUDIO for gcc"
    This is real hackish, but I'll live.

    If the value of the key/value pair is a string, then that string is appended to the key
    e.g. a big-endian system defines "-DBMO_ENDIAN_BIG when using gcc and /DBMO_ENDIAN_BIG for cl.exe
    """
    defines = []
    dp = env.Dictionary()['CPPDEFPREFIX']
    for key in config:
        if(type(config[key]) != bool):
            defines.extend([dp + "BMO_" + key.upper()+ "_" + str(config[key]).upper()])
        elif((type(config[key]) == bool) and config[key] == True):
            defines.extend([dp + "BMO_"+ key.upper() ])
    return defines


env = Environment(ENV=os.environ)
env["CC"] = os.getenv("CC") or env["CC"]
std_switches(env)

############check build system/compiler and get user options############
if os.name == 'nt':
    #MSVC is the SCons default on windows but MSVC with anything approaching c99 won't work.
    env = Environment(ENV=os.environ, tools=['mingw'])

    #mingw doesn't allow including unistd.h in c99 mode:
    #    http://sourceforge.net/p/mingw/bugs/2046/
    try:
        i = env['CCFLAGS'].index('-std=c99')
        env['CCFLAGS'][i] = '-std=gnu99'
    except ValueError:
        env.Append(CCFLAGS=['-std=gnu99'])

if ARGUMENTS.get('analyse', False):
    """This fetches environment variables set by clang's scan-build
    to allow clang to run the static analyser
    """
    env["CC"] = os.getenv("CC") or env["CC"]
    env["CXX"] = os.getenv("CXX") or env["CXX"]
    env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

if ARGUMENTS.get('clang', 0):
    env.Replace(CC='clang')

if env.GetOption('clean'):
    deps = {}
else:
    conf = Configure(env)
    deps = check_dependencies(env, conf)
    env = conf.Finish()

######  get compiler switches  ##########
env.Append(CCFLAGS=get_preprocessor_switches(env, deps))
std_switches(env)
if ARGUMENTS.get('release', False):
    print "Building in release mode with optimisations"
    release(env)

elif ARGUMENTS.get('profile', None):
    profile(env)
else:
    env.Append(CCFLAGS=['-g3',])


############sources#############
env.Append(source = Glob('src/*.c'))
env.Append(source = Glob('src/drivers/*.c'))
env.Append(source = Glob('src/dsp/*.c'))
env.Append(source = Glob('src/lua/*.c'))
env.Append(source = Glob('src/memory/*.c'))

#####We need to autogenerate some files for the lua interface
if deps.get('have_lua', False):
    env.SConscript('src/lua/SConscript', 'env')

# Build the optional tests #

########### TODO add installer###############
if 'pkg' in COMMAND_LINE_TARGETS:
    env.SConscript("pkg/SConscript", 'env')

if os.name == 'nt':
    #we seem to need to specify the what to link the dll against on windows.
    libs = [x.lstrip('have_') for x in deps.keys() if deps[x] == True and x.startswith('have_')]
    beemo = env.SharedLibrary('beemo', env['source'], LIBS=libs)
else:
    beemo = env.SharedLibrary('beemo', env['source'])

tests = env.SConscript('tests/SConscript', 'env')
env.Depends(tests, beemo)

Default(beemo)

