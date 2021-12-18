import numpy as np
import subprocess
import matplotlib as mpl
import matplotlib.pyplot as plt
from datetime import datetime
import os

# arr = np.arange(64).reshape(8, 8)
# print(arr.tobytes())

def run_c_routine(n):
    RunCprogram = "mpirun -np 1 ./conv"

    p = subprocess.Popen(RunCprogram,shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    stdout, err = p.communicate(input=str(n).encode())
    result=stdout.splitlines()

    return result


def test():
    n = [i for i in range(10, 410, 10)]
    res_x = []
    res_y = []
    for j in n:
        a = []
        result = run_c_routine(j)
        for i in result:
            a.append(float(i.decode('utf-8')))
        time_elaspsed = (a[-2],a[-1])
        print("Input: ", j, " done")
        res_x.append(j)
        res_y.append(time_elaspsed[0])

    return res_x, res_y

def viz_bar(x, y):
    fig, ax = plt.subplots(figsize=(10, 10), layout='constrained')
    ax.bar(x, y)
    plt.xlabel('Input Size')
    plt.ylabel('Time (s)')
    plt.title("Input Size vs Time(s) Bar Plot")
    t = datetime.now().strftime("%m_%d_%Y__%H_%M_%S")
    plt.savefig(os.path.join("viz",f'vis_bar_{t}.png'))

def viz_line(x, y):
    fig, ax = plt.subplots(figsize=(10, 10), layout='constrained')
    ax.plot(x, y)
    plt.xlabel('Input Size')
    plt.ylabel('Time (s)')
    plt.title("Input Size vs Time(s) Line Plot")
    t = datetime.now().strftime("%m_%d_%Y__%H_%M_%S")
    plt.savefig(os.path.join("viz",f'vis_line_{t}.png'))


ch = int(input("Test or Manuel Input: "))

if ch > 0:
    n = int(input("Enter Array Size: "))
    result = run_c_routine(n)
    a = []
    for i in result:
        a.append(float(i.decode('utf-8')))

    print(a)
    time_elaspsed = (a[-2],a[-1])
    conv = np.asarray(a[:-2])
    conv.shape = (n,n)

    print(f"TIME: {time_elaspsed}")
    print(conv.shape)

else: 
    x,y = test()
    viz_bar(x,y)
    viz_line(x,y)