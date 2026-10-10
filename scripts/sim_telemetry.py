# -*- coding: utf-8 -*-
"""
UXO排弹系统 M4E 遥测模拟器
==========================
作用：不依赖真机，向 EMQX 注入合成 OSD 遥测（按大疆上云API物模型格式），
     用于打通并验证 排弹系统 的整条数据链路。
模拟内容：一条排弹侦察航迹（起飞→网格搜索→激光标记疑似点→返航→降落），
         含电量衰减、RTK状态、云台姿态、激光目标坐标、HMS告警事件。
用法：python sim_telemetry.py [--broker 127.0.0.1] [--port 1883] [--sn 1581F5BKSIMUL001]
依赖：pip install paho-mqtt
停止：Ctrl+C
"""
import json
import math
import argparse
import time
import uuid
import datetime

try:
    import paho.mqtt.client as mqtt
except ImportError:
    raise SystemExit("缺少依赖：请先执行  pip install paho-mqtt")

# ---------------- 参数 ----------------
parser = argparse.ArgumentParser(description="M4E 遥测模拟器")
parser.add_argument("--broker", default="127.0.0.1", help="EMQX 地址")
parser.add_argument("--port", type=int, default=1883, help="EMQX 端口")
parser.add_argument("--sn", default="1581F5BKSIMUL001", help="模拟飞机 SN")
parser.add_argument("--gateway", default="RC2SIMUL0001", help="模拟遥控器(网关) SN")
parser.add_argument("--speedup", type=float, default=1.0, help="时间加速倍率")
args = parser.parse_args()

DRONE_SN = args.sn
GW_SN = args.gateway
INTERVAL = 2.0 / args.speedup   # OSD 0.5Hz

# 场地基准点（示例坐标，接你们自己的机场/场地底图时改这里）
HOME_LAT = 30.000000
HOME_LON = 120.000000
HOME_ALT = 12.0          # 起飞点海拔(m)

# ---------------- 航线：网格搜索 + 两个激光标记点 + 返航 ----------------
def build_route():
    """返回航点列表 (lat, lon, rel_alt, speed, phase)"""
    d_lat = lambda m: HOME_LAT + m / 111320.0
    d_lon = lambda m: HOME_LON + m / (111320.0 * math.cos(math.radians(HOME_LAT)))
    r = []
    # 起飞爬升到 60m
    r.append((d_lat(0), d_lon(0), 60, 3, "起飞爬升"))
    # 网格搜索：4 条平行线，间距 40m，每条 160m
    for i in range(4):
        y = 30 + i * 40
        r.append((d_lat(y), d_lon(-80), 60, 6, "网格搜索"))
        # 每条线中点附近做一次激光标记（悬停）
        r.append((d_lat(y), d_lon(-20), 58, 0, "激光标记"))
        r.append((d_lat(y), d_lon(0), 60, 0, "悬停观测"))
        r.append((d_lat(y), d_lon(80), 60, 6, "网格搜索"))
        r.append((d_lat(y + 20), d_lon(80), 60, 5, "转弯"))
    # 返航
    r.append((d_lat(0), d_lon(0), 80, 8, "返航"))
    r.append((d_lat(0), d_lon(0), 0, 2, "自动降落"))
    return r

ROUTE = build_route()

def lerp(a, b, t):
    return a + (b - a) * t

# ---------------- 报文构造 ----------------
def make_osd(t_s, wp_idx, frac, lat, lon, rel_alt, speed, phase,
             bat_pct, rtk_fixed, gimbal_pitch, gimbal_yaw, zoom,
             laser=None):
    """按 Cloud API 物模型构造 OSD JSON（字段名与官方文档一致）"""
    data = {
        "elevation": HOME_ALT + rel_alt,
        "latitude": round(lat, 9),
        "longitude": round(lon, 9),
        "height": round(rel_alt, 2),
        "horizontal_speed": round(speed, 2),
        "vertical_speed": 0.0,
        "attitude_head": round((wp_idx * 37) % 360, 1),
        "attitude_pitch": round(math.sin(frac * math.pi) * 3, 2),
        "attitude_roll": round(math.cos(frac * math.pi) * 2, 2),
        "mode_code": {  # 起飞3 网格/返航9 悬停0 降落10
            "起飞爬升": 3, "网格搜索": 5, "激光标记": 0,
            "悬停观测": 0, "转弯": 5, "返航": 9, "自动降落": 10,
        }.get(phase, 0),
        "gear": 0,
        "battery": {
            "capacity_percent": bat_pct,
            "batteries": [{
                "index": 0, "capacity_percent": bat_pct,
                "voltage": int(44000 + bat_pct * 80),
                "temperature": 28.5, "loop_times": 12,
                "sn": "BAT-SIM-0001", "type": 0, "sub_type": 0,
                "firmware_version": "01.00.00.00",
            }],
            "remain_flight_time": int(bat_pct * 0.8),
            "return_home_power": 30, "landing_power": 15,
        },
        "position_state": {
            "is_fixed": 2 if rtk_fixed else 0,
            "quality": 5 if rtk_fixed else 1,
            "gps_number": 18, "rtk_number": 28 if rtk_fixed else 0,
        },
        "home_distance": round(math.hypot((lat-HOME_LAT)*111320, (lon-HOME_LON)*100000), 1),
        "wind_speed": 2.3, "wind_direction": 145,
        "total_flight_time": round(t_s, 1),
        "total_flight_distance": round(t_s * speed * 0.6, 1),
        "rth_altitude": 80, "rc_lost_action": 2, "distance_limit_status": 0,
        "storage": {"total": 62000000, "used": 15000000},
        "firmware_version": "12.00.01.06",
        # M4E 相机 89-0-0（激光测距目标字段挂在载荷下）
        "89-0-0": {
            "gimbal_pitch": round(gimbal_pitch, 1),
            "gimbal_roll": 0.0,
            "gimbal_yaw": round(gimbal_yaw, 1),
            "zoom_factor": zoom,
            "payload_index": "89-0-0", "version": 1,
        },
    }
    if laser:  # 激光测距目标点（排弹核心字段）
        data["89-0-0"].update({
            "measure_target_latitude": round(laser[0], 9),
            "measure_target_longitude": round(laser[1], 9),
            "measure_target_altitude": round(laser[2], 1),
            "measure_target_distance": round(laser[3], 1),
            "measure_target_error_state": 1,
        })
    return {
        "tid": str(uuid.uuid4()),
        "bid": str(uuid.uuid4()),
        "timestamp": int(time.time() * 1000),
        "data": data,
        "gateway": GW_SN,
    }

