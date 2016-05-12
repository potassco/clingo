#!/usr/bin/env python3

# small script that brings the includes in line with the bingo project

import glob
import os.path

includes = glob.glob("luasql/*.h") + glob.glob("luasql/*.h.in")
sources  = glob.glob("src/*.c") + glob.glob("src/*.cpp")
lua      = [ os.path.relpath(x, "../liblua") for x in glob.glob("../liblua/lua/*.h") ]

for x in includes + sources:
	content = open(x).read()
	for y in includes + lua:
		z = os.path.basename(y)
		content = content.replace('#include "{0}"'.format(z), '#include <{0}>'.format(y))
	content = content.replace("See Copyright Notice in license.html", "See Copyright Notice in COPYING")
	open(x, "w").write(content)

