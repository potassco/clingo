# Benchmarks + PGO

The run script provides a way to run benchmarks, collect profiling data for
PGO, and build an optimized clingo binary.

## Requirements

The script requires the benchmark tool which can be installed via pip:

```bash
pip install git+https://github.com/potassco/benchmark-tool.git
```

A [runlim] executable has to be compiled and put into the programs subfolder.

The benchmarks directory has to be filled with benchmark instances and the file
`runscripts/local.xml` be adjusted accordingly.

## Usage

```bash
# build clingo versions to benchmark
./run.sh prepare
# profile previously build instrumented clingo version
./run.sh profile
# build an optimized clingo version using collected profile data
./run.sh build
# benchmark the various clingo version
./run.sh benchmark
# evaluate benchmark results
./run.sh eval
# remove the output folder with benchmark results
./run.sh clean
```

[runlim]: https://github.com/arminbiere/runlim
