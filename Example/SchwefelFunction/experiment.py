import subprocess
import os

print(os.getcwd())

res = subprocess.run(["Release/GA_run", "Example/SchwefelFunction/parameters.json"], cwd=os.getcwd())
s = str(res)