def make_event(evt):
    return {
        "tid": str(uuid.uuid4()), "bid": str(uuid.uuid4()),
        "timestamp": int(time.time() * 1000),
        "gateway": GW_SN, "method": evt, "data": {},
        "need_reply": 0,
    }

# ---------------- MQTT ----------------
try:
    # paho-mqtt 2.x
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1,
                         client_id=f"sim_{DRONE_SN}", clean_session=True)
except AttributeError:
    # paho-mqtt 1.x
    client = mqtt.Client(client_id=f"sim_{DRONE_SN}", clean_session=True)
# 若 EMQX 开了认证，在此填账号：
# client.username_pw_set("simulator", "simulator")

def on_connect(c, u, f, rc):
    print(f"[MQTT] 已连接 broker（rc={rc}），开始注入模拟遥测…")
    # 设备上线状态（简化拓扑）
    topo = {
        "tid": str(uuid.uuid4()), "bid": str(uuid.uuid4()),
        "timestamp": int(time.time() * 1000),
        "gateway": GW_SN,
        "data": {"sub_devices": [{
            "device_sn": DRONE_SN, "device_model_key": "0-99-0",
            "device_online_status": 1, "device_paired": 1,
        }]},
    }
    c.publish(f"thing/product/{GW_SN}/state", json.dumps(topo), qos=1)

client.on_connect = on_connect
try:
    client.connect(args.broker, args.port, keepalive=30)
except Exception as e:
    raise SystemExit(f"[MQTT] 无法连接 {args.broker}:{args.port} —— {e}\n"
                     f"请确认 EMQX 已启动（仓根 docker-compose.yml）且地址正确。")
client.loop_start()

# ---------------- 主循环：沿航线注入 ----------------
print(f"模拟航迹：{len(ROUTE)} 个航段 ｜ 飞机SN={DRONE_SN} ｜ broker={args.broker}:{args.port}")
print("订阅方参考：thing/product/%s/osd" % DRONE_SN)
print("按 Ctrl+C 停止\n")

osd_topic = f"thing/product/{DRONE_SN}/osd"
evt_topic = f"thing/product/{DRONE_SN}/events"

try:
    total_wp = len(ROUTE) - 1
    t_s = 0.0
    for i in range(total_wp):
        (la0, lo0, a0, _, ph) = ROUTE[i]
        (la1, lo1, a1, sp, _) = ROUTE[i + 1]
        seg_len = math.hypot((la1-la0)*111320, (lo1-lo0)*100000) or 1.0
        n_steps = max(2, int(seg_len / 20 / args.speedup)) if sp > 0 else max(2, int(4 * args.speedup))
        for s in range(n_steps):
            frac = s / n_steps
            lat, lon, alt = lerp(la0, la1, frac), lerp(lo0, lo1, frac), lerp(a0, a1, frac)
            bat = max(20, 95 - int(t_s / 18))          # ~36min 放电曲线
            rtk = True
            zoom = 1.0 if "搜索" in ph else (12.0 if "标记" in ph else 4.0)
            laser = None
            if ph == "激光标记":
                # 激光指向航迹下方偏移一点的目标（模拟疑似弹着点）
                laser = (lat - 15/111320, lon + 8/100000, HOME_ALT + 0.5, 42.5)
            msg = make_osd(t_s, i, frac, lat, lon, alt, sp, ph, bat, rtk,
                           gimbal_pitch=-35 if laser else -10,
                           gimbal_yaw=(i*37) % 360, zoom=zoom, laser=laser)
            client.publish(osd_topic, json.dumps(msg), qos=0)
            print(f"\r[{datetime.datetime.now():%H:%M:%S}] {ph:6s} "
                  f"alt={alt:5.1f}m bat={bat:3d}% lat={lat:.6f} lon={lon:.6f}"
                  + (f" [LASER→{laser[0]:.6f},{laser[1]:.6f}]" if laser else ""),
                  end="", flush=True)
            time.sleep(INTERVAL)
            t_s += INTERVAL
    # 结束事件
    client.publish(evt_topic, json.dumps(make_event("device_exit_hms")), qos=1)
    print("\n[完成] 模拟航迹注入结束。")
except KeyboardInterrupt:
    print("\n[停止] 用户中断。")
finally:
    client.loop_stop()
    client.disconnect()
