import wx
import wx.lib.newevent
import wx.html2
import time
import socket
import threading
from datetime import datetime
import sys
import os
from pathlib import Path

CustomButtonEvent, EVT_CUSTOM_BUTTON = wx.lib.newevent.NewEvent()

# 获取资源文件的正确路径
def resource_path(relative_path):
    """获取资源的绝对路径，支持开发环境和PyInstaller打包后的环境"""
    try:
        # 打包后资源所在的临时文件夹
        base_path = sys._MEIPASS
    except Exception:
        # 开发环境中的路径
        base_path = os.path.abspath(".")
    
    return os.path.join(base_path, relative_path)


class MyPanel(wx.Panel):
    def __init__(self, parent):
        super().__init__(parent)
        self.Bind(wx.EVT_PAINT, self.on_paint)
        
    def on_paint(self, event):
        dc = wx.PaintDC(self)
        dc.SetPen(wx.Pen(wx.BLACK, 1))
        dc.DrawLine(400,0,400,350)
        dc.DrawLine(0,350,800,350)
        dc.DrawLine(200,350,200,800)
        dc.DrawLine(800,0,800,800)
        dc.DrawLine(25,50,315,50)
        dc.DrawLine(25,50,25,315)
        dc.DrawLine(315,50,315,315)
        dc.DrawLine(315,315,25,315)


