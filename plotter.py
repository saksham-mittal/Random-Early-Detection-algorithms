import matplotlib.pyplot as plt


with open("log-red.txt", "r") as fp:
    lines = fp.readlines()

queue = []
avg=[]
for line in lines:
    queue_len, avg_len = line.split("\t")
    queue.append(float(queue_len.strip("\n")))
    avg.append(float(avg_len.strip("\n")))
print(avg)
print(queue)


plt.plot(range(len(queue)), queue,label="current queue length")
plt.plot(range(len(avg)), avg,label="Average queue len")
plt.legend()
plt.show()

