from threading import Thread,Lock,Semaphore
import queue
import time

q=queue.Queue(maxsize=0)
res="" #与OS课本知识不同，单单做定义，线程之间并不直接共享，而要在使用处显示声明
sema=Semaphore(0)

from threading import Thread
class myThread(Thread):
    def __init__(self, threadID, name):
        Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.setDaemon(daemonic=True) #子线程daemon属性为True，主线程运行结束时子线程将随主线程一起结束
    
    
    def run(self):
        print("Starting "+self.name)
        while True:
            if not q.empty():
                s=q.get() #Queue自带锁，是线程安全的
            if(s=="999999"):
                global res #先声明为全局变量
                res=s #后使用
                sema.release()
                return

thread_num=9 #最优线程数为NCPU+1，故最优子线程数为NCPU

if __name__ == '__main__':
    for i in range(1000000):
        q.put(str(i))
    #time_start=time.time()
    threads=[]
    for i in range(thread_num):
        t=myThread(i+1,"Thread-"+str(i+1))
        threads.append(t)
        #t.start() #不能调用join阻塞主线程，因为主线程不必等待所有子线程完成
    time_start=time.time()
    for i in range(thread_num):
        threads[i].start()
    sema.acquire() #信号量同步
    time_end=time.time()
    print("Password Found! It is "+res+"\n")
    print('Totally Time Cost',time_end-time_start)