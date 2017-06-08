from distutils.core import setup, Extension
import numpy
import sys
import os

if sys.platform == 'darwin':
    from distutils import sysconfig
    vars = sysconfig.get_config_vars()
    vars['LDSHARED'] = vars['LDSHARED'].replace('-bundle', '-dynamiclib')
    os.environ["CC"] = "/usr/local/bin/clang-omp++"
    os.environ['CXX'] = "/usr/local/bin/clang-omp++"
else:
    os.environ["CC"] = "g++"
    os.environ['CXX'] = "g++"
bsf_mod = Extension('bsf', sources=['bsf_py.cpp', 'bsf-core/BSFCoreDll.cpp'],
                    language="c++",
                    extra_compile_args=['-fopenmp', '-std=c++11', '-mpopcnt', '-g'],
                    extra_link_args=["-fopenmp"],
                    include_dirs=[numpy.get_include()]
                    )

setup(name='bsf',
      version='1.0',
      description='a bsf python wrapper',
      ext_modules=[bsf_mod]
      )
