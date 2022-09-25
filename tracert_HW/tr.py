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


def myTracert(name: str):
    # dstIP = '58.192.118.142'
    dstIP = socket.gethostbyname(name)
    print(f"\ntracerouting\t{name}:")

    nodeLink = []

    taps = 20
    # lastNode = set()

    icmpPkt = IP(dst=dstIP) / ICMP()
    for tap in range(taps):
        icmpPkt[IP].ttl = tap
        print(f"TTL: {tap}\t", end="")
        ans, unans = sr([icmpPkt] * 3, timeout=3, verbose=0)  # verbose表示sr()是否展示详细信息
        node = set()
        for pkts_snd_rcv in ans:
            for pkt in pkts_snd_rcv:
                if (pkt[ICMP].type == 11):
                    node.add(pkt[IP].src)
                if (pkt[ICMP].type == 0):
                    print(f"Reach!!! {dstIP}")
                    # node.add(name)
                    # for n in node:
                    #     G.add_node(n)
                    #     for ltN in lastNode:
                    #         G.add_edge(ltN, n)
                    nodeLink += [name]
                    return nodeLink

        if (len(node) > 0):
            print(node)
            nodeLink += [node.pop()]

        else:
            print('*')
            # node.add('*')
            if (tap == taps - 1):
                print("Unreachable!!!")
                return []

        
        # for n in node:
        #     G.add_node(n)
        #     for ltN in lastNode:
        #         G.add_edge(ltN, n)
        # lastNode = node


hostnames = [
    "www.seu.edu.cn", "ehall.seu.edu.cn", "jwc.seu.edu.cn",
    "newxk.urp.seu.edu.cn", "seu.olab.top", "pub.seu.edu.cn",
    "dekt.seu.edu.cn", "lib.seu.edu.cn", "tj.seu.edu.cn", "phylab.seu.edu.cn",
    "cyber.seu.edu.cn", "nic.seu.edu.cn", "gsas.seu.edu.cn"
]
for hostname in hostnames:
    nodeLink = myTracert(hostname)
    lastNode = "host"
    G.add_node(lastNode)
    for nextNode in nodeLink:
        G.add_edge(lastNode, nextNode)
        lastNode = nextNode

nx.draw(
    G,
    with_labels=True,
    node_color='y',
)
plt.show()