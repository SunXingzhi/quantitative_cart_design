import wx
import wx.lib.newevent
import wx.html2
import socket
import threading
from datetime import datetime
import json


CustomButtonEvent, EVT_CUSTOM_BUTTON = wx.lib.newevent.NewEvent()

class MyPanel(wx.Panel):
    def __init__(self, parent):
        super().__init__(parent)
        self.Bind(wx.EVT_PAINT, self.on_paint)

    def on_paint(self, event):
        dc = wx.PaintDC(self)
        dc.SetPen(wx.Pen(wx.BLACK, 1))
        dc.DrawLine(500, 0, 500, 350)
        dc.DrawLine(0, 350, 800, 350)
        dc.DrawLine(200, 350, 200, 800)
        dc.DrawLine(800, 0, 800, 800)
        dc.DrawLine(25, 50, 400, 50)
        dc.DrawLine(25, 50, 25, 315)
        dc.DrawLine(400, 50, 400, 315)
        dc.DrawLine(400, 315, 25, 315)
        dc.DrawLine(800, 675, 1400, 675)


class Btn(wx.Panel):
    def __init__(self, parent, label, pos, size):
        super().__init__(parent, pos=pos, size=size)
        self.parent = parent
        self.label = label
        self.normal_color = wx.Colour("#E4E6E6")
        self.hover_color = wx.Colour("#5e5f5e")
        self.pressed_color = wx.Colour("#050505")
        self.text_color = wx.Colour("#080808")
        self.current_color = self.normal_color
        self.font = wx.Font(12, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
      
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
        dc.SetFont(self.font)
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

        # 初始化
        self.dynamic_browser = None
        self.sock = None
        self.map_loaded = False          
        self.pending_position = None     
        self.update_timer = None 

        # 字体
        text_font01 = wx.Font(15, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
        text_font02 = wx.Font(12, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
        text_font03 = wx.Font(10, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)

        # 标签
        wx.StaticText(panel, label="请输入IP地址:", pos=(10, 5))
        text02 = wx.StaticText(panel, label="WIFI配置", pos=(600, 5))
        text02.SetFont(text_font01)

        wx.StaticText(panel, label="WIFI名称", pos=(515, 50)).SetFont(text_font02)
        wx.StaticText(panel, label="WIFI密码", pos=(515, 125)).SetFont(text_font02)
        wx.StaticText(panel, label="状态栏", pos=(250, 375)).SetFont(text_font02)
        wx.StaticText(panel, label="自主导航区", pos=(820, 23)).SetFont(text_font02)
        wx.StaticText(panel, label="小车信息", pos=(810, 680)).SetFont(text_font02)
        wx.StaticText(panel, label="经度：", pos=(810, 715)).SetFont(text_font03)
        wx.StaticText(panel, label="纬度：", pos=(810, 740)).SetFont(text_font03)
        wx.StaticText(panel, label="V1:", pos=(1050, 715)).SetFont(text_font03)
        wx.StaticText(panel, label="V2:", pos=(1200, 715)).SetFont(text_font03)
        wx.StaticText(panel, label="V3:", pos=(1050, 740)).SetFont(text_font03)
        wx.StaticText(panel, label="V4:", pos=(1200, 740)).SetFont(text_font03)

        # 按钮
        self.button01 = Btn(panel, label="确认", pos=(275, 0), size=(50, 25))
        self.button02 = Btn(panel, label="刷新", pos=(425, 150), size=(50, 25))
        self.button03 = Btn(panel, label="确认", pos=(625, 250), size=(50, 25))
        self.button04 = Btn(panel, label="本机ip获取", pos=(50, 450), size=(100, 50))
        self.button06 = Btn(panel, label="清空状态栏", pos=(50, 600), size=(100, 50))
        self.button07 = Btn(panel, label="打开地图", pos=(900, 500), size=(200, 50))
        self.button08 = Btn(panel, label="发送途径坐标", pos=(1150, 500), size=(200, 50))
        self.button09 = Btn(panel, label="获取当前位置", pos=(900, 600), size=(200, 50))
        self.button10 = Btn(panel, label="开始导航", pos=(1150, 600), size=(200, 50))
        self.button11 = Btn(panel, label="断开连接", pos=(50, 525), size=(100, 50))

        # 文本框
        self.textctrl01 = wx.TextCtrl(panel, pos=(100, 0), size=(160, -1))
        self.textctrl02 = wx.TextCtrl(panel, pos=(550, 75), size=(200, -1))
        self.textctrl03 = wx.TextCtrl(panel, pos=(550, 150), size=(200, -1))
        self.textctrl04 = wx.TextCtrl(panel, style=wx.TE_MULTILINE | wx.TE_READONLY, pos=(250, 420), size=(500, 300))
        self.textctrl05 = wx.TextCtrl(panel, style=wx.TE_READONLY, pos=(850, 705), size=(100, -1))
        self.textctrl06 = wx.TextCtrl(panel, style=wx.TE_READONLY, pos=(850, 735), size=(100, -1))
        self.textctrl07 = wx.TextCtrl(panel, style=wx.TE_READONLY, pos=(1100, 705), size=(50, -1))
        self.textctrl08 = wx.TextCtrl(panel, style=wx.TE_READONLY, pos=(1250, 705), size=(50, -1))
        self.textctrl09 = wx.TextCtrl(panel, style=wx.TE_READONLY, pos=(1100, 735), size=(50, -1))
        self.textctrl10 = wx.TextCtrl(panel, style=wx.TE_READONLY, pos=(1250, 735), size=(50, -1))

        # 绑定事件
        self.button01.Bind(EVT_CUSTOM_BUTTON, self.on_button_click)
        self.button02.Bind(EVT_CUSTOM_BUTTON, self.on_refresh)
        self.button03.Bind(EVT_CUSTOM_BUTTON, self.connect_to_pi)
        self.button04.Bind(EVT_CUSTOM_BUTTON, self.get_local_ip)
        self.button06.Bind(EVT_CUSTOM_BUTTON, self.on_button_clear)
        self.button07.Bind(EVT_CUSTOM_BUTTON, self.get_gps)
        self.button08.Bind(EVT_CUSTOM_BUTTON, self.on_send_point)
        self.button09.Bind(EVT_CUSTOM_BUTTON,self.on_get_position)
        self.button10.Bind(EVT_CUSTOM_BUTTON,self.on_send_start)
        self.button11.Bind(EVT_CUSTOM_BUTTON, self.disconnect)

        # 地图 WebView
        self.map_browser = wx.html2.WebView.New(panel, pos=(820, 50), size=(550, 420))
        
        
        self.Show()

    def btn_search(self):
        '''确保IP内容'''
        text_value = self.textctrl02.GetValue().strip()
        if not text_value:
            self.textctrl02.SetBackgroundColour(wx.Colour("#FFCCCC"))
            wx.MessageBox("内容不能为空！", "警告", wx.OK | wx.ICON_WARNING)
            self.textctrl02.Refresh()
            return False
        else:
            self.textctrl02.SetBackgroundColour(wx.Colour("#CCFFCC"))
            self.textctrl02.Refresh()
            return True

    def on_webview_loaded(self, event):
        """地图加载"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]地图页面加载完成\n")
        wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        self.map_loaded = True

    def _safe_run_js(self, script):
        """安全执行 JS，避免崩溃""" 
        try:
            self.map_browser.RunScript(script)
        except Exception as e:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]JS执行异常: {str(e)[:60]}\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())

    def _schedule_position_update(self, lng, lat):
        """防抖：合并高频位置更新"""
        self.pending_position = (lng, lat)
        if self.update_timer is None:
            self.update_timer = wx.CallLater(50, self._do_position_update)  # 50ms 延迟

    def _do_position_update(self):
        """执行位置更新"""
        if self.pending_position and self.map_loaded:
            lng, lat = self.pending_position
            try:
                float(lng)
                float(lat)
                self._safe_run_js(f'updateVehiclePosition({lng}, {lat});')
            except ValueError:
                pass  # 忽略无效数据
        self.pending_position = None
        self.update_timer = None
    
    def get_gps(self, event):
        """打开地图"""
        map_url = "file:///G:/GUI/dsds.html"
        self.map_browser.Bind(wx.html2.EVT_WEBVIEW_LOADED, self.on_webview_loaded)
        self.map_browser.LoadURL(map_url)
       
    def on_refresh(self, event):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        if self.dynamic_browser:
            self.dynamic_browser.Reload()
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]已刷新网页\n")
        else:
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]未打开网页，无法刷新\n")
        wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
    
    def on_button_clear(self, event):
        """清空状态栏"""
        self.textctrl04.Clear()

    def on_button_click(self, event):
        text_value = self.textctrl01.GetValue().strip()
        if not text_value:
            self.textctrl01.SetBackgroundColour(wx.Colour("#FFCCCC"))
            wx.MessageBox("内容不能为空！", "警告", wx.OK | wx.ICON_WARNING)
            self.textctrl01.Refresh()
            return
        if not text_value.startswith(('http://', 'https://')):
            text_value = 'http://' + text_value
        self.textctrl01.SetBackgroundColour(wx.Colour("#CCFFCC"))
        self.textctrl01.Refresh()
        if self.dynamic_browser:
            self.dynamic_browser.Destroy()
        self.dynamic_browser = wx.html2.WebView.New(self.GetChildren()[0], pos=(25, 50), size=(390, 280))
        self.dynamic_browser.LoadURL(text_value)

    def get_local_ip(self, event):
        """获取本机ip"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]本机局域网IP: {ip}\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
            return ip
        except Exception as e:
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]获取IP失败: {e}\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
            return "127.0.0.1"

    def connect_to_pi(self, event):
        if not self.btn_search():
            return
        HOST = self.textctrl02.GetValue().strip()
        PORT = 65432
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]尝试连接 {HOST}:{PORT} ...\n")
        wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        def connect():
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(60)
                self.sock.connect((HOST, PORT))
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]✅ 连接树莓派成功\n")
                wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
                threading.Thread(target=self.receive_messages, daemon=True).start()
            except socket.timeout:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]❌ 连接超时\n")
            except Exception as e:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]❌ 连接失败: {e}\n")
            finally:
                wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        threading.Thread(target=connect, daemon=True).start()

    def on_send_point(self, event):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        JS_pos = "JSON.stringify(markerCoordinates)"
        result = self.map_browser.RunScript(JS_pos)
        if not result or result[1] == "undefined":
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]未获取到路径点\n")
            return

        try:
            
            points = json.loads(result[1])
            if len(points) > 10:
                dialog = wx.MessageDialog(self, "途经点不超过10个", "提示", wx.OK | wx.ICON_INFORMATION)
                dialog.ShowModal()
                dialog.Destroy()
                return

            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]=== 发送路径点 ===\n")
            point_parts =[]
            for i, point in enumerate(points):
                lng = point['lng']
                lat = point['lat']
                point_parts.append(f"/{lng}/{lat}")
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]点{i+1}: 经度:{lng}, 纬度:{lat}\n")
            msg = "@p" + "".join(point_parts) + "*"

            if self.sock:
                try:
                    self.sock.sendall(msg.encode('utf-8'))
                except Exception as e:
                    wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]发送失败: {e}\n")
            else:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]未连接，无法发送\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        except json.JSONDecodeError:
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]JSON解析失败\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())

    def on_send_start(self, event):
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            try:
                msg = "@q/Nstart*"
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]请求中导航\n")

                if self.sock:
                    try:
                        self.sock.sendall(msg.encode('utf-8'))
                    except Exception as e:
                        wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]发送失败: {e}\n")
                else:
                    wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]未连接，无法发送\n")
                wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
            except json.JSONDecodeError:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]JSON解析失败\n")
                wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())


    def receive_messages(self):
        buffer = ""
        try:
            while self.sock is not None:
                data = self.sock.recv(1024)
                if not data:
                    wx.CallAfter(self.textctrl04.AppendText, f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]树莓派断开连接\n")
                    break
                buffer += data.decode('utf-8', errors='ignore')
                while True:
                    start = buffer.find('@')
                    end = buffer.find('*', start)
                    if start == -1 or end == -1:
                        break
                    segment = buffer[start+1:end]
                    buffer = buffer[end+1:]
                    wx.CallAfter(self.process_segment, segment)
        except ConnectionResetError:
            wx.CallAfter(self.textctrl04.AppendText, f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]连接被强制关闭\n")
        except Exception as e:
            wx.CallAfter(self.textctrl04.AppendText, f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]接收异常: {e}\n")
        finally:
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
                self.sock = None
            wx.CallAfter(self.textctrl04.AppendText, f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]接收线程结束\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())


    def on_get_position(self, event):
        self.textctrl05.Clear()
        self.textctrl06.Clear()
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        lng_str = self.textctrl05.GetValue().strip()
        lat_str = self.textctrl06.GetValue().strip()
        try:
                msg = "@q/NPget/*"
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]请求位置中\n")

                if self.sock:
                    try:
                        self.sock.sendall(msg.encode('utf-8'))
                    except Exception as e:
                        wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]发送失败: {e}\n")
                else:
                    wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]未连接，无法发送\n")
                wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        except json.JSONDecodeError:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]JSON解析失败\n")
                wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())

        if not lng_str or not lat_str:
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]当前无有效经纬度数据，无法显示位置。\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
            return
        try:
            lng = float(lng_str)
            lat = float(lat_str)
            if self.map_loaded:
                wx.CallAfter(self._schedule_position_update, lng, lat)
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]已在地图显示当前位置: {lng}, {lat}\n")
            else:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]请先点击“打开地图”\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        except ValueError:
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]经纬度格式错误\n")
            wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())

    def process_segment(self, segment):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        if segment.startswith('n/'):
            parts = segment.split('/')
            if len(parts) >= 3:
                self.textctrl05.SetValue(parts[1]) 
                self.textctrl06.SetValue(parts[2])
                if self.map_loaded:
                    self._schedule_position_update(parts[1], parts[2])  
        elif segment.startswith('m/'):
            parts = segment.split('/')
            if len(parts) >= 5:
                self.textctrl07.SetValue(parts[1])
                self.textctrl08.SetValue(parts[2])
                self.textctrl09.SetValue(parts[3])
                self.textctrl10.SetValue(parts[4])
        elif segment == 'b':
            self.textctrl04.AppendText(f"[{timestamp}]正在刹车\n")
        elif segment == 'o':
            self.textctrl04.AppendText(f"[{timestamp}]打开GPS\n")
        elif segment == 'c':
            self.textctrl04.AppendText(f"[{timestamp}]关闭GPS\n")
        self.textctrl04.ShowPosition(self.textctrl04.GetLastPosition())


    def disconnect(self, event):
        """断开连接（线程安全）"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        if self.sock is not None:
            try:
                self.sock.close()
            except Exception as e:
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]断开连接出错: {e}\n")
            finally:
                self.sock = None
                wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}]已手动断开与树莓派的连接。\n")
        else:
            wx.CallAfter(self.textctrl04.AppendText, f"[{timestamp}][提示]当前未连接。\n")
        wx.CallAfter(self.textctrl04.ShowPosition, self.textctrl04.GetLastPosition())
        
def main():
    app = wx.App()
    frame = MainFrame()
    app.MainLoop()


if __name__ == '__main__':
    main()