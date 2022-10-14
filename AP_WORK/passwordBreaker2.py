from threading import Thread, Lock, Semaphore
import queue
import time
import pywifi
from pywifi import const
from asyncio.tasks import sleep
import itertools as its

q = queue.Queue(maxsize=0)
characters = list("0123")
ssid = ""  #wifi名
testPwd = ""  #测试密码
res = ""  #与OS课本知识不同，单单做定义，线程之间并不直接共享，而要在使用处显示声明
sema = Semaphore(0)
resSema = Semaphore(1)


# def Iterator(num):
#     iterator = its.product(characters, repeat=num)
#     with open('D:\ 1.txt', 'w') as f:
#         for i in iterator:
#             string = i[0] + i[1] + i[2] + i[3] + "\n"

#         #f.write(i[0]+i[1]+i[2]+i[3]+"\n")


def getWifiList():
    wifi = pywifi.PyWiFi()  # 抓取网卡接口
    ifaces = wifi.interfaces()[0]  # 抓取第一个无限网卡
    ifaces.disconnect()  # 测试链接断开所有链接
    time.sleep(1)
    result = ifaces.scan_results()  #扫描所有wifi
    for data in result:
        print("SSID: " + data.ssid)


def Connect(pwd):
    wifi = pywifi.PyWiFi()

    ifaces = wifi.interfaces()[0]  #获取第一个无线网卡
    ifaces.disconnect()  #断开所有连接
    time.sleep(1)
    profile = pywifi.Profile()  # 创建wifi链接文件
    profile.ssid = ssid  # wifi名称
    profile.auth = const.AUTH_ALG_OPEN  # 网卡的开放，
    profile.akm.append(const.AKM_TYPE_WPA2PSK)  # wifi加密算法
    profile.cipher = const.CIPHER_TYPE_CCMP  # 加密单元
    profile.key = pwd  # 密码
    tmp_profile = ifaces.add_network_profile(profile)  # 设定新的链接文件
    ifaces.connect(tmp_profile)  # 链接
    time.sleep(1)
    if ifaces.status() == const.IFACE_CONNECTED:  # 判断是否连接上
        resSema.acquire()  #保证只有一个信号量的“结果正确”状态可以上交
        isOK = True
        print("Password is " + pwd)
    else:
        isOK = False
        print("Invalid Testing Password: " + pwd)

    return isOK


class myThread(Thread):
    def __init__(self, threadID, name):
        Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.setDaemon(daemonic=True)  #子线程daemon属性为True，主线程运行结束时子线程将随主线程一起结束

    def run(self):
        print("Starting " + self.name)
        while True:
            if not q.empty():
                s = q.get()  #Queue自带锁，是线程安全的
                print(s)
                r = Connect(s)
                global res  #先声明为全局变量
                res = s  #后使用
                if (r):
                    #print("connection")
                    sema.release()
                    return
            else:
                print(self.name + "over!")
                return


thread_num = 10  #最优线程数为NCPU+1，故最优子线程数为NCPU

if __name__ == '__main__':
    #for i in range(1000000):
    getWifiList()
    ssid = input("Please input your SSID:\n")
    testPwd = input("Please input your password:\n")
    # iterator = its.product(characters, repeat=2)
    # for i in iterator:
    #     string = i[0] + i[1] + "\n"
    #     q.put(string)

    q.put(testPwd)
    time_start = time.time()
    for i in range(thread_num):
        t = myThread(i + 1, "Thread-" + str(i + 1))
        t.start()  #不能调用join阻塞主线程，因为主线程不必等待所有子线程完成
        # time.sleep(2)

    sema.acquire()  #信号量同步
    time_end = time.time()
    print("Password Found! It is " + res + "\n")
    print('Totally Time Cost', time_end - time_start)
