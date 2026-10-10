-- UXO 排弹系统 · 轨迹库初始化（REQ-012 WP6 TrackStore，docker 首次启动自动执行）
-- 收编自开工包 uxo_m4e_kickstart/deploy/init-db/01_schema.sql（2026-10-11 verbatim 收编）：
-- 仅 trajectory / events 两表；laser_targets / task / media 三表按 WP6 裁决④不收录
-- （laser_targets 计划原文不启用；task/media 一期无数据源）。
-- 库：uxo（PostGIS 扩展随 postgis/postgres 镜像自带）

CREATE EXTENSION IF NOT EXISTS postgis;

-- 1. 遥测轨迹表：一行 = 一条 OSD（0.5Hz）
CREATE TABLE IF NOT EXISTS trajectory (
    id           BIGSERIAL PRIMARY KEY,
    task_id      TEXT,
    drone_sn     TEXT           NOT NULL,
    gateway_sn   TEXT,
    ts_utc       TIMESTAMPTZ    NOT NULL,           -- 报文 timestamp（毫秒→时间戳）
    lat          DOUBLE PRECISION,
    lon          DOUBLE PRECISION,
    rel_alt      DOUBLE PRECISION,                  -- 相对起飞高度
    elevation    DOUBLE PRECISION,                  -- 海拔
    h_speed      DOUBLE PRECISION,
    v_speed      DOUBLE PRECISION,
    attitude_head DOUBLE PRECISION,
    attitude_pitch DOUBLE PRECISION,
    attitude_roll  DOUBLE PRECISION,
    mode_code    INT,
    battery_pct  INT,
    remain_flight_time INT,
    rtk_fixed    INT,                               -- 2=RTK固定解
    wind_speed   DOUBLE PRECISION,
    home_distance DOUBLE PRECISION,
    raw          JSONB,                             -- 原始报文全量留存
    geom         GEOMETRY(PointZ, 4326)             -- 空间列（PostGIS）
);
CREATE INDEX IF NOT EXISTS idx_traj_sn_ts   ON trajectory (drone_sn, ts_utc);
CREATE INDEX IF NOT EXISTS idx_traj_task    ON trajectory (task_id);
CREATE INDEX IF NOT EXISTS idx_traj_geom    ON trajectory USING GIST (geom);

-- 2. 事件表：HMS告警 / 返航 / 低电量等
CREATE TABLE IF NOT EXISTS events (
    id           BIGSERIAL PRIMARY KEY,
    task_id      TEXT,
    drone_sn     TEXT           NOT NULL,
    ts_utc       TIMESTAMPTZ    NOT NULL,
    method       TEXT,                              -- 事件方法名
    level        INT,                              -- 告警级别
    data         JSONB,
    raw          JSONB
);
CREATE INDEX IF NOT EXISTS idx_evt_sn_ts ON events (drone_sn, ts_utc);
