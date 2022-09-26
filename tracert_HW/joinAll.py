#!/usr/bin/python3
# import multiprocessing
# from tabnanny import verbose
# from time import sleep
from scapy.all import *
import networkx as nx
import matplotlib.pyplot as plt

G_list = []
whoami='suzhou'
for times in range(3):
    A = nx.read_gpickle(f"{whoami}_inside{times}.gpickle")
    G_list += [A]
    # B = nx.read_gpickle(f"{whoami}_outside{times}.gpickle")
    # G_list += [B]
G = nx.compose_all(G_list)
pos = nx.spring_layout(G, scale=10)

nx.draw(
    G,
    pos=pos,
    with_labels=True,
    node_color='y',
    font_size=6,
    # node_size=800
)
edge_labels = nx.get_edge_attributes(G, 'score')
nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels)
# nx.draw_networkx_labels(G,pos,alpha=0.5)

plt.show()
