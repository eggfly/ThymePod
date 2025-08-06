

import time
from platformio.device.monitor.client import SerialMonitor  # 正确导入路径

def delay_before_monitor(env):
    print("[INFO] Waiting 3 seconds for serial port release...")
    time.sleep(3)  # 3秒延迟
    
    # 启动串口监视器
    port = env.subst("$UPLOAD_PORT")
    baud = env.subst("$UPLOAD_SPEED")
    monitor = SerialMonitor(port, baud)
    monitor.start()

delay_before_monitor(env)  # SCons 环境注入
