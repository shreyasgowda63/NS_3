import os
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D


def read_file(file_path):
    with open(file_path) as f:
        data = f.readlines()
    f.close()
    return data


def plotfig(X, Y, Z):
    fig = plt.figure()
    ax = Axes3D(fig)
    ax.scatter3D(X, Y, Z, cmap='blue')
    ax.plot3D(X, Y, Z, 'green')
    plt.show()


def plotgif(X, Y, Z):
    fig = plt.figure()
    ax = Axes3D(fig)
    plt.ion()
    ax.set_xlim(-250,250)
    ax.set_ylim(-250,250)
    ax.set_zlim(0,200)
    for i in range(0, len(X)):
        ax.scatter3D(X[i], Y[i], Z[i], cmap='blue')
        ax.plot3D(X[:i], Y[:i], Z[:i], 'green')
        plt.pause(0.001)
    plt.show()


def plotgif2(X, Y, Z):
    fig = plt.figure()
    ax = Axes3D(fig)
    plt.ion()
    ax.set_xlim(-250,250)
    ax.set_ylim(-250,250)
    ax.set_zlim(0,200)
    for j in range(0, int(len(X)/50)):   
        plt.cla()
        ax.set_xlim(-250,250)
        ax.set_ylim(-250,250)
        ax.set_zlim(0,200)
        for i in range(j*50, (j+1)*50):
            ax.scatter3D(X[i], Y[i], Z[i], 'blue')
            plt.pause(0.001)
    plt.show()


if __name__ == "__main__":
    file_path = r"./tarballs/ns-allinone-3.29/ns-3.29/SRCM.mob"
    data = read_file(file_path)
    X = []
    Y = []
    Z = []
    for i in range(len(data)):
        str = data[i]
        s = str.index("pos=") + 4
        e = str.index("vel=") - 1
        XYZ_list = str[s:e].split(":")
        X.append(float(XYZ_list[0]))
        Y.append(float(XYZ_list[1]))
        Z.append(float(XYZ_list[2]))

    X = np.asarray(X)
    Y = np.asarray(Y)
    Z = np.asarray(Z)

    plotfig(X, Y, Z)

    #plotgif(X, Y, Z)

    #plotgif2(X, Y, Z)
