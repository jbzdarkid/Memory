from subprocess import run
from pathlib import Path
import sys

if __name__ == '__main__':
    Path('_build').mkdir(exist_ok=True)
    # Path('_build/CMakeCache.txt').unlink(missing_ok=True)

    args = ['cmake', '..']
    if '--debug' in sys.argv[1:]:
        args.append('-DCMAKE_BUILD_TYPE=Debug')
    run(args, cwd='_build')
    run(['cmake', '--build', '.', '--config', 'Debug'], cwd='_build')
