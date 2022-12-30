import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from matplotlib.ticker import LinearLocator

def SchwefelFunct(x):
    return np.sum(-x * np.sin(np.sqrt(np.abs(x))))

lbound = -500
ubound = 500

x1 = np.linspace(lbound, ubound, 1000)
x2 = np.linspace(lbound, ubound, 1000)
X1, X2 = np.meshgrid(x1, x2)
f = np.array([SchwefelFunct(x) for x in np.concatenate((X1.reshape(-1, 1), X2.reshape(-1,1)), axis=1)])

def schwefel2D():
    # display 2-D schwefel function
    fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
    # Plot the surface.
    surf = ax.plot_surface(X1, X2, f.reshape(X1.shape), cmap=cm.coolwarm,
                        linewidth=0, antialiased=False)
    ax.set_xlabel("x1")
    ax.set_ylabel("x2")
    ax.set_zlabel("f(x1, x2)")
    # Add a color bar which maps values to colors.
    fig.colorbar(surf, shrink=0.7, aspect=10)
    plt.savefig("result.png")

def visualisePopulation(path, savepath):
    data = np.loadtxt(path, delimiter=',')
    cs = plt.contour(X1, X2, f.reshape(X1.shape), 10)
    plt.colorbar(cs, shrink=0.5, aspect=10)
    indexes = [(0, 26), (26, 52), (52, 78), (78, 100)]
    for i in range(len(indexes)):
        s, e = indexes[i]
        label = "subset " + str(i)
        plt.scatter(data[s:e,0], data[s:e,1], marker='x', label=label)
    plt.legend(bbox_to_anchor=(1, 1.05))
    plt.savefig(savepath)
    plt.close()
    print("saved figure at: ", savepath)

def bestSoln(path):
    data = np.loadtxt(path, delimiter=',')
    print(data[np.argmin(data[:,2])])

# schwefel2D()
visualisePopulation("Release/populationInitial.txt", "2d_initial.png")
for i in [20, 40, 80, 60, 100, 220]:
    datapath = "Release/Results/iter" + str(i) + ".txt"
    outpath = "2d_iter" + str(i) + ".png"
    visualisePopulation(datapath, outpath)
visualisePopulation("Release/populationEnd.txt", "2d_End.png")

bestSoln("Release/populationEnd.txt")
