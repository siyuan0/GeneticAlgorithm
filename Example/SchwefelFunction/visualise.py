import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from matplotlib.ticker import LinearLocator

def SchwefelFunct(x):
    return np.sum(-x * np.sin(np.sqrt(np.abs(x))))

# display 2-D schwefel function
lbound = -500
ubound = 500

x1 = np.linspace(lbound, ubound, 1000)
x2 = np.linspace(lbound, ubound, 1000)
X1, X2 = np.meshgrid(x1, x2)
# coords = np.concatenate((coords[0].reshape(-1,1), coords[1].reshape(-1,1)), axis=1)
f = np.array([SchwefelFunct(x) for x in np.concatenate((X1.reshape(-1, 1), X2.reshape(-1,1)), axis=1)])
print(f.shape)
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
