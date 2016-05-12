#!/bin/bash
set -eu
cd "$(dirname "$(readlink -f "$0")")"
config="${HOME}/.potassco_sourceforge_username"
echo -n "username: "
if [[ -e "$config" ]]
then
	config_username=$(cat "$config")
	echo -n "[$config_username] "
fi
read username
if [[ -n "$username" ]]
then
	echo "$username" > "$config"
else
	username="$config_username"
fi
version=$(grep GRINGO_VERSION libgringo/gringo/gringo.h | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
files=$(ls -d {lib{gringo,clasp,lua,luasql,program_opts},lemon,cmake,app,CMakeLists.txt,Makefile,README,INSTALL,CHANGES,COPYING})
cpus=$[$(cat /proc/cpuinfo| grep processor | wc -l)+1]
make -j${cpus} release  target=all
make -j${cpus} static32 target=all
make -j${cpus} mingw32  target=all
gringo_gen=$(ls build/static32/libgringo/src/{converter.cpp,converter_impl.cpp,converter_impl.h,parser.cpp,parser_impl.cpp,parser_impl.h})
rm -rf build/dist
mkdir -p build/dist
for x in gringo clingo iclingo; do
	rsync --delete --exclude ".svn" -a ${files} build/dist/${x}-${version}-source/
	sed "s/target=gringo/target=${x}/" < Makefile > build/dist/${x}-${version}-source/Makefile
	cp ${gringo_gen} build/dist/${x}-${version}-source/libgringo/src/
	(cd build/dist; tar -czf ${x}-${version}-source.tar.gz ${x}-${version}-source)
	mkdir -p build/dist/{${x}-${version}-win32,${x}-${version}-x86-linux}
	cp build/mingw32/bin/$x.exe CHANGES COPYING build/dist/${x}-${version}-win32
	cp build/static32/bin/$x CHANGES COPYING build/dist/${x}-${version}-x86-linux
	strip build/dist/${x}-${version}-x86-linux/$x
	i586-mingw32msvc-strip build/dist/${x}-${version}-win32/$x.exe
	(cd build/dist; tar -czf ${x}-${version}-x86-linux.tar.gz ${x}-${version}-x86-linux)
	(cd build/dist; zip -r  ${x}-${version}-win32.zip ${x}-${version}-win32)
	rm -rf build/dist/${x}-${version}-{win32,x86-linux,source}
	mkdir -p build/dist/${x}/${version}
	(cd build/dist; scp -r ${x} ${username},potassco@frs.sourceforge.net:/home/frs/project/p/po/potassco/)
	scp CHANGES build/dist/${x}-${version}{-win32.zip,-x86-linux.tar.gz,-source.tar.gz} rkaminski,potassco@frs.sourceforge.net:/home/frs/project/p/po/potassco/${x}/${version}/
done

