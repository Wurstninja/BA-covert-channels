from scipy.stats import wasserstein_distance
from scipy.stats import entropy

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
for y in range(0,8):
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
    print('Accuracy:',acc)
    print('Errorrate:',1-acc)
    print('TP = ', tp, ' FP = ', fp, ' TN = ', tn, ' FN = ', fn)
    print('Capacity = ', cap)