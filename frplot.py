import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

fp = open("frtest.txt", "r")
uncached = []
cached = []
for x in range (0,9999):
    cached.append(int(fp.readline()))
    uncached.append(int(fp.readline()))

sorted(cached)
median_c = cached[499]
sorted(uncached)
median_uc = uncached[499]
min = 1000

for x in range(0,499):
    if(uncached[x]<min):
        print(median_c)
        print(median_uc)
        if(uncached[x]>median_uc - (median_uc - median_c)*0.3):
            min = uncached[x]

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

plt.plot(frequency_uc, 'r^',frequency_c, 'bo')
red_patch = mpatches.Patch(color='red', label='uncached reload')
blue_patch = mpatches.Patch(color='blue', label='cached reload')
plt.legend(handles=[red_patch,blue_patch])
plt.vlines(min-1, 0, 10000, linestyles="dashed", colors="k")

plt.suptitle('F+R Timings')
plt.axis([0,1000,0,10000])
plt.xlabel('CPU Cycles')
plt.show()



