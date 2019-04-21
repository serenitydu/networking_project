import socket,sys
import _thread

host = '127.0.0.1'
port = 9999
sok = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

class serverEnd:
    s = 0
    connectNumber = 0
    def __init__(self, s, num):
        self.s = s
        self.connectNumber = num
    
    def setServer(self,host, port):
        try:
            self.s.bind((host, port))
        except socket.error as e:
            print(str(e))
        self.s.listen(self.connectNumber)
        print('Waiting for connecting...')

    def checkInfo(self, conn,addr):
        username = conn.recv(1024).decode('utf-8')
        

    #start a thread to handle new connection
    def thread_client(self, conn, addr):
        conn.send(str.encode('You are now connecting to: ' + host))
        self.checkInfo(conn,addr)
        while True:
            data = conn.recv(2048)
            checkData = data.decode('utf-8')
            #handle 'logout' command
            if not data or checkData == '-Logout':
                print('Disconnect: '+ addr[0] + ':'+ str(addr[1]))
                break
            
            conn.sendall(str.encode('Server reply: '+checkData))
        conn.close()
    
    def addThread(self):
        while True:
            conn, addr = self.s.accept()
            print('Connect to: '+ addr[0] + ':'+ str(addr[1]))
            
            _thread.start_new_thread(self.thread_client, (conn, addr))

ser = serverEnd(sok, 5)
ser.setServer(host,port)
ser.addThread()
