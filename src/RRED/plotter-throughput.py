import matplotlib.pyplot as plt
import sys
import glob
import numpy as np
from scipy.ndimage.filters import gaussian_filter

# plt.style.use('ggplot')

# Get all filepaths of senders
a = ["Host", "LDoS Attacker"]

def get_sender_filepaths():
    return glob.glob("samples/RRED/log/log-client/sent*.txt")

# Get all filepaths of receivers
def get_receiver_filepaths():
    return glob.glob("samples/RRED/log/log-server/re*.txt")

def get_traffic_level():
    with open("samples/RRED/log/log-{}.txt".format(1), "r") as fp:
        lines = fp.readlines()

        for count, line in enumerate(lines):
            if count == 0:
                traffic = line.strip("\n")
                break
    return traffic

def parse_sender_log(sender_filepath):
    sentTillNow = []
    with open(sender_filepath, "r") as fp:
        lines = fp.readlines()

    for line in lines:
        line = line.strip("\n")

    priority = int(lines[0])

    for line in lines[1:]:
        sentTillNow.append(int(line))

    return priority, np.array(sentTillNow)


def parse_receiver_log(receiver_filepath, simTime):
    recvdTillNow = {}
    current_index = 0
    with open(receiver_filepath, "r") as fp:
        lines = fp.readlines()

    for line in lines:
        line = line.strip("\n")

    priority = int(lines[0])

    for line in lines:
        if " " in line:
            priority, recvd = line.split(" ")
            priority, recvd = int(priority), int(recvd)
            if priority in recvdTillNow.keys():
                recvdTillNow[priority][current_index] = recvd
            else:
                recvdTillNow[priority] = np.zeros(simTime)
                recvdTillNow[priority][current_index] = recvd
        else:
            # for key in recvdTillNow.keys():
            #     for i in range(current_index, int(line)):
            #         recvdTillNow[key][i] = recvdTillNow[key][current_index]
            current_index = int(line)

    return recvdTillNow

def plot(send_dict, recv_dict, traffic_level):
    epsilon = 1e-6
    plt.figure(num=None, figsize=(12, 7), dpi=90, facecolor='w', edgecolor='k')
    plt.locator_params(axis='x', nbins=10)
    plt.xlabel("Simulation Time")
    plt.ylabel("PDR")
    plt.title("PDR vs Time for traffic level {}".format(traffic_level))
    plt.ylim((0, 1.3))

    for priority in recv_dict.keys():
        goodput_instantaneous = (recv_dict[priority][:-2] + epsilon) / \
            (send_dict[priority][:-2] + epsilon)

        goodput_instantaneous = gaussian_filter(
            goodput_instantaneous, sigma=0)

        plt.plot(range(goodput_instantaneous.shape[0]), goodput_instantaneous,
                 label="{}".format(a[priority]))

    for priority in recv_dict.keys():
        goodput = (np.cumsum(recv_dict[priority][:-2]) + epsilon) / \
            (np.cumsum(send_dict[priority][:-2]) + epsilon)

        plt.plot(range(goodput.shape[0]), goodput,
                 label="Cumulative: {}".format(a[priority]))

    plt.legend()
    plt.savefig("././samples/RRED/{}/plot-throughput.png".format(traffic_level),
                bbox_inches='tight')

def main():
    sent_dict = {}
    recv_dict = {}

    filepaths = get_sender_filepaths()
    for filepath in filepaths:
        priority, sentTillNow = parse_sender_log(filepath)
        if priority in sent_dict.keys():
            sent_dict[priority] += sentTillNow
        else:
            sent_dict[priority] = sentTillNow

    recvFilePaths = get_receiver_filepaths()
    for filepath in recvFilePaths:
        recvdTillNow = parse_receiver_log(filepath, sent_dict[0].shape[0])
        for priority in recvdTillNow.keys():
            if priority in recv_dict.keys():
                recv_dict[priority] += recvdTillNow[priority]
            else:
                recv_dict[priority] = recvdTillNow[priority]

    traffic_level = get_traffic_level()

    plot(sent_dict, recv_dict, traffic_level)

    print("Graph for PDR plotted succesfully")

if __name__ == "__main__":
    main()
