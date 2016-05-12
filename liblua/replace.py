#!/usr/bin/env python3

# small script that brings the includes in line with the bingo project

import glob
import os.path

includes = glob.glob("lua/*.h")
sources  = glob.glob("src/*.c")

for x in includes + sources:
	content = open(x).read()
	for y in includes:
		z = os.path.basename(y)
		content = content.replace('#include "{0}"'.format(z), '#include <{0}>'.format(y))
	open(x, "w").write(content)

