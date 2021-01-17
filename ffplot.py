import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import scipy.stats as stats

# write ff values into arrays cached, uncached (hit,miss)
fp = open("fftest.txt", "r")
# arrays to store hit and miss timings (will later be updated)
uncached = []
cached = []
# arrays to store hit and miss timings (will not be edited in the future)
true_uncached = []
true_cached = []
for x in range (0,1000):
    uncached.append(int(fp.readline()))
    cached.append(int(fp.readline()))
true_uncached = uncached.copy()
true_cached = cached.copy()

# calc average
avg_uc = 0
avg_c = 0
for x in range(0,1000):
    avg_uc += uncached[x]
    avg_c += cached[x]

avg_uc = avg_uc/1000
avg_c = avg_c/1000

# calc median
med_uc = np.median(uncached)
med_c = np.median (cached)

# calc standard deviation
std_uc = np.std(uncached)
std_c = np.std(cached)

# discard standard_deviation*10 or higher from uncached
for x in range (0, max(uncached) + 1):
    if (x < avg_uc - std_uc*10) or (x > avg_uc + std_uc*10):
        # discard from uncached
        for y in range(0, len(uncached)):
            if true_uncached[y] == x:
                uncached.pop(y)

# discard standard_deviation*10 or higher from cached
for x in range(0, max(cached) + 1):
    if (x < avg_c - std_c*10) or (x > avg_c + std_c*10):
        # discard from cached
        for y in range(0, len(cached)):
            if true_cached[y] == x:
                cached.pop(y)

# https://stackoverflow.com/questions/22579434/python-finding-the-intersection-point-of-two-gaussian-curves
def solve(m1, m2, std1, std2):
    a = 1 / (2 * std1 ** 2) - 1 / (2 * std2 ** 2)
    b = m2 / (std2 ** 2) - m1 / (std1 ** 2)
    c = m1 ** 2 / (2 * std1 ** 2) - m2 ** 2 / (2 * std2 ** 2) - np.log(std2 / std1)
    return np.roots([a, b, c])

# plot
plt.hist(true_uncached, density=True, bins = 20, align="mid", alpha=0.6, color='b', label="'0'")
plt.hist(true_cached, density=True, bins = 20, align="mid", histtype="barstacked", alpha=0.6, color='r', label="'1'")
# legend
blue_patch = mpatches.Patch(color='blue', label='Misses')
red_patch = mpatches.Patch(color='red', label='Hits')
plt.legend(handles=[red_patch,blue_patch])

# set viewable frame
xmax = max(max(uncached), max(cached))
xmin = min(min(uncached), min(cached))
plt.xlim(xmin - 5, xmax + 5)
# draw gaussian function for hits
x = np.linspace(xmin, xmax, 100)
plt.plot(x, stats.norm.pdf(x, avg_c, std_c), color='k')
# draw gaussian function for misses
x = np.linspace(avg_uc- 10*std_uc, avg_uc + 10*std_uc, 1000)
plt.plot(x, stats.norm.pdf(x, avg_uc, std_uc), color='k')

# calc threshold
threshold = solve(avg_uc, avg_c, std_uc, std_c)
# solve gives out multiple intersections
# calc which intersection is the best threshold by comparing the accuracy
# correct counts the correct classified bits
correct = [0] * len(threshold)
for x in range(0,len(threshold)):
    for y in range(0, len(uncached)):
        if true_uncached[y] < threshold[x] :
            correct[x] += 1

for x in range(0,len(threshold)):
    for y in range(0, len(cached)):
        if true_cached[y] > threshold[x] :
            correct[x] += 1

best_threshold = 0
max_correct = 0 # highest correct classified  bits value stored here
for x in range(0,len(correct)):
    if max_correct < correct[x]:
        max_correct = correct[x]
        best_threshold = threshold[x]

# round threshold
best_threshold = round(best_threshold, 0)
# draw threshold
plt.vlines(best_threshold, 0, 0.2, linestyles="dashed", colors="k")
# calc accuracy
accuracy = max_correct/20000

print('Accuracy:', accuracy)
print('Best Threshold', int(best_threshold))

plt.suptitle('F+F Timings')
plt.xlabel('CPU Cycles')
plt.ylabel('Frequency')
plt.show()



