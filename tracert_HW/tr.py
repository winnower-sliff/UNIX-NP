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


def myTracert(name: str, whoami: str):
    # dstIP = '58.192.118.142'
    dstIP = socket.gethostbyname(name)
    print(f"\ntracerouting\t{name}:")

    nodeLink = [whoami]

    taps = 20
    # lastNode = set()

    icmpPkt = IP(dst=dstIP) / ICMP()
    for tap in range(taps):
        icmpPkt[IP].ttl = tap
        print(f"TTL: {tap}\t", end="")
        ans, unans = sr([icmpPkt] * 3, timeout=3,
                        verbose=0)  # verbose表示sr()是否展示详细信息
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
                    nodeLink += [name + "\n" + pkt[IP].src]
                    return nodeLink

        if (len(node) > 0):
            print(node)
            nodeLink += [node.pop()]

        else:
            print('*')
            # node.add('*')
            nodeLink += ['*']
            if (tap == taps - 1):
                print("Unreachable!!!")
                return nodeLink + [name + "\nunreachable"]

        # for n in node:
        #     G.add_node(n)
        #     for ltN in lastNode:
        #         G.add_edge(ltN, n)
        # lastNode = node


def main(hostnames, filename, whoami):
    for hostname in hostnames:
        nodeLink = myTracert(hostname, whoami=whoami)
        if (len(nodeLink) == 0):
            continue
        lastNode = nodeLink[0]
        G.add_node(lastNode)
        starNum = 0
        for nextNodeNo in range(1, len(nodeLink)):
            if (nodeLink[nextNodeNo] == '*'):
                starNum += 1
                continue
            G.add_edge(lastNode, nodeLink[nextNodeNo], score='*' * starNum)
            starNum = 0
            lastNode = nodeLink[nextNodeNo]

    nx.write_gpickle(G, filename)


hostnames_inside = [
    "www.seu.edu.cn",
    "ehall.seu.edu.cn",
    "jwc.seu.edu.cn",
    "newxk.urp.seu.edu.cn",
    "pub.seu.edu.cn",
    "dekt.seu.edu.cn",
    "lib.seu.edu.cn",
    "tj.seu.edu.cn",
    "phylab.seu.edu.cn",
    "cyber.seu.edu.cn",
    "nic.seu.edu.cn",
    "gsas.seu.edu.cn",
]
hostnames_outside = [
    "www.seu.edu.cn", "ehall.seu.edu.cn", "jwc.seu.edu.cn", "www.bilibili.com",
    "www.baidu.com", "8.8.8.8"
]
# hostnames = ["www.seu.edu.cn","ehall.seu.edu.cn", "jwc.seu.edu.cn",]
whoami = "suzhou"
for times in range(3):
    main(hostnames_inside, whoami + f'_inside{times}.gpickle', whoami)
    # main(hostnames_outside, whoami + f'_outside{times}.gpickle', whoami)
