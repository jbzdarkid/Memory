from subprocess import run
from pathlib import Path
import sys

if __name__ == '__main__':
    Path('_build').mkdir(exist_ok=True)
    if '--clean' in sys.argv[1:]:
      Path('_build/CMakeCache.txt').unlink(missing_ok=True)

    build_args = ['cmake', '..']
    if '--debug' in sys.argv[1:]:
        build_args.append('-DCMAKE_BUILD_TYPE=Debug')
    run(build_args, cwd='_build')

    make_args = ['cmake', '--build', '_build', '--config', 'Debug']
    run(make_args)

    install_args = ['cmake', '--install', '_build', '--prefix', '_install', '--config', 'Debug']
    run(install_args)
