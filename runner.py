import numpy as np
import subprocess

# arr = np.arange(64).reshape(8, 8)
# print(arr.tobytes())

RunCprogram = "mpirun ./conv"
n = int(input("Enter Array Size: "))

p = subprocess.Popen(RunCprogram,shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
stdout, err = p.communicate(input=str(n).encode())
result=stdout.splitlines()

a = []
for i in result:
    a.append(float(i.decode('utf-8')))

print(a)
time_elaspsed = a[-1]
conv = np.asarray(a[:-1])
conv.shape = (n,n)

print(f"TIME: {time_elaspsed}")
print(conv.shape)

