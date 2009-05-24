#-*- Python -*-

import os
import pickle
import re
from build_config import *
from build_support import *

if str(Platform()) == 'win32':
    env = Environment(ENV = {'PATH' : os.environ['PATH'],
                             'LIB' : os.environ['LIB'],
                             'INCLUDE' : os.environ['INCLUDE']})
else:
    env = Environment()

missing_pkg_config = not WhereIs('pkg-config')

##################################################
# create options                                 #
##################################################
options_cache_filename = 'options.cache'
old_options_cache = ParseOptionsCacheFile(options_cache_filename)
options = Options(options_cache_filename)
options.Add('CC', 'The C-Compiler used to compile C-Files')
options.Add('CXX', 'The C++-Compiler used to compile C++-Files')
options.Add(BoolOption('debug', 'Generate debug code', 0))
options.Add(BoolOption('multithreaded', 'Generate multithreaded code', 1))
options.Add(BoolOption('dynamic', 'Generate a shared (dynamic-link) library', 1))
if WhereIs('ccache'):
    options.Add(BoolOption('use_ccache', 'Use ccache to build GG', 0))
if WhereIs('distcc'):
    options.Add(BoolOption('use_distcc', 'Use distcc to build GG', 0))
if str(Platform()) == 'win32':
    options.Add('prefix', 'Location to install GG', 'C:\\')
else:
    options.Add('prefix', 'Location to install GG', '/usr/local')
options.Add('scons_cache_dir', 'Directory to use for SCons object file caching (specifying any directory will enable caching)')
options.Add('incdir', 'Location to install headers', os.path.normpath(os.path.join('$prefix', 'include')))
options.Add('bindir', 'Location to install executables', os.path.normpath(os.path.join('$prefix', 'bin')))
options.Add('libdir', 'Location to install libraries', os.path.normpath(os.path.join('$prefix', 'lib')))
if not missing_pkg_config:
    options.Add('pkgconfigdir', 'Location to install pkg-config .pc files', '/usr/lib/pkgconfig')
options.Add('with_boost', 'Root directory of Boost installation')
options.Add('with_boost_include', 'Specify exact include dir for Boost headers')
options.Add('with_boost_libdir', 'Specify exact library dir for Boost library')
options.Add('boost_lib_suffix', 'Specify the suffix placed on user-compiled Boost libraries (e.g. "-vc71-mt-gd-1_31")')
options.Add('boost_signals_namespace',
            'Specify alternate namespace used for boost::signals (only needed if you changed it using the BOOST_SIGNALS_NAMESPACE define when you built boost)')
options.Add('with_ft', 'Root directory of FreeType2 installation')
options.Add('with_ft_include', 'Specify exact include dir for FreeType2 headers')
options.Add('with_ft_libdir', 'Specify exact library dir for FreeType2 library')
options.Add(BoolOption('use_devil', 'Enables optional use of the DevIL image-loading library.  This should only be enabled if you need to load image files other than PNG, JPEG, and TIFF', 0))
options.Add('with_devil', 'Root directory of DevIL installation (only applicable if use_devil=1)')
options.Add('with_devil_include', 'Specify exact include dir for DevIL headers (only applicable if use_devil=1)')
options.Add('with_devil_libdir', 'Specify exact library dir for DevIL library (only applicable if use_devil=1)')
options.Add('with_jpeg', 'Root directory of JPEG installation (only applicable if use_devil=0)')
options.Add('with_jpeg_include', 'Specify exact include dir for JPEG headers (only applicable if use_devil=0)')
options.Add('with_jpeg_libdir', 'Specify exact library dir for JPEG library (only applicable if use_devil=0)')
options.Add('with_png', 'Root directory of PNG installation (only applicable if use_devil=0)')
options.Add('with_png_include', 'Specify exact include dir for PNG headers (only applicable if use_devil=0)')
options.Add('with_png_libdir', 'Specify exact library dir for PNG library (only applicable if use_devil=0)')
options.Add('with_tiff', 'Root directory of TIFF installation (only applicable if use_devil=0)')
options.Add('with_tiff_include', 'Specify exact include dir for TIFF headers (only applicable if use_devil=0)')
options.Add('with_tiff_libdir', 'Specify exact library dir for TIFF library (only applicable if use_devil=0)')
options.Add(BoolOption('build_tutorials', 'Build tutorial apps (requires SDL driver).', 1))

##################################################
# Drivers                                        #
##################################################
# SDL
options.Add(BoolOption('build_sdl_driver', 'Builds GG SDL support (the GiGiSDL library)', 1))
options.Add('with_sdl', 'Root directory of SDL installation (only applicable when build_sdl_driver=1)')
options.Add('with_sdl_include', 'Specify exact include dir for SDL headers (only applicable when build_sdl_driver=1)')
options.Add('with_sdl_libdir', 'Specify exact library dir for SDL library (only applicable when build_sdl_driver=1)')

# Ogre
options.Add(BoolOption('build_ogre_driver', 'Builds GG Ogre support (the GiGiOgre library)', 1))
options.Add('with_ogre', 'Root directory of Ogre installation (only applicable when build_ogre_driver=1)')
options.Add('with_ogre_include', 'Specify exact include dir for Ogre headers (only applicable when build_ogre_driver=1)')
options.Add('with_ogre_libdir', 'Specify exact library dir for Ogre library (only applicable when build_ogre_driver=1)')
options.Add(BoolOption('build_ogre_ois_plugin', 'Builds OIS input plugin for the GiGiOgre library', 1))
options.Add('with_ois', 'Root directory of OIS installation (only applicable when build_ogre_ois_plugin=1)')
options.Add('with_ois_include', 'Specify exact include dir for OIS headers (only applicable when build_ogre_ois_plugin=1)')
options.Add('with_ois_libdir', 'Specify exact library dir for OIS library (only applicable when build_ogre_ois_plugin=1)')



