import re
import sys

import yaml
from clingo.ast import _type_info_yaml

yaml = yaml.safe_dump(yaml.safe_load(_type_info_yaml()), sort_keys=False)
with open("lib/c-api/src/ast.cc", "r") as file:
    content = re.sub(r'R"yaml\(.*\)yaml"', f'R"yaml({yaml})yaml"', file.read(), 1, re.S)

with open("lib/c-api/src/ast.cc", "w") as file:
    file.write(content)
