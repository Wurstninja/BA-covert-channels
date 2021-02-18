# BA-covert-channels
This is my repository for the bachelor exam "Verdeckte Kommunikation durch Seitenkanaleffekte auf ARM-Prozessoren".

## Preparations on Arch

```bash
sudo pacman -Syu
sudo pacman -Syu gcc make python python-pip 
pip install numpy matplotlib scipy
```

## Preparations on Ubuntu

```bash
sudo apt update
sudo apt install gcc make python3 python3-pip
pip3 install numpy matplotlib scipy
```

## Compiling

```bash
make
```

## Execution

### Same-Core
Terminal 1:
```bash
taskset 0x1 make runreceiver "ARG1=[FR/FF]" "ARG2=[interval in ns]" "ARG3=SC"
```
Terminal 2:
```bash
taskset 0x1 make runsender "ARG1=[FR/FF]" "ARG2=[interval in ns]"
```

### Cross-Core

Terminal 1:
```bash
taskset 0x1 make runreceiver "ARG1=[FR/FF]" "ARG2=[interval in ns]" "ARG3=CC"
```
Terminal 2:
```bash
taskset 0x4 make runsender "ARG1=[FR/FF]" "ARG2=[interval in ns]"
```

## How to use

Receiver is executed in Terminal 1 and sender is executed in Terminal 2. The Receiver will calculate the recommended threshold in CPU cycles. When the calculation is over, the receiver expects a threshold in CPU cycles via user input.

Terminal 1:
```bash
threshold: [user input]
```

When the user inserts a threshold, the receiver listens on the cache and waits for the sender to send the first ethernet frame.

After execution of the sender, the sender is ready to send ethernet frames via the given method specified in ARG1. The sender expects an input string for each ethernet frame, that will be sent to the receiver. When the input is longer than 1500 characters, only the first 1500 will be sent.

Terminal 2:
```bash
Message:
[user input]
```

In Terminal 1 the received string will be displayed along with other informations about the ethernet frame. During one execution multipli frames can be transfered between sender and receiver.

## Verified method

The implemented method has been tested with a cache emulator. For the implementation of side channel attacks, it is recommended to test it with a cache emulator first, to verify the correctness of the method.
The used cache emulator can be found here: https://git.cs.uni-bonn.de/boes/vlsg_sidechannel_proto