##################################################
# build vars                                     #
##################################################
# important vars that need to be set for later use when generating and building certain targets

# This is supposed to be detected during configuration, but sometimes isn't, and is harmless, so we're including it all
# the time now on POSIX systems.
env['need__vsnprintf_c'] = str(Platform()) == 'posix'

env['have_jpeg'] = False
env['have_png'] = False
env['have_tiff'] = False


# fill Environment using saved and command-line provided options, save options for next time, and fill environment
# with save configuration values.
import sys
gigi_preconfigured = False
sdl_preconfigured = False
ogre_preconfigured = False
ogre_ois_preconfigured = False
force_configure = False
command_line_args = sys.argv[1:]
help_only = ('-h' in command_line_args) or ('--help' in command_line_args)
if 'configure' in command_line_args and not help_only:
    force_configure = True
elif help_only:
    # ensure configuration gets skipped when help is requested
    gigi_preconfigured = True
    sdl_preconfigured = True
    ogre_preconfigured = True
    ogre_ois_preconfigured = True
ms_linker = 'msvs' in env['TOOLS'] or 'msvc' in env['TOOLS']

env_cache_keys = [
    'CC',
    'CXX',
    'CCFLAGS',
    'CPPDEFINES',
    'CPPFLAGS',
    'CPPPATH',
    'CXXFLAGS',
    'LIBPATH',
    'LIBS',
    'LINKFLAGS',
    'need__vsnprintf_c',
    'libltdl_defines',
    'build_sdl_driver',
    'build_ogre_driver',
    'build_ogre_ois_plugin',
    'have_jpeg',
    'have_png',
    'have_tiff'
    ]
if not force_configure:
    try:
        f = open('gigi_config.cache', 'r')
        up = pickle.Unpickler(f)
        pickled_values = up.load()
        for key, value in pickled_values.items():
            env[key] = value
        gigi_preconfigured = True
    except Exception:
        pass

options.Update(env)

if os.environ.has_key('PKG_CONFIG_PATH'):
    env['ENV']['PKG_CONFIG_PATH'] = os.environ['PKG_CONFIG_PATH']

if env.has_key('use_distcc') and env['use_distcc']:
    env['CC'] = 'distcc %s' % env['CC']
    env['CXX'] = 'distcc %s' % env['CXX']
    for i in ['HOME',
              'DISTCC_HOSTS',
              'DISTCC_VERBOSE',
              'DISTCC_LOG',
              'DISTCC_FALLBACK',
              'DISTCC_MMAP',
              'DISTCC_SAVE_TEMPS',
              'DISTCC_TCP_CORK',
              'DISTCC_SSH']:
        if os.environ.has_key(i) and not env.has_key(i):
            env['ENV'][i] = os.environ[i]

if env.has_key('use_ccache') and env['use_ccache']:
    env['CC'] = 'ccache %s' % env['CC']
    env['CXX'] = 'ccache %s' % env['CXX']
    for i in ['HOME',
              'CCACHE_DIR',
              'CCACHE_TEMPDIR',
              'CCACHE_LOGFILE',
              'CCACHE_PATH',
              'CCACHE_CC',
              'CCACHE_PREFIX',
              'CCACHE_DISABLE',
              'CCACHE_READONLY',
              'CCACHE_CPP2',
              'CCACHE_NOSTATS',
              'CCACHE_NLEVELS',
              'CCACHE_HARDLINK',
              'CCACHE_RECACHE',
              'CCACHE_UMASK',
              'CCACHE_HASHDIR',
              'CCACHE_UNIFY',
              'CCACHE_EXTENSION']:
        if os.environ.has_key(i) and not env.has_key(i):
            env['ENV'][i] = os.environ[i]

Help(GenerateHelpText(options, env))
options.Save(options_cache_filename, env)

new_options_cache = ParseOptionsCacheFile(options_cache_filename)
if gigi_preconfigured and sdl_preconfigured and ogre_preconfigured and ogre_ois_preconfigured:
    for i in old_options_cache.keys():
        if not new_options_cache.has_key(i) or old_options_cache[i] != new_options_cache[i]:
            gigi_preconfigured = False
            sdl_preconfigured = False
            ogre_preconfigured = False
            ogre_ois_preconfigured = False
            break
if gigi_preconfigured and sdl_preconfigured and ogre_preconfigured and ogre_ois_preconfigured:
    for i in new_options_cache.keys():
        if not old_options_cache.has_key(i) or old_options_cache[i] != new_options_cache[i]:
            gigi_preconfigured = False
            sdl_preconfigured = False
            ogre_preconfigured = False
            ogre_ois_preconfigured = False
            break

if not force_configure and env['build_sdl_driver']:
    sdl_env = env.Clone()
    try:
        f = open('sdl_config.cache', 'r')
        up = pickle.Unpickler(f)
        pickled_values = up.load()
        for key, value in pickled_values.items():
            sdl_env[key] = value
        sdl_preconfigured = True
        if not sdl_env['build_sdl_driver']:
            print 'Warning: You have requested to build SDL support, but SDL was not found, and so has been disabled.  To fix this, run scons configure.'
            env['build_sdl_driver'] = False
    except Exception:
        pass

