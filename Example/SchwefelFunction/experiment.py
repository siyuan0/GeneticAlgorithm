import subprocess
import os
import matplotlib.pyplot as plt
import numpy as np

DIMENSIONS = 6
# 2d optimal
# x_true = [420.966, 420.982]
# f_true = -837.966
# 6d optimal
x_true = [420.973, 420.996, 420.967, 420.976, 420.95, 420.97] 
f_true = -2513.9
repeat = 1000
l2_limit = 10


failed_runs = 0

bins = np.linspace(-2600, -1500, 100)
parameter_paths = [
    ["S=1", "Example/SchwefelFunction/parameters1.json"],
    ["S=2", "Example/SchwefelFunction/parameters2.json"],
    ["S=5", "Example/SchwefelFunction/parameters3.json"],
    ["S=10", "Example/SchwefelFunction/parameters4.json"],
    ["S=20", "Example/SchwefelFunction/parameters5.json"],
    ["S=50", "Example/SchwefelFunction/parameters6.json"],
]

for desc, parameter_path in parameter_paths:
    f_list = []
    numSuccess = 0
    for i in range(repeat):
        print(str(i), "/", repeat, end='\r', flush=True)
        # run the GA algor
        res = subprocess.run(["Release/GA_run", parameter_path], cwd=os.getcwd(), stdout=subprocess.PIPE)
        s = str(res.stdout)
        start = s.find("x: [") + 4
        end = s.find("]", start)

        try:
            x = [float(xi) for xi in s[start:end].split(", ")]
            f = float(s[s.find("f: ")+3:s.find("\n", start)-2])
            #  # find the true best
            # if f < f_true:
            #     f_true = f
            #     x_true = x
            # find probability of success
            # l2 = 0
            # for i in range(DIMENSIONS): l2 += (x[i]-x_true[i])**2
            # if(l2**(0.5) < l2_limit): numSuccess += 1
            f_list.append(f)
        except:
            failed_runs += 1
        
    # print(x_true, f_true)
    print("prob success:", numSuccess/repeat)
    plt.hist(f_list, bins, label=desc, alpha=0.5)

print(failed_runs)
plt.xlabel("optimisation result")
plt.ylabel(str("number of instances out of " + str(repeat) + " repeats"))
plt.legend()
plt.savefig("outcome_dist.png")