class Btn(wx.Panel):
    def __init__(self,parent,label,pos,size):
        super().__init__(parent, pos=pos, size=size)
        self.parent = parent
        self.label  = label
        self.pos    = pos
        self.size   = size
        self.normal_color = wx.Colour("#E4E6E6")
        self.hover_color = wx.Colour("#5e5f5e")
        self.pressed_color = wx.Colour("#050505")
        self.text_color = wx.Colour("#080808")
        self.current_color = self.normal_color
        self.font = wx.Font(20,wx.FONTFAMILY_SWISS,wx.FONTSTYLE_NORMAL,wx.FONTWEIGHT_BOLD)
        self.Bind(wx.EVT_PAINT, self.on_paint)
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.on_erase_background)
        self.Bind(wx.EVT_LEFT_DOWN, self.on_left_down)
        self.Bind(wx.EVT_LEFT_UP, self.on_left_up)
        self.Bind(wx.EVT_LEAVE_WINDOW, self.on_leave_window)
        self.Bind(wx.EVT_ENTER_WINDOW, self.on_enter_window)
        
    def on_paint(self, event):
        dc = wx.PaintDC(self)
        width, height = self.GetSize()
        dc.SetBackground(wx.Brush(self.GetParent().GetBackgroundColour()))
        dc.Clear()
        dc.SetBrush(wx.Brush(self.current_color))
        dc.SetPen(wx.Pen(self.current_color))
        dc.DrawRoundedRectangle(0, 0, width, height, 5)
        dc.SetTextForeground(self.text_color)
        dc.SetFont(self.GetFont())
        text_width, text_height = dc.GetTextExtent(self.label)
        dc.DrawText(self.label, (width - text_width) // 2, (height - text_height) // 2)
        
    def on_erase_background(self, event):
        pass
        
    def on_left_down(self, event):
        self.current_color = self.pressed_color
        self.Refresh()
        event.Skip()
        
    def on_left_up(self, event):
        evt = CustomButtonEvent(buttonId=self.GetId(), button=self)
        wx.PostEvent(self, evt)
        self.current_color = self.hover_color if self.HasFocus() else self.normal_color
        self.Refresh()
        event.Skip()
        
    def on_enter_window(self, event):
        self.current_color = self.hover_color
        self.Refresh()
        event.Skip()
        
    def on_leave_window(self, event):
        self.current_color = self.normal_color
        self.Refresh()
        event.Skip()
        
    def GetLabel(self):
        return self.label
        
    def SetLabel(self, label):
        self.label = label
        self.Refresh()


class MainFrame(wx.Frame):
    def __init__(self):
        super().__init__(None, title="定量化小车配套程序", size=(1400, 800))
        panel = MyPanel(self)

        # 初始化动态 WebView 为 None
        self.dynamic_browser = None

        #文本
        text_font01 = wx.Font(15, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
        text_font02 = wx.Font(12, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)

        # 文本标签
        wx.StaticText(panel, label="请输入IP地址:", pos=(10, 5))
        text02 = wx.StaticText(panel, label="WIFI配置", pos=(550, 5))
        text02.SetFont(text_font01)

        wx.StaticText(panel, label="WIFI名称", pos=(450, 75)).SetFont(text_font02)
        wx.StaticText(panel, label="WIFI密码", pos=(450, 125)).SetFont(text_font02)
        wx.StaticText(panel, label="状态栏", pos=(250, 375)).SetFont(text_font02)
        wx.StaticText(panel, label="自主导航区", pos=(820, 23)).SetFont(text_font02)

        #按钮
        self.button01 = Btn(panel, label="确认", pos=(275, 0), size=(50, 25))
        self.button02 = Btn(panel, label="刷新", pos=(325, 150), size=(50, 25))
        self.button03 = Btn(panel, label="确认", pos=(575, 250), size=(50, 25))
        self.button05 = Btn(panel, label="刷新", pos=(50, 450), size=(100, 50))
        self.button06 = Btn(panel, label="清空状态栏", pos=(50, 600), size=(100, 50))
        self.button07 = Btn(panel, label="获取当前位置", pos=(900, 500), size=(200, 50))
        self.button08 = Btn(panel, label="选择终点位置", pos=(1150, 500), size=(200, 50))
        self.button09 = Btn(panel, label="获取路径", pos=(900, 600), size=(200, 50))
        self.button10 = Btn(panel, label="开始导航", pos=(1150, 600), size=(200, 50))

        # 文本框
        self.textctrl01 = wx.TextCtrl(panel, pos=(100, 0), size=(160, -1))
        self.textctrl02 = wx.TextCtrl(panel, pos=(550, 75), size=(200, -1))
        self.textctrl03 = wx.TextCtrl(panel, pos=(550, 125), size=(200, -1))
        self.textctrl04 = wx.TextCtrl(panel, style=wx.TE_MULTILINE | wx.TE_RICH2, pos=(250, 420), size=(500, 300))

        # 嵌入地图 WebView - 使用资源路径函数
        self.map_browser = wx.html2.WebView.New(panel, pos=(820, 50), size=(550, 420))
        html_path = resource_path("av_web_copy.html")
        self.map_browser.LoadURL(f"file:///{html_path}")
        
        self.map_browser.Bind(wx.html2.EVT_WEBVIEW_LOADED, self.on_webview_loaded)

        # 绑定事件
        self.button01.Bind(EVT_CUSTOM_BUTTON, self.on_button_click)
        self.button07.Bind(EVT_CUSTOM_BUTTON, self.get_gps)
        self.button03.Bind(EVT_CUSTOM_BUTTON, self.on_button_click_wifi)
        self.button06.Bind(EVT_CUSTOM_BUTTON, self.on_button_clear)

        self.Show()


    #事件
    def on_webview_loaded(self, event):
        print("WebView 加载完成,可安全执行JS")

        #清空状态栏
    def on_button_clear(self, event):
        self.textctrl04.Clear()
        
        #地图
    def get_gps(self, event):
    # 确保页面已加载
        if self.map_browser.GetCurrentURL().startswith("file://"):
        # 检查高德地图API是否加载成功
            if not self.map_browser.RunScript("typeof AMap !== 'undefined'"):
                wx.MessageBox("高德地图API加载失败，请检查网络连接", "错误", wx.OK | wx.ICON_ERROR)
                return
        # 检查 initMap 是否存在
            if not self.map_browser.RunScript("typeof initMap !== 'undefined'"):
                wx.MessageBox("initMap 函数未定义", "错误", wx.OK | wx.ICON_ERROR)
                return
            js_code = "initMap(15, [116.397428, 39.90923], '2D');"
            self.map_browser.RunScript(js_code)
        else:
            wx.MessageBox("地图页面未加载完成", "提示")

        #IP
    def on_button_click(self, event):
        text_value = self.textctrl01.GetValue()
        if text_value.strip() == "":
            self.textctrl01.SetBackgroundColour(wx.Colour("#FFCCCC"))
            wx.MessageBox("内容不能为空！", "警告", wx.OK | wx.ICON_WARNING)
        else:
            self.textctrl01.SetBackgroundColour(wx.Colour("#CCFFCC"))
            # 销毁旧的 WebView（如果存在）
            if hasattr(self, 'dynamic_browser') and self.dynamic_browser:
                self.dynamic_browser.Destroy()
            # 创建新的
            self.dynamic_browser = wx.html2.WebView.New(self, pos=(25, 50), size=(290, 280))
            self.dynamic_browser.LoadURL(text_value)
        self.textctrl01.Refresh()

        #paiWiFi通信 
    def on_button_click_wifi(self, event):
        def server_worker():
            HOST = '0.0.0.0'  
            PORT = 65432
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    s.bind((HOST, PORT))
                    s.listen(1)
                    s.settimeout(60)
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] 等待WIFI配置连接...（60秒超时）\n")

                    try:
                        conn, addr = s.accept()
                    except socket.timeout:
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] 等待超时，未收到连接。\n")
                        return  

                    with conn:
                        timestamp = datetime.now().strftime("%H:%M:%S")
                        wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] 已连接设备：{addr}\n")
                        conn.settimeout(30)
                        while True:
                            try:
                                data = conn.recv(1024)
                                if not data: 
                                    break
                                msg = data.decode('utf-8', errors='replace')
                                timestamp = datetime.now().strftime("%H:%M:%S")
                                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] [树莓派]: {msg}\n")
                            except socket.timeout:
                                timestamp = datetime.now().strftime("%H:%M:%S")
                                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] 接收超时，断开连接。\n")
                                break
                            except Exception as e:
                                timestamp = datetime.now().strftime("%H:%M:%S")
                                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] 接收错误: {e}\n")
                                break
            except Exception as e:
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}] 服务器错误: {e}\n")
                wx.CallAfter(wx.MessageBox, f"网络服务启动失败：{e}", "错误", wx.OK | wx.ICON_ERROR)

        # 启动线程
        thread = threading.Thread(target=server_worker, daemon=True)
        thread.start()



def main():
    app = wx.App()
    frame = MainFrame()
    app.MainLoop()


if __name__ == '__main__':
    main()