if not force_configure and env['build_ogre_driver']:
    ogre_env = env.Clone()
    try:
        f = open('ogre_config.cache', 'r')
        up = pickle.Unpickler(f)
        pickled_values = up.load()
        for key, value in pickled_values.items():
            ogre_env[key] = value
        ogre_preconfigured = True
        if not ogre_env['build_ogre_driver']:
            print 'Warning: You have requested to build Ogre support, but Ogre was not found, and so has been disabled.  To fix this, run scons configure.'
            env['build_ogre_driver'] = False
    except Exception:
        pass

if not force_configure and env['build_ogre_driver'] and env['build_ogre_ois_plugin']:
    ogre_ois_env = ogre_env.Clone()
    try:
        f = open('ogre_ois_config.cache', 'r')
        up = pickle.Unpickler(f)
        pickled_values = up.load()
        for key, value in pickled_values.items():
            ogre_ois_env[key] = value
        ogre_ois_preconfigured = True
        if not ogre_ois_env['build_ogre_ois_plugin']:
            print 'Warning: You have requested to build the Ogre OIS plugin, but OIS was not found, and so has been disabled.  To fix this, run scons configure.'
            env['build_ogre_ois_plugin'] = False
            ogre_env['build_ogre_ois_plugin'] = False
    except Exception:
        pass

if not env['multithreaded']:
    if env['build_sdl_driver']:
        print 'Warning: since multithreaded code is disabled, the GiGiSDL build is disabled as well.'
        env['build_sdl_driver'] = 0
if str(Platform()) == 'win32':
    if not env['multithreaded']:
        if env['dynamic']:
            print 'Warning: since the Win32 platform does not support singlethreaded DLLs, singlethreaded static libraries will be produced instead.'
            env['dynamic'] = 0


if gigi_preconfigured and sdl_preconfigured and ogre_preconfigured and ogre_ois_preconfigured and not help_only:
    print 'Using previous successful configuration; if you want to re-run the configuration step, run "scons configure".'


