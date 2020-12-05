import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

fp = open("fftest.txt", "r")
uncached = []
cached = []
for x in range (0,9999):
    uncached.append(int(fp.readline()))
    cached.append(int(fp.readline()))

sorted(cached)
median_c = cached[499]
sorted(uncached)
median_uc = uncached[499]
min = 1000

for x in range(0,499):
    if(cached[x]<min):
        if(cached[x]>median_c - (median_c - median_uc)*0.3):
            min = cached[x]

frequency_uc = [0] * 1000
frequency_c = [0] * 1000

for x in range(0,9999):
    frequency_uc[uncached[x]] += 1
    frequency_c[cached[x]] += 1

for x in range(0,499):
    if(frequency_c[x]==0):
        frequency_c[x]=-1000

for x in range(0,499):
    if(frequency_uc[x]==0):
        frequency_uc[x]=-1000


plt.plot(frequency_c, 'ro', frequency_uc, 'bo')
red_patch = mpatches.Patch(color='red', label='cached flush')
blue_patch = mpatches.Patch(color='blue', label='uncached flush')
plt.legend(handles=[red_patch,blue_patch])
plt.vlines(min-1, 0, 10000, linestyles="dashed", colors="k")

plt.suptitle('F+F Timings')
plt.axis([150,400,0,10000])
plt.xlabel('CPU Cycles')
plt.show()


