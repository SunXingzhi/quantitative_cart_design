import wx
import wx.html2
import os
import json

class MapFrame(wx.Frame):
	def __init__(self):
		super().__init__(None, title="路径规划工具", size=(1000, 800))

		panel = wx.Panel(self)
		vbox = wx.BoxSizer(wx.VERTICAL)

		# WebView
		self.browser = wx.html2.WebView.New(panel)
		current_dir = os.path.dirname(os.path.abspath(__file__))
		html_path = os.path.join(current_dir, "map.html")
		self.browser.LoadURL(f"file://{html_path}")

		# 按钮区
		hbox = wx.BoxSizer(wx.HORIZONTAL)
		self.btn_get_car = wx.Button(panel, label="获取小车位置")
		self.btn_set_waypoints = wx.Button(panel, label="开始设置途径点")
		self.btn_confirm = wx.Button(panel, label="确定途径点")
		self.btn_reset = wx.Button(panel, label="重置")

		hbox.Add(self.btn_get_car, 0, wx.ALL, 5)
		hbox.Add(self.btn_set_waypoints, 0, wx.ALL, 5)
		hbox.Add(self.btn_confirm, 0, wx.ALL, 5)
		hbox.Add(self.btn_reset, 0, wx.ALL, 5)

		vbox.Add(self.browser, 1, wx.EXPAND | wx.ALL, 5)
		vbox.Add(hbox, 0, wx.ALIGN_CENTER)

		panel.SetSizer(vbox)

		# 绑定事件
		self.btn_get_car.Bind(wx.EVT_BUTTON, self.on_get_car_position)
		self.btn_set_waypoints.Bind(wx.EVT_BUTTON, self.on_set_waypoints)
		self.btn_confirm.Bind(wx.EVT_BUTTON, self.on_confirm)
		self.btn_reset.Bind(wx.EVT_BUTTON, self.on_reset)

		# 注入 wx 对象到 JS
		self.browser.AddScriptMessageHandler("wx")
		self.browser.Bind(wx.html2.EVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, self.on_js_message)

		self.waypoints = []  # 存储途径点

	def run_js(self, script):
		"""执行 JavaScript"""
		if self.browser:
			self.browser.RunScript(script)

	def on_get_car_position(self, event):
		script = """
		const pos = window.getCurrentCarPosition();
		window.wx.postMessage(JSON.stringify({type: 'car_position', data: pos}));
		"""
		self.run_js(script)

	def on_set_waypoints(self, event):
		self.run_js("window.setWaypointMode(true);")
		self.waypoints.clear()  # 清空旧数据
		wx.MessageBox("进入设置途径点模式，点击地图添加点", "提示")

	def on_confirm(self, event):
		script = """
		const wps = window.getWaypoints();
		window.wx.postMessage(JSON.stringify({type: 'waypoints', data: wps}));
		"""
		self.run_js(script)

	def on_reset(self, event):
		self.run_js("window.resetWaypoints();")
		self.waypoints.clear()
		wx.MessageBox("已重置途径点", "提示")

	def on_js_message(self, event):
		try:
			msg = json.loads(event.GetString())
			msg_type = msg.get("type")
			data = msg.get("data")

			if msg_type == "car_position":
				wx.MessageBox(f"小车位置：经度 {data[0]:.6f}, 纬度 {data[1]:.6f}", "当前位置")
			elif msg_type == "waypoints":
				self.waypoints = data
				wx.MessageBox(f"已确认 {len(data)} 个途径点\n{data}", "途径点列表")
				# 你可以在这里把数据传给其他模块，或保存到文件
				print("途径点列表：", self.waypoints)
			elif msg_type == "waypoint_added":
				# 可选：实时显示添加的点
				pt = json.loads(data)
				print(f"新增途径点：{pt}")
		except Exception as e:
			print("JS消息解析错误：", e)


class MyApp(wx.App):
	def OnInit(self):
		frame = MapFrame()
		frame.Show()
		return True


if __name__ == "__main__":
	app = MyApp()
	app.MainLoop()