##################################################
# check configuration                            #
##################################################
if not env.GetOption('clean'):
    custom_tests_dict = {'CheckVersionHeader' : CheckVersionHeader,
                         'CheckPkgConfig' : CheckPkgConfig,
                         'CheckPkg' : CheckPkg,
                         'CheckBoost' : CheckBoost,
                         'CheckBoostLib' : CheckBoostLib,
                         'CheckSDL' : CheckSDL,
                         'CheckLibLTDL' : CheckLibLTDL,
                         'CheckConfigSuccess' : CheckConfigSuccess}

    if not gigi_preconfigured:
        conf = env.Configure(custom_tests = custom_tests_dict)
        
        pkg_config = conf.CheckPkgConfig('0.15.0')

        if str(Platform()) == 'posix':
            print 'Configuring for POSIX system...'
        elif str(Platform()) == 'win32':
            print 'Configuring for WIN32 system...'
        else:
            print 'Configuring unknown system (assuming the system is POSIX-like) ...'

        signals_namespace = 'signals'
        if OptionValue('boost_signals_namespace', env):
            signals_namespace = OptionValue('boost_signals_namespace', env)
            env.AppendUnique(CPPDEFINES = [
                ('BOOST_SIGNALS_NAMESPACE', signals_namespace),
                ('signals', signals_namespace)
                ])

        boost_libs = [
            ('boost_signals', 'boost/signals.hpp', 'boost::' + signals_namespace + '::connection();'),
            ('boost_system', 'boost/system/error_code.hpp', 'boost::system::get_system_category();'),
            ('boost_filesystem', 'boost/filesystem/operations.hpp', 'boost::filesystem::initial_path();'),
            ('boost_thread', 'boost/thread/thread.hpp', 'boost::thread::yield();')
            ]
        if not conf.CheckBoost(boost_version_string, boost_libs, conf, not ms_linker):
            Exit(1)

        # pthreads
        if str(Platform()) == 'posix':
            if env['multithreaded']:
                if conf.CheckCHeader('pthread.h') and conf.CheckLib('pthread', 'pthread_create', autoadd = 0):
                    env.Append(CCFLAGS = ['-pthread'])
                    env.Append(LINKFLAGS = ['-pthread'])
                else:
                    Exit(1)

        # GL and GLU
        if str(Platform()) == 'win32':
            env.Append(LIBS = [
                'opengl32.lib',
                'glu32.lib'
                ])
        else:
            if not conf.CheckCHeader('GL/gl.h') or \
                   not conf.CheckCHeader('GL/glu.h') or \
                   not conf.CheckLib('GL', 'glBegin') or \
                   not conf.CheckLib('GLU', 'gluLookAt'):
                Exit(1)

        # FreeType2
        AppendPackagePaths('ft', env)
        found_it_with_pkg_config = False
        if pkg_config:
            if conf.CheckPkg('freetype2', ft_pkgconfig_version):
                env.ParseConfig('pkg-config --cflags --libs freetype2')
                found_it_with_pkg_config = True
        if not found_it_with_pkg_config:
            version_regex = re.compile(r'FREETYPE_MAJOR\s*(\d+).*FREETYPE_MINOR\s*(\d+).*FREETYPE_PATCH\s*(\d+)', re.DOTALL)
            if not conf.CheckVersionHeader('freetype2', 'freetype/freetype.h', version_regex, ft_version, True):
                Exit(1)
        if not conf.CheckCHeader('ft2build.h'):
            Exit(1)
        if str(Platform()) != 'win32':
            if not conf.CheckLib('freetype', 'FT_Init_FreeType'):
                Exit(1)
        else:
            env.Append(LIBS = [ft_win32_lib_name])

        # Image loading lib(s)
        if env['use_devil']:
            # DevIL (aka IL, aka OpenIL)
            AppendPackagePaths('devil', env)
            version_regex = re.compile(r'IL_VERSION\s*(\d+)')
            if not conf.CheckVersionHeader('DevIL', 'IL/il.h', version_regex, devil_version, True, 'Checking DevIL version >= %s... ' % devil_version_string):
                Exit(1)
            if not conf.CheckCHeader('IL/il.h') or \
                   not conf.CheckCHeader('IL/ilu.h'):
                Exit(1)
            if str(Platform()) != 'win32' and (not conf.CheckLib('IL', 'ilInit') or \
                                               not conf.CheckLib('ILU', 'iluInit')):
                print 'Trying IL again with local _vnsprintf.c...'
                if conf.CheckLib('IL', 'ilInit', header = '#include "../src/_vsnprintf.c"') and \
                       conf.CheckLib('ILU', 'iluInit', header = '#include "../src/_vsnprintf.c"'):
                    env['need__vsnprintf_c'] = True
                    print 'DevIL is broken.  It depends on a function _vnsprintf that does not exist on your system.  GG will provide a vnsprintf wrapper called _vnsprintf.'
                else:
                    Exit(1)
        else:
            # no DevIL
            AppendPackagePaths('jpeg', env)
            if not conf.CheckCHeader(['stdio.h', 'jpeglib.h']):
                env['have_jpeg'] = False
            elif str(Platform()) != 'win32' and not conf.CheckLib('jpeg'):
                env['have_jpeg'] = False
            else:
                env['have_jpeg'] = True
                env.AppendUnique(LIBS = ['jpeg'])
            AppendPackagePaths('png', env)
            if not conf.CheckCHeader('png.h'):
                env['have_png'] = False
            elif str(Platform()) != 'win32' and not conf.CheckLib('png'):
                env['have_png'] = False
            else:
                env['have_png'] = True
                env.AppendUnique(LIBS = ['png'])
            AppendPackagePaths('tiff', env)
            if not conf.CheckCHeader('tiffio.h'):
                env['have_tiff'] = False
            elif str(Platform()) != 'win32' and not conf.CheckLib('tiff'):
                env['have_tiff'] = False
            else:
                env['have_tiff'] = True
                env.AppendUnique(LIBS = ['tiff'])
            if not (env['have_jpeg'] or env['have_png'] or not env['have_tiff']):
                Exit(1)

        # ltdl
        if str(Platform()) != 'win32':
            if not conf.CheckLibLTDL():
                print 'Check libltdl/config.log to see what went wrong.'
                Exit(1)

        # define platform-specific flags
        env.Append(CPPPATH = [
            '#',
            '#/libltdl'
            ])

        if str(Platform()) == 'win32':
            if env['multithreaded']:
                if env['dynamic']:
                    env.AppendUnique(CPPDEFINES = ['BOOST_ALL_DYN_LINK'])
                    if env['debug']:
                        code_generation_flag = '/MDd'
                    else:
                        code_generation_flag = '/MD'
                else:
                    if env['debug']:
                        code_generation_flag = '/MTd'
                    else:
                        code_generation_flag = '/MT'
            else:
                if env['debug']:
                    code_generation_flag = '/MLd'
                else:
                    code_generation_flag = '/ML'
            flags = [
                code_generation_flag,
                '/EHsc',
                '/W3',
                '/Zc:forScope',
                '/GR',
                '/Gd',
                '/Z7',
                '/wd4146', '/wd4099', '/wd4251', '/wd4800', '/wd4267', '/wd4275', '/wd4244', '/wd4101', '/wd4258', '/wd4351', '/wd4996'
                ]
            env.AppendUnique(CCFLAGS = flags)
            env.AppendUnique(CPPDEFINES = [
                (env['debug'] and '_DEBUG' or 'NDEBUG'),
                'WIN32',
                '_WINDOWS',
                'ADOBE_TEST_MICROSOFT_NO_DEPRECATE=0'
                ])
            if env['dynamic']:
                env.AppendUnique(CPPDEFINES = [
                '_USRDLL',
                '_WINDLL'
                ])
            env.AppendUnique(LINKFLAGS = [
                '/NODEFAULTLIB:LIBCMT',
                '/DEBUG'
                ])
            env.AppendUnique(LIBS = [
                'kernel32',
                'user32',
                'gdi32',
                'winspool',
                'comdlg32',
                'advapi32',
                'shell32',
                'ole32',
                'oleaut32',
                'uuid',
                'odbc32',
                'odbccp32',
                ])
        else:
            if env['debug']:
                env.Append(CCFLAGS = ['-Wall', '-Wno-parentheses', '-g', '-O0'])
            else:
                env.Append(CCFLAGS = ['-Wall', '-Wno-parentheses', '-O2'])

        env['libltdl_defines'] = [
            'HAVE_CONFIG_H'
            ]
        if env.has_key('CPPDEFINES'):
            env['libltdl_defines'] += env['CPPDEFINES']

        if env['debug']:
            env.AppendUnique(CPPDEFINES = ['ADOBE_STD_SERIALIZATION'])

        # finish config and save results for later
        conf.CheckConfigSuccess(True)
        conf.Finish();
        f = open('gigi_config.cache', 'w')
        p = pickle.Pickler(f)
        cache_dict = {}
        for i in env_cache_keys:
            cache_dict[i] = env.has_key(i) and env.Dictionary(i) or []
        p.dump(cache_dict)

        # for win32, create libltdl/config.h
        if str(Platform()) == 'win32':
            f = open(os.path.normpath('libltdl/config.h'), 'w')
            f.write("""/* WARNING: Generated by GG's SConstruct file.  All local changes will be lost! */
#define error_t int
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_CTYPE_H 1
#define HAVE_MEMORY_H 1
#define HAVE_ERRNO_H 1
#define __WIN32__
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define LTDL_OBJDIR ".libs"
#define LTDL_DLOPEN_DEPLIBS 1
#define LTDL_SHLIBPATH_VAR "PATH"
#define LTDL_SHLIB_EXT ".dll"
""")
            f.close()

        # copy ltdl.h and config.h into the header tree before compiling
        Execute(Copy(os.path.normpath('GG/ltdl.h'), os.path.normpath('libltdl/ltdl.h')))
        Execute(Copy(os.path.normpath('GG/ltdl_config.h'), os.path.normpath('libltdl/config.h')))

    # SDL
    if not sdl_preconfigured and env['build_sdl_driver']:
        print 'Configuring GiGiSDL driver...'
        sdl_env = env.Clone()
        sdl_conf = sdl_env.Configure(custom_tests = custom_tests_dict)
        sdl_config_script = WhereIs('sdl-config')
        if not sdl_conf.CheckSDL(options, sdl_conf, sdl_config_script, not ms_linker):
            print 'Warning: SDL not configured.  The GiGiSDL library will not be built!'
            env['build_sdl_driver'] = False
            sdl_env['build_sdl_driver'] = False
        sdl_conf.CheckConfigSuccess(sdl_env['build_sdl_driver'])
        sdl_conf.Finish();
        f = open('sdl_config.cache', 'w')
        p = pickle.Pickler(f)
        cache_dict = {}
        for i in env_cache_keys:
            cache_dict[i] = sdl_env.has_key(i) and sdl_env.Dictionary(i) or []
        p.dump(cache_dict)

    # Ogre
    if not ogre_preconfigured and env['build_ogre_driver']:
        print 'Configuring GiGiOgre driver...'
        ogre_env = env.Clone()
        ogre_conf = ogre_env.Configure(custom_tests = custom_tests_dict)
        pkg_config = ogre_conf.CheckPkgConfig('0.15.0')
        AppendPackagePaths('ogre', ogre_env)
        found_it_with_pkg_config = False
        if pkg_config:
            if ogre_conf.CheckPkg('OGRE', ogre_version):
                ogre_env.ParseConfig('pkg-config --cflags --libs OGRE')
                found_it_with_pkg_config = True
        ogre_config_failed = False
        if not found_it_with_pkg_config:
            version_regex = re.compile(r'OGRE_VERSION_MAJOR\s*(\d+).*OGRE_VERSION_MINOR\s*(\d+).*OGRE_VERSION_PATCH\s*(\d+)', re.DOTALL)
            if not ogre_conf.CheckVersionHeader('Ogre', 'OgrePrerequisites.h', version_regex, ogre_version, True):
                ogre_config_failed = True
        if not ogre_config_failed and not ogre_conf.CheckCXXHeader('Ogre.h'):
            ogre_config_failed = True
        if not ogre_config_failed:
            if str(Platform()) != 'win32':
                if not ogre_conf.CheckLib('OgreMain', 'Ogre::Root', '#include <Ogre.h>', 'C++'):
                    ogre_config_failed = True
            else:
                ogre_env.Append(LIBS = ['OgreMain'])

        if ogre_config_failed:
            print 'Warning: Ogre not configured.  The GiGiOgre library will not be built!'
            env['build_ogre_driver'] = False
            ogre_env['build_ogre_driver'] = False
        ogre_conf.CheckConfigSuccess(ogre_env['build_ogre_driver'])
        ogre_conf.Finish();
        f = open('ogre_config.cache', 'w')
        p = pickle.Pickler(f)
        cache_dict = {}
        for i in env_cache_keys:
            cache_dict[i] = ogre_env.has_key(i) and ogre_env.Dictionary(i) or []
        p.dump(cache_dict)

    # Ogre OIS Plugin
    if not ogre_ois_preconfigured and env['build_ogre_driver'] and env['build_ogre_ois_plugin']:
        print "Configuring GiGiOgre's OIS plugin..."
        ogre_ois_env = ogre_env.Clone()
        ogre_ois_conf = ogre_ois_env.Configure(custom_tests = custom_tests_dict)
        pkg_config = ogre_ois_conf.CheckPkgConfig('0.15.0')
        ois_config_failed = False
        AppendPackagePaths('ois', ogre_ois_env)
        found_it_with_pkg_config = False
        if pkg_config:
            if ogre_ois_conf.CheckPkg('OIS', ois_version):
                ogre_ois_env.ParseConfig('pkg-config --cflags --libs OIS')
                found_it_with_pkg_config = True
        if not found_it_with_pkg_config:
            version_regex = re.compile(r'OIS_VERSION_MAJOR\s*(\d+).*OIS_VERSION_MINOR\s*(\d+).*OIS_VERSION_PATCH\s*(\d+)', re.DOTALL)
            if not ogre_ois_conf.CheckVersionHeader('OIS', 'OISPrereqs.h', version_regex, ois_version, True):
                ois_config_failed = True
        if not ois_config_failed and not ogre_ois_conf.CheckCXXHeader('OIS.h'):
            ois_config_failed = True
        if str(Platform()) == 'win32':
            ogre_ois_env.AppendUnique(LIBS = 'OIS')
        if ois_config_failed:
            print "Warning: OIS not configured.  The GiGiOgre library's OIS plugin will not be built!"
            env['build_ogre_ois_plugin'] = False
            ogre_env['build_ogre_ois_plugin'] = False
            ogre_ois_env['build_ogre_ois_plugin'] = False
        ogre_ois_conf.CheckConfigSuccess(not ois_config_failed)
        ogre_ois_conf.Finish();
        f = open('ogre_ois_config.cache', 'w')
        p = pickle.Pickler(f)
        cache_dict = {}
        for i in env_cache_keys:
            cache_dict[i] = ogre_ois_env.has_key(i) and ogre_ois_env.Dictionary(i) or []
        p.dump(cache_dict)

    if not help_only:
        print '''
Summary:
    Build GiGi.........................................Yes
    Build GiGiSDL......................................%s
    Build GiGiOgre.....................................%s
    Build GiGiOgrePlugin_OIS...........................%s
    Build Tutorials (requires GiGiSDL).................%s

Code generation:
    Debug/Release......................................%s
    Single-/Multi-threaded.............................%s
    Dynamic/Static.....................................%s

Image Loading:
    Use DevIL..........................................%s
''' % \
        (TruthStr(env['build_sdl_driver']),
         TruthStr(env['build_ogre_driver']),
         TruthStr(env['build_ogre_driver'] and env['build_ogre_ois_plugin']),
         TruthStr(env['build_sdl_driver'] and env['build_tutorials']),
         env['debug'] and 'Debug' or 'Release',
         env['multithreaded'] and 'Multi-threaded' or 'Single-threaded',
         env['dynamic'] and 'Dynamic' or 'Static',
         TruthStr(env['use_devil']))
        if env['use_devil']:
            print '''    PNG Files..........................................[Via DevIL]
    JPEG Files.........................................[Via DevIL]
    TIFF Files.........................................[Via DevIL]
'''
        else:
            print '''    PNG Files..........................................%s
    JPEG Files.........................................%s
    TIFF Files.........................................%s
''' % \
            (TruthStr(env['have_png']),
             TruthStr(env['have_jpeg']),
             TruthStr(env['have_tiff']))

    # We re-save here to ensure that any changes to the options necessitated
    # by the configure step are preserved.
    options.Save(options_cache_filename, env)

    if 'configure' in command_line_args:
        Exit(0)

