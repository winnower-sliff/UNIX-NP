#!/usr/bin/python3
import multiprocessing
from tabnanny import verbose
from time import sleep
from scapy.all import *
dstIP='36.152.44.96'
a = IP(dst=dstIP) / ICMP()
for i in range(16):
    a[IP].ttl = i
    print(f"TTL: {i}\t", end="")
    ans, unans = sr([a] * 3, timeout=3, verbose=0)  # verbose表示sr()是否展示详细信息
    nodes = set()
    for i in ans:
        for j in i:

            if (j[ICMP].type == 11):
                nodes.add(j[IP].src)
            if (j[ICMP].type == 0):
                print(f"Reach!!! {dstIP}")
                exit(0)

    if(len(nodes)>0):
        print(nodes)
    else:
        print('*')
