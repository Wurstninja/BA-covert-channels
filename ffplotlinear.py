import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

fp = open("ffexec.txt", "r")
timings = []
for x in range (0,999):
    timings.append(int(fp.readline()))

plt.plot(timings, 'ro')

plt.suptitle('F+F Execution Timings')
plt.axis([0,1000,200,500])
plt.ylabel('CPU Cycles')
plt.show()


