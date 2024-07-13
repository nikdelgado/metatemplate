
import subprocess

def _has_git():
    return subprocess.run(['which','git'], stdout=subprocess.PIPE).returncode == 0

def repo_path():
    if _has_git():
        path = subprocess.run(['git','remote','-v'], cwd='.', stdout=subprocess.PIPE, encoding='utf-8').stdout or ''
        rows = path.split('\n')
        for row in rows:
            if 'ssh' in row or 'http' in row:
                url = row.split()[1].split('/')[3:]
                if url[0] in [ 'aero']:
                    url.pop(0)
                url[-1] = url[-1].split('.')[0]
                return url
    else:
        print('WARNING: Unable to check destination git status (`which git` failed), continuing anyway...')