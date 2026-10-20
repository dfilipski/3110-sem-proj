"""Run from repository root: python3 report/scripts/collect.py."""
from pathlib import Path
import csv, hashlib, json, platform, random, re, statistics, subprocess, tempfile
root = Path(__file__).resolve().parents[2]
subprocess.run(['make', 'build/benchmark'], cwd=root, check=True)
subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-O2', '-Isrc', 'report/scripts/verify.cpp', *[str(p) for p in (root/'build/benchmark_objects').glob('*.o') if p.name != 'benchmark.o'], '-o', 'build/report_verify'], cwd=root, check=True)
movies = (root/'data/movies.dat').read_bytes().splitlines(keepends=True)
random.Random(20261020).shuffle(movies)
ratings = (root/'data/ratings.dat').read_bytes().splitlines(keepends=True)
implementations = ['MovieDatabase', 'MovieDatabaseMap', 'MovieDatabaseTreap']
rows, sizes = [], []
pattern = re.compile(r'^(\w+): (\d+) calls, ([\d.]+) ms total, ([\d.]+) ns/call, checksum=(\d+)$', re.M)
with tempfile.TemporaryDirectory(prefix='movie-report-') as temp:
 for n in [500, 2000, 3883]:
  folder = Path(temp)/str(n); folder.mkdir()
  selected = sorted(movies[:n], key=lambda line:int(line.split(b'::')[0]))
  ids = {int(line.split(b'::')[0]) for line in selected}
  subset = [line for line in ratings if int(line.split(b'::')[1]) in ids]
  (folder/'movies.dat').write_bytes(b''.join(selected))
  (folder/'ratings.dat').write_bytes(b''.join(subset))
  (folder/'users.dat').write_bytes((root/'data/users.dat').read_bytes())
  sizes.append({'movies':n,'ratings':len(subset),'users':6040,'id_hits_per_round':n,'id_misses_per_round':3954-n})
  verified = subprocess.run([str(root/'build/report_verify'), str(folder)], capture_output=True, text=True, check=True)
  (root/f'report/raw/verification-{n}.txt').write_text(verified.stdout)
  expected = {}
  for trial in range(1,6):
   order = implementations[trial%3:] + implementations[:trial%3]
   for impl in order:
    result = subprocess.run([str(root/'build/benchmark'),impl,'100',str(folder)],capture_output=True,text=True,check=True)
    (root/f'report/raw/n{n}-trial{trial}-{impl}.txt').write_text(result.stdout.replace(str(folder),f'<fixture-{n}>'))
    matches = pattern.findall(result.stdout)
    assert len(matches)==4
    for operation,calls,ms,ns,checksum in matches:
     if operation in expected: assert expected[operation]==checksum, (n,operation,impl)
     expected[operation]=checksum
     rows.append(dict(movies=n,trial=trial,implementation=impl,operation=operation,calls=int(calls),total_ms=float(ms),ns_per_call=float(ns),checksum=checksum))
   print(f'Completed n={n}, trial={trial}',flush=True)
with (root/'report/raw/measurements.csv').open('w',newline='') as f:
 w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
summary=[]
for n in [500,2000,3883]:
 for impl in implementations:
  for op in sorted({r['operation'] for r in rows}):
   values=[r['ns_per_call'] for r in rows if r['movies']==n and r['implementation']==impl and r['operation']==op]
   summary.append(dict(movies=n,implementation=impl,operation=op,median_ns=statistics.median(values),min_ns=min(values),max_ns=max(values)))
(root/'report/raw/summary.json').write_text(json.dumps(summary,indent=2))
metadata={'seed':20261020,'rounds_per_trial':100,'trials':5,'sizes':sizes,'platform':platform.platform(),'compiler':subprocess.check_output(['clang++','--version'],text=True),'flags':'-std=c++17 -Wall -Wextra -O2','sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'data').glob('*.dat')}}
(root/'report/raw/environment.json').write_text(json.dumps(metadata,indent=2))