##################################################
# define targets                                 #
##################################################
Export('env')

# define libGiGi objects
gigi_env = env.Clone()
if str(Platform()) == 'win32' and gigi_env['dynamic']:
    gigi_env.AppendUnique(CPPDEFINES = ['GIGI_EXPORTS'])
if help_only:
    lib_gigi = ['gigi']
else:
    gigi_objects, gigi_sources = SConscript(os.path.normpath('src/SConscript'), exports = 'gigi_env')
    result_objects, result_sources = SConscript(os.path.normpath('libltdl/SConscript'), exports = 'gigi_env')
    gigi_objects += result_objects
    gigi_sources += result_sources

    if env['dynamic']:
        lib_gigi = env.SharedLibrary('GiGi', gigi_objects)
    else:
        lib_gigi = env.StaticLibrary('GiGi', gigi_objects)

# define libGiGiSDL objects
if env['build_sdl_driver']:
    if str(Platform()) == 'win32':
        sdl_env.Append(LIBS = ['SDL', 'GiGi'])
        if sdl_env['dynamic']:
            sdl_env.AppendUnique(CPPDEFINES = ['GIGI_SDL_EXPORTS'])
    gigi_sdl_objects, gigi_sdl_sources = SConscript(os.path.normpath('src/SDL/SConscript'), exports = 'sdl_env')

    if env['dynamic']:
        lib_gigi_sdl = sdl_env.SharedLibrary('GiGiSDL', gigi_sdl_objects)
    else:
        lib_gigi_sdl = sdl_env.StaticLibrary('GiGiSDL', gigi_sdl_objects)

    Depends(lib_gigi_sdl, lib_gigi)

    # The tutorials depend on libGiGiSDL, so we add targets for them here
    tutorial_env = sdl_env.Clone()
    tutorial_env.PrependUnique(LIBPATH = ['.'])
    tutorial_env.AppendUnique(LIBS = ['GiGi', 'GiGiSDL'])
    if str(Platform()) == 'win32':
        tutorial_env.AppendUnique(LIBS = [
            'opengl32',
            'glu32',
            'wsock32',
            'kernel32',
            'user32',
            'gdi32.lib',
            'winspool.lib',
            'comdlg32.lib',
            'SDLmain',
            ])
        tutorial_env.AppendUnique(CCFLAGS = [
            '/D "WIN32"',
            '/D "_WINDOWS"',
            '/D "BOOST_SIGNALS_STATIC_LINK"',
            '/D "_MBCS"',
            '/FD',
            '/EHsc',
            '/MD',
            '/GS',
            '/Zc:forScope',
            '/GR',
            '/W3',
            '/nologo',
            '/c',
            '/Wp64',
            '/Z7',
            '/wd4099',
            '/wd4251',
            '/wd4800',
            '/wd4267',
            '/wd4275',
            '/wd4244',
            '/wd4101',
            '/wd4258'
            ])
        tutorial_env.AppendUnique(LINKFLAGS = [
            '/INCREMENTAL:NO',
            '/NOLOGO',
            '/NODEFAULTLIB:"LIBCMT"',
            '/DEBUG',
            '/SUBSYSTEM:CONSOLE',
            '/OPT:REF',
            '/OPT:ICF',
            '/MACHINE:X86'
            ])
    else:
        serialization_lib_name = 'boost_serialization'
        suffix = OptionValue('boost_lib_suffix', tutorial_env)
        if suffix:
            serialization_lib_name += suffix
        tutorial_env.AppendUnique(LIBS = [serialization_lib_name])

    tutorials = [
        tutorial_env.Program('minimal', ['tutorial/minimal.cpp']),
        tutorial_env.Program('controls', ['tutorial/controls.cpp']),
        tutorial_env.Program('serialization', ['tutorial/serialization.cpp', 'tutorial/saveload.cpp']),
        tutorial_env.Program('adam', ['tutorial/adam.cpp'])
        ]


