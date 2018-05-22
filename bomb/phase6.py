import subprocess
import itertools


file_header = """Border relations with Canada have never been better.
1 2 4 8 16 32
7 327
0 0
ionefg
"""

inputs = [1,2,3,4,5,6]
permutations = list(itertools.permutations(inputs))

for p in permutations:
    with open('gen.file', 'w') as ff:
        ff.write(file_header)
        ff.write(' '.join([str(i) for i in p]))
        ff.write('\n')

    failed = False
    try:
        subprocess.run(['/home/shaurya/Development/csapp/bomb/bomb', 'gen.file'], check=True)
    except subprocess.CalledProcessError:
        failed = True

    if not failed:
        break




