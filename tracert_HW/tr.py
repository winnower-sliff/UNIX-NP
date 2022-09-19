#!/usr/bin/python3
# import multiprocessing
# from tabnanny import verbose
# from time import sleep
from scapy.all import *
import networkx as nx
import matplotlib.pyplot as plt

G = nx.Graph()
# nodes = ['A', 'B', 'C', 'D', 'E', 'F', 'G']

# for node in nodes:
#     G.add_node(node)


def tra(name: str):
    # dstIP = '58.192.118.142'
    dstIP = socket.gethostbyname(name)

    taps = 20
    lastNode = set(["host"])

    a = IP(dst=dstIP) / ICMP()
    for tap in range(taps):
        a[IP].ttl = tap
        print(f"TTL: {tap}\t", end="")
        ans, unans = sr([a] * 3, timeout=3, verbose=0)  # verbose表示sr()是否展示详细信息
        node = set()
        for pkts_snd_rcv in ans:
            for pkt in pkts_snd_rcv:
                if (pkt[ICMP].type == 11):
                    node.add(pkt[IP].src)
                if (pkt[ICMP].type == 0):
                    print(f"Reach!!! {dstIP}")
                    node.add(name)
                    for n in node:
                        G.add_node(n)
                        for ltN in lastNode:
                            G.add_edge(ltN, n)
                    return

        if (len(node) > 0):
            print(node)
        else:
            print('*')
            node.add('*')
            if (tap == taps - 1):
                print("Unreachable!!!")
                return

        for n in node:
            G.add_node(n)
            for ltN in lastNode:
                G.add_edge(ltN, n)
        lastNode = node

hostnames=["www.seu.edu.cn","ehall.seu.edu.cn","jwc.seu.edu.cn","newxk.urp.seu.edu.cn","seu.olab.top","pub.seu.edu.cn","dekt.seu.edu.cn","lib.seu.edu.cn","tj.seu.edu.cn","phylab.seu.edu.cn","cyber.seu.edu.cn","nic.seu.edu.cn","gsas.seu.edu.cn"]
for hostname in hostnames:
    tra(hostname)

nx.draw(
    G,
    with_labels=True,
    node_color='y',
)
plt.show()