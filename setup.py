from distutils.core import setup, Extension
import numpy
import sys
import os

if sys.platform == 'darwin':
    from distutils import sysconfig
    vars = sysconfig.get_config_vars()
    vars['LDSHARED'] = vars['LDSHARED'].replace('-bundle', '-dynamiclib')

# Please uncomment these two lines if you want to specify the compiler
# os.environ["CC"] = "/usr/bin/g++"
# os.environ['CXX'] = "/usr/bin/g++"
bsf_mod = Extension('bsf',
                    sources=['bsf_py.cpp', 'bsf-core/BSFCoreDll.cpp'],
                    language="c++",
                    extra_compile_args=['-fopenmp', '-std=c++11', '-mpopcnt', '-g'],
                    extra_link_args=["-fopenmp"],
                    include_dirs=[numpy.get_include()])

setup(name='bsf',
      version='1.0',
      description='BSF python wrapper',
      license='BSD 2',
      long_description=open('README.md').read(),
      ext_modules=[bsf_mod])
