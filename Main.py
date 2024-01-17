from PyQt5.QtCore import Qt, QThread, pyqtSignal
from PyQt5.QtWidgets import QWidget, QApplication,QPlainTextEdit
from PyQt5.QtGui import QKeyEvent, QTextCursor
import sys
from Ui_main import Ui_WSRC
import socket
from ctypes import string_at

class ReadThread(QThread):
    data_received = pyqtSignal(str)

    def run(self):
        global client_socket,server_socket
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind(('0.0.0.0', 8888))
        server_socket.listen(5)
        client_socket, client_address = server_socket.accept()
        try:
            while True:
                data = client_socket.recv(1024000)
                if data:
                    self.data_received.emit(string_at(data).decode('gbk'))
                else:
                    break
        finally:
            client_socket.close()

    

class MainWidget(QWidget, Ui_WSRC):
    block_num,line_num,currect_block_num,currect_line_num=int,int,0,0
    def __init__(self):
        super().__init__()
        self.setupUi(self)
        self.append_text("Welcome use WSRC Server\n")
        self.read_thread = ReadThread()
        self.read_thread.data_received.connect(self.on_data_received)
        self.read_thread.start()
        self.send_button.clicked.connect(self.send_msg)
        self.send_thing.returnPressed.connect(self.send_msg)
        self.remote_terminal.cursorPositionChanged.connect(self.on_cursor_changed)
        self.remote_terminal.keyPressEvent=self.output_key


    def on_data_received(self, data):
        self.append_text(data)

    def closeEvent(self, event):
        self.read_thread.terminate()
        event.accept()

    def send_msg(self,data=None):
        if not data:
            data=self.send_thing.text()
        try:
            client_socket.send(data.encode('utf-8'))
            self.send_thing.setText("")

        except:
            self.append_text("客户端未连接或网络错误\n")

    def append_text(self,data):
        cursor = self.remote_terminal.textCursor()
        cursor.movePosition(QTextCursor.End)
        if data=="end":
            self.remote_terminal.appendPlainText(">>")
        else:
            self.remote_terminal.insertPlainText(data)
        cursor.movePosition(QTextCursor.End)
        self.line_num=cursor.columnNumber()
        self.block_num=cursor.blockNumber()
        self.remote_terminal.setTextCursor(cursor)




    def on_cursor_changed(self):
        self.cursor_edit()
    

    def cursor_edit(self):
        cursor=self.remote_terminal.textCursor()
        self.currect_block_num= cursor.blockNumber()
        self.currect_line_num=cursor.columnNumber()


    def output_key(self,event):
        if event.key()==Qt.Key_Backspace:
            if self.currect_block_num<self.block_num or (self.currect_line_num==self.line_num and self.currect_block_num<=self.block_num):
                return
        elif event.key()==Qt.Key_Return:
            self.get_command()
            return
        else: 
            if self.currect_block_num<self.block_num or (self.currect_block_num==self.block_num and self.currect_line_num<self.line_num):  
                return
        QPlainTextEdit.keyPressEvent(self.remote_terminal,event)

    def get_command(self,terminal="remote"):
        cursor=self.remote_terminal.textCursor()
        cursor.movePosition(QTextCursor.End)
        cursor.movePosition(QTextCursor.Left,n=self.currect_line_num-self.line_num,mode=QTextCursor.KeepAnchor)
        text=cursor.selectedText()
        if text:
            self.append_text("\n")
            self.send_msg(text)



if __name__ == "__main__":
    app = QApplication(sys.argv)
    widget = MainWidget()
    widget.showMaximized()
    sys.exit(app.exec_())
