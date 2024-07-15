import json
import shlex

with open("compile_commands.json") as hnd:
    data = json.load(hnd)

for i, call in enumerate(data):
    if "command" in call:
        cmd = call["command"]
        prg = shlex.split(cmd)[0]
        if "clang++" in prg and cmd.endswith(".h"):
            call["command"] = f"{prg} -x c++-header{cmd[len(prg):]}"

with open("compile_commands.json", "w") as hnd:
    json.dump(data, hnd, indent=2)
