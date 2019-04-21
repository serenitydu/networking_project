#client
import socket,sys, time

host = '127.0.0.1'
port = 9999
sok = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

class ClientEnd:
    username = ''
    password = ''#need to avoid parsing error,didn't fix
    s = 0
    def __init__(self, s):
        self.s = s

    def setConnection(self,host,port):
        try:   
            self.s.connect((host, port))
            time.sleep(0.5)
        except socket.error as e:
            print(str(e))
            sys.exit(0)

    def checkInfo(self):
        while True:
            self.username = input('Username: ')
            self.s.send(self.username.encode('utf-8'))
            buf = self.s.recv(1024)
            if(buf.decode('utf-8')=='1'):
                
                break
            print('Wrong username,please re-input or regist!')
        while True:
            self.password = input('Password: ')
            buf = self.s.recv(1024)
            if(buf.decode('utf-8')=='1'):
                break
            print('PLease check your password again!')
        
    def chat(self):
        #receive connection information from server
        buf = self.s.recv(2048)
        print('Server: '+buf.decode('utf-8'))

        #start dialogue
        while True:
            data = input('client: ')
            #check exit command
            if(data == '-Logout'):
                self.s.send(data.encode('utf-8'))
                break
            
            self.s.send(data.encode('utf-8'))
            buf = self.s.recv(2048)
            if len(buf):
                print('Server: '+buf.decode('utf-8'))
        
        #End dialogue
        print('Dialogue Over!')
        self.s.close()
        sys.exit(0)

user1 = ClientEnd(sok)            
user1.setConnection(host,port)
#user1.checkInfo()
user1.chat()