# define libGiGiOgre objects
if env['build_ogre_driver']:
    if str(Platform()) == 'win32':
        ogre_env.Append(LIBS = ['GiGi'])
        if ogre_env['dynamic']:
            ogre_env.AppendUnique(CPPDEFINES = ['GIGI_OGRE_EXPORTS'])
    gigi_ogre_objects, gigi_ogre_sources = SConscript(os.path.normpath('src/Ogre/SConscript'), exports = 'ogre_env')

    if env['dynamic']:
        lib_gigi_ogre = ogre_env.SharedLibrary('GiGiOgre', gigi_ogre_objects)
    else:
        ogre_env.AppendUnique(CPPDEFINES = ['OGRE_STATIC_LIB'])
        lib_gigi_ogre = ogre_env.StaticLibrary('GiGiOgre', gigi_ogre_objects)

    Depends(lib_gigi_ogre, lib_gigi)

# define libGiGiOgre Plugin objects
if env['build_ogre_driver']:
    ogre_plugin_envs = {}
    if env['build_ogre_ois_plugin']:
        ogre_plugin_envs['OIS'] = ogre_ois_env
    for key, value in ogre_plugin_envs.items():
        if str(Platform()) == 'win32':
            value.AppendUnique(LIBS = ['GiGi', 'GiGiOgre'])
            if value['dynamic']:
                if 'GIGI_OGRE_EXPORTS' in value['CPPDEFINES']:
                    value['CPPDEFINES'].remove('GIGI_OGRE_EXPORTS')
                value.AppendUnique(CPPDEFINES = ['GIGI_OGRE_PLUGIN_EXPORTS'])
    gigi_ogre_plugin_objects, gigi_ogre_plugin_sources = SConscript(os.path.normpath('src/Ogre/Plugins/SConscript'), exports = 'ogre_plugin_envs')

    lib_gigi_ogre_plugins = {}
    for key, value in gigi_ogre_plugin_objects.items():
        if env['dynamic']:
            lib_gigi_ogre_plugins[key] = ogre_plugin_envs[key].SharedLibrary(OgrePluginName(key), value)
        else:
            ogre_plugin_envs[key].AppendUnique(CPPDEFINES = ['OGRE_STATIC_LIB'])
            lib_gigi_ogre_plugins[key] = ogre_plugin_envs[key].StaticLibrary(OgrePluginName(key), value)
        Depends(lib_gigi_ogre_plugins[key], lib_gigi_ogre)

