import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

fp = open("frtest.txt", "r")
uncached = []
cached = []
for x in range (0,9999):
    uncached.append(int(fp.readline()))
    cached.append(int(fp.readline()))

min = 10000

for x in range (0,9999):
    if(min>cached[x]&cached[x]>300):
        min=cached[x]

frequency_uc = [0] * 1000
frequency_c = [0] * 1000

for x in range(0,9999):
    frequency_uc[uncached[x]] += 1
    frequency_c[cached[x]] += 1

for x in range(0,1000):
    if(frequency_c[x]==0):
        frequency_c[x]=-1000

for x in range(0,1000):
    if(frequency_uc[x]==0):
        frequency_uc[x]=-1000

plt.plot(frequency_c, 'r^',frequency_uc, 'bo')
red_patch = mpatches.Patch(color='red', label='uncached reload')
blue_patch = mpatches.Patch(color='blue', label='cached reload')
plt.legend(handles=[red_patch,blue_patch])
plt.vlines(min, 0, 10000, linestyles="dashed", colors="k")

plt.suptitle('F+R Timings')
plt.axis([0,1000,0,10000])
plt.xlabel('CPU Cycles')
plt.show()



