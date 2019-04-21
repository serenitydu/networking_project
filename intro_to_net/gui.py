import tkinter
import tkinter.messagebox as messagebox

class Login(object):
    def __init__(self):
        self.root = tkinter.Tk()
        self.root.title('Chat')
        self.root.geometry('450x300')

        #create label
        self.label_account = tkinter.Label(self.root, text = 'Account')
        self.label_passwd = tkinter.Label(self.root, text = 'Password')

        #create input frame
        self.input_account = tkinter.Entry(self.root, width = 30)
        self.input_passwd = tkinter.Entry(self.root, show = '*', width = 30)

        #create button
        self.button_login = tkinter.Button(self.root, command = self.log_interface, text = "Login", width=10)
        self.button_sign = tkinter.Button(self.root, command = self.sign_interface, text = "Sign up", width=10)

    def gui_look(self):
        self.label_account.place(x= 60, y = 170)
        self.label_passwd.place(x = 60, y = 195)
        self.input_account.place(x = 135, y = 170)
        self.input_passwd.place(x = 135, y = 195)
        self.button_login.place(x = 140, y = 235)
        self.button_sign.place(x = 240, y = 235)

    def log_interface(self):
        tkinter.messagebox.showinfo(title='Chat', message='hello')

    def sign_interface(self):
        account = self.input_account.get()
        passwd = self.input_passwd.get()

def main():
    L = Login()
    L.gui_look()
    tkinter.mainloop()
    
if __name__ == '__main__':
        main()