# Generate pkg-config .pc files
if not missing_pkg_config and str(Platform()) != 'win32' and not help_only:
    CreateGiGiPCFile(['GiGi.pc'], ['GiGi.pc.in'], env)
    if env['build_sdl_driver']:
        CreateGiGiDriverPCFile(['GiGiSDL.pc'], ['GiGiSDL.pc.in'], sdl_env)
    if env['build_ogre_driver']:
        CreateGiGiDriverPCFile(['GiGiOgre.pc'], ['GiGiOgre.pc.in'], ogre_env)

# Generate GG/Config.h
CreateConfigHeader(['GG/Config.h'], ['Config.h.in'], env)

header_dir = os.path.normpath(os.path.join(env.subst(env['incdir']), 'GG'))
lib_dir = os.path.normpath(env.subst(env['libdir']))

# install target
gigi_libname = str(lib_gigi[0])
installed_gigi_libname = gigi_libname

if env['build_sdl_driver']:
    gigi_sdl_libname = str(lib_gigi_sdl[0])
    installed_gigi_sdl_libname = gigi_sdl_libname

if env['build_ogre_driver']:
    gigi_ogre_libname = str(lib_gigi_ogre[0])
    installed_gigi_ogre_libname = gigi_ogre_libname

if str(Platform()) == 'posix' and env['dynamic']:
    gigi_version_suffix = '.' + gigi_version
    installed_gigi_libname += gigi_version_suffix
    if env['build_sdl_driver']:
        installed_gigi_sdl_libname += gigi_version_suffix
    if env['build_ogre_driver']:
        installed_gigi_ogre_libname += gigi_version_suffix

