#!/bin/bash
#SBATCH --job-name=dispatch  # Job name
#SBATCH --output=%x-%j.out   # Standard output file
#SBATCH --error=%x-%j.err    # Standard error file
#SBATCH --time=24:00:00      # Maximum runtime (HH:MM:SS)
#SBATCH --partition=kr       # Partition or queue name
#SBATCH --cpus-per-task=8    # Number of CPU cores per task

source ~/.local/opt/conda/etc/profile.d/conda.sh
conda activate clang
case $1 in
clean)
	rm -rf output ../gcov-data
	;;
instrument-gcc)
	(cd ..; ./scripts/pgo.py --compiler gcc instrument)
	;;
instrument-clang)
	(cd ..; ./scripts/pgo.py --compiler clang instrument)
	;;
profile)
	./output/clingo-instrument/kr-node/start.py
	;;
build-gcc)
	(cd ..; ./scripts/pgo.py --compiler gcc build ./bench/output)
	;;
build-clang)
	(cd ..; ./scripts/pgo.py --compiler clang build ./bench/output)
	;;
*)
	echo "usage: dispatch.sh {{instrument|build}-{gcc|clang}|profile}"
	echo "error: unexpected argument $1"
	exit 1
	;;
esac
