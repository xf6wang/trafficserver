#import numpy as np
#import matplotlib.pyplot as plt

with open("test.txt") as f:
	content = f.readlines()

plugin_time = []
my_time = []
for x in content:
	if "latency:" in x:
		right = x.split("latency:", 1)[1]
		plugin_time.append(right.split(" ")[1])
		my_time.append(right.split(" ")[2])
res = []
with open('output.txt', 'a') as out_file:   
	for y in range(len(plugin_time)):
		diff = int(plugin_time[y]) - int(my_time[y])
		out_file.write(str(diff))
		out_file.write("\n")
