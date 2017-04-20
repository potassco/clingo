set -ex

cd "$(dirname $0)/.."

make gringo

mkdir -p libgringo/gen/src/
rsync -ra build/debug/libgringo/src/input libgringo/gen/src/
