from scipy.stats import wasserstein_distance
from scipy.stats import entropy
from numpy import median
from numpy import average
from numpy import var

#read sender data (just load once)
fp_send = open("sen_exec.txt", "r")
send = []
done = 0
cur_bit = 0
while not done:
    cur_bit = int(fp_send.read(1))
    if cur_bit == 0:
        send.append(0)
    elif cur_bit == 1:
        send.append(1)
    else:
        done = 1

# read receiver data (will be used more than once)
def load_receiver_data(recv, length):
    # load bits until int 2 is read, length+10, because sometimes recv has more bits than send
    for x in range(0,length+10):
        cur_bit = int(fp_recv.read(1))
        if cur_bit == 0:
            recv[x] = 0
        elif cur_bit == 1:
            recv[x] = 1
        elif cur_bit == 2:
            return
        else:
            print('Unknown Symbol:',cur_bit)

fp_recv = open("rec_exec.txt", "r")
recv  = [0] * (len(send)+100) # longer than send, because sometimes recv has more bits
array_wsd = [0] * 20
array_acc = [0] * 20
array_err = [0] * 20
array_tp = [0] * 20
array_fp = [0] * 20
array_tn = [0] * 20
array_fn = [0] * 20
array_cap = [0] * 20


for y in range(0,20):
    print('Test:',y)
    load_receiver_data(recv, len(send))
    # calc confusionmatrix
    tp = 0  # true positives
    fp = 0  # false positives
    tn = 0  # true negatives
    fn = 0  # false negatives
    for x in range(0,len(send)):
        if send[x] == 1 and recv[x] == 1:
            tp += 1
        elif send[x] == 1 and recv[x] == 0:
            fp += 1
        elif send[x] == 0 and recv[x] == 0:
            tn += 1
        else:
            fn += 1
    acc = (tp + tn) / ( tp + fp + tn + fn)

    # calc channel capacity (shannon)
    cap = 1-entropy([acc,1-acc])

    # calc wasserstein distance
    wsd = wasserstein_distance(recv,send)
    print('Wassersteindistance:',wsd)
    array_wsd[y] = wsd
    print('Accuracy:',acc)
    array_acc[y] = acc
    print('Errorrate:',1-acc)
    array_err[y] = 1-acc
    print('TP = ', tp, ' FP = ', fp, ' TN = ', tn, ' FN = ', fn)
    array_tp[y] = tp
    array_fp[y] = fp
    array_tn[y] = tn
    array_fn[y] = fn
    print('Capacity = ', cap)
    array_cap[y] = cap

print('Median WSD:', median(array_wsd))
print('Median ACC:', median(array_acc))
print('Median ERR:', 1-median(array_acc))
print('Median TP:', median(array_tp))
print('Median FP:', median(array_fp))
print('Median TN:', median(array_tn))
print('Median FN:', median(array_fn))
print('Median CAP:', median(array_cap))

print('Average WSD:', average(array_wsd))
print('Average ACC:', average(array_acc))
print('Average ERR:', 1-average(array_acc))
print('Average TP:', average(array_tp))
print('Average FP:', average(array_fp))
print('Average TN:', average(array_tn))
print('Average FN:', average(array_fn))
print('Average CAP:', average(array_cap))

print('Variance WSD:', var(array_wsd))
print('Variance ACC:', var(array_acc))
print('Variance ERR:', var(array_err))
print('Variance TP:', var(array_tp))
print('Variance FP:', var(array_fp))
print('Variance TN:', var(array_tn))
print('Variance FN:', var(array_fn))
print('Variance CAP:', var(array_cap))