for root, dirs, files in os.walk('GG'):
    if '.svn' in dirs:
        dirs.remove('.svn')
    if 'CVS' in dirs:
        dirs.remove('CVS')
    for f in [f for f in files if f.find('.scons') != 0]:
        Alias('install', Install(os.path.normpath(os.path.join(env.subst(env['incdir']), root)),
                                 os.path.normpath(os.path.join(root, f))))

if str(Platform()) == 'win32':
    Alias('install', Install(lib_dir, lib_gigi))
else:
    Alias('install', InstallAs(lib_dir + '/' + installed_gigi_libname, lib_gigi))
    if env['dynamic']:
        Alias('install',
              env.Command(lib_dir + '/' + gigi_libname,
                          lib_dir + '/' + installed_gigi_libname,
                          'ln -s ' + lib_dir + '/' + installed_gigi_libname + ' ' + lib_dir + '/' + gigi_libname))
if not missing_pkg_config and str(Platform()) != 'win32':
    Alias('install', Install(env.subst(env['pkgconfigdir']), env.File('GiGi.pc')))
if env['build_sdl_driver']:
    if str(Platform()) == 'win32':
        Alias('install', Install(lib_dir, lib_gigi_sdl))
    else:
        Alias('install', InstallAs(lib_dir + '/' + installed_gigi_sdl_libname, lib_gigi_sdl))
        if env['dynamic']:
            Alias('install',
                  env.Command(lib_dir + '/' + gigi_sdl_libname,
                              lib_dir + '/' + installed_gigi_sdl_libname,
                              'ln -s ' + lib_dir + '/' + installed_gigi_sdl_libname + ' ' + lib_dir + '/' + gigi_sdl_libname))
    if not missing_pkg_config and str(Platform()) != 'win32':
        Alias('install', Install(env.subst(env['pkgconfigdir']), env.File('GiGiSDL.pc')))
if env['build_ogre_driver']:
    if str(Platform()) == 'win32':
        Alias('install', Install(lib_dir, lib_gigi_ogre))
    else:
        Alias('install', InstallAs(lib_dir + '/' + installed_gigi_ogre_libname, lib_gigi_ogre))
        if env['dynamic']:
            Alias('install',
                  env.Command(lib_dir + '/' + gigi_ogre_libname,
                              lib_dir + '/' + installed_gigi_ogre_libname,
                              'ln -s ' + lib_dir + '/' + installed_gigi_ogre_libname + ' ' + lib_dir + '/' + gigi_ogre_libname))
    if not missing_pkg_config and str(Platform()) != 'win32':
        Alias('install', Install(env.subst(env['pkgconfigdir']), env.File('GiGiOgre.pc')))
if env['build_ogre_driver'] and env['build_ogre_ois_plugin']:
    for key, value in lib_gigi_ogre_plugins.items():
        Alias('install', Install(lib_dir, value))

deletions = [
    Delete(header_dir),
    Delete(os.path.normpath(os.path.join(lib_dir, str(lib_gigi[0]))))
    ]
if str(Platform()) == 'posix' and env['dynamic']:
    deletions.append(Delete(os.path.normpath(os.path.join(lib_dir, installed_gigi_libname))))
if not missing_pkg_config and str(Platform()) != 'win32':
    deletions.append(Delete(os.path.normpath(os.path.join(env.subst(env['pkgconfigdir']), 'GiGi.pc'))))
if env['build_sdl_driver']:
    deletions.append(Delete(os.path.normpath(os.path.join(lib_dir, str(lib_gigi_sdl[0])))))
    if str(Platform()) == 'posix' and env['dynamic']:
        deletions.append(Delete(os.path.normpath(os.path.join(lib_dir, installed_gigi_sdl_libname))))
    if not missing_pkg_config and str(Platform()) != 'win32':
        deletions.append(Delete(os.path.normpath(os.path.join(env.subst(env['pkgconfigdir']), 'GiGiSDL.pc'))))
if env['build_ogre_driver']:
    deletions.append(Delete(os.path.normpath(os.path.join(lib_dir, str(lib_gigi_ogre[0])))))
    if str(Platform()) == 'posix' and env['dynamic']:
        deletions.append(Delete(os.path.normpath(os.path.join(lib_dir, installed_gigi_ogre_libname))))
    if not missing_pkg_config and str(Platform()) != 'win32':
        deletions.append(Delete(os.path.normpath(os.path.join(env.subst(env['pkgconfigdir']), 'GiGiOgre.pc'))))
if env['build_ogre_driver']:
    for key, value in lib_gigi_ogre_plugins.items():
        deletions.append(Delete(os.path.normpath(os.path.join(lib_dir, str(value)))))
uninstall = env.Command('uninstall', '', deletions)
env.AlwaysBuild(uninstall)
env.Precious(uninstall)

# default targets
default_targets = [lib_gigi]
if env['build_sdl_driver']:
    default_targets += lib_gigi_sdl
    if env['build_tutorials']:
        default_targets += tutorials
if env['build_ogre_driver']:
    default_targets += lib_gigi_ogre
if env['build_ogre_driver'] and env['build_ogre_ois_plugin']:
    for key, value in lib_gigi_ogre_plugins.items():
        default_targets += value
Default(default_targets)

if OptionValue('scons_cache_dir', env):
    CacheDir(OptionValue('scons_cache_dir', env))
