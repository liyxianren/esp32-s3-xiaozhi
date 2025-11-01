# reminder.py
"""
定时提醒 MCP 工具
功能:添加、查询、取消定时提醒

实现原理:
1. MCP 工具返回结构化参数(delay, hour, minute等)给 AI
2. AI 将参数包含在回复文本中
3. 设备端解析 AI 回复,提取参数并创建本地定时器
4. 定时器触发时,设备模拟用户查询,AI 回复提醒内容
"""
from mcp.server.fastmcp import FastMCP
import sys
import logging
from datetime import datetime, timedelta
from typing import Dict

logger = logging.getLogger('Reminder')

# Fix UTF-8 encoding for Windows console
if sys.platform == 'win32':
    sys.stderr.reconfigure(encoding='utf-8')
    sys.stdout.reconfigure(encoding='utf-8')

# Create an MCP server
mcp = FastMCP("Reminder")

# 简单的提醒记录(用于查询历史)
reminders: Dict[int, dict] = {}
reminder_id_counter = 0

@mcp.tool()
def reminder_add(delay_seconds: int, message: str, hour: int = -1, minute: int = -1, repeat: int = 1, interval: int = 0) -> dict:
    """
    添加定时提醒

    ⚠️ 重要:调用此工具后,工具会返回一个 instruction 字段,你必须在回复中原样包含这个指令!

    参数:
        delay_seconds: 延迟秒数(倒计时场景),如"10秒后提醒我"则为10
        message: 提醒内容,如"吃药"
        hour: 小时(定时场景),如"早上7点"则为7,默认-1表示不使用
        minute: 分钟(定时场景),如"7:30"则为30,默认-1表示不使用
        repeat: 重复次数,默认1次(不重复),如"每天提醒"则为10000
        interval: 重复间隔(秒),如"每小时"则为3600,默认0

    回复格式:
        用户:"10秒后提醒我吃药"
        工具返回:{"instruction": "[REMINDER_SET:delay=10,hour=-1,minute=-1,repeat=1,interval=0,msg=吃药]", ...}
        你的回复:"好的![REMINDER_SET:delay=10,hour=-1,minute=-1,repeat=1,interval=0,msg=吃药] 已设置10秒后提醒你吃药!"
    """
    global reminder_id_counter

    try:
        # 参数验证
        if delay_seconds < 0:
            delay_seconds = 0

        if not message or len(message.strip()) == 0:
            return {
                "success": False,
                "error": "提醒内容不能为空"
            }

        # 计算触发时间
        now = datetime.now()

        # 如果指定了 hour 或 minute,计算具体时间
        if (hour >= 0 and hour < 24) or (minute >= 0 and minute < 60):
            target_time = datetime.now()
            target_time = target_time.replace(
                hour=hour if (hour >= 0 and hour < 24) else target_time.hour,
                minute=minute if (minute >= 0 and minute < 60) else target_time.minute,
                second=0,
                microsecond=0
            )
            # 如果目标时间已过,则设置为明天
            if target_time <= now:
                target_time += timedelta(days=1)

            trigger_time = target_time
            # 重新计算 delay_seconds 用于日志显示
            delay_seconds = int((trigger_time - now).total_seconds())
        else:
            trigger_time = now + timedelta(seconds=delay_seconds)

        # 创建提醒记录
        reminder_id_counter += 1
        rid = reminder_id_counter

        reminders[rid] = {
            'id': rid,
            'message': message.strip(),
            'delay_seconds': delay_seconds,
            'hour': hour,
            'minute': minute,
            'repeat': repeat,
            'interval': interval,
            'create_time': now,
            'trigger_time': trigger_time,
            'status': 'pending'
        }

        logger.info(f"✅ Added reminder: ID={rid}, delay={delay_seconds}s, message='{message}', hour={hour}, minute={minute}, repeat={repeat}, interval={interval}")

        # 构造返回结果 - AI 会将这些参数包含在回复中
        # 设备端解析 AI 回复提取这些参数
        result = {
            "success": True,
            "reminder_id": rid,
            "message": message.strip(),
            "delay_seconds": delay_seconds,
            "hour": hour,
            "minute": minute,
            "repeat": repeat,
            "interval": interval,
            "trigger_time": trigger_time.strftime('%Y-%m-%d %H:%M:%S'),
            "description": f"已设置提醒,将在{delay_seconds}秒后({trigger_time.strftime('%H:%M:%S')})提醒你:{message}"
        }

        # 输出日志便于调试
        logger.info(f"📤 Returning reminder params: {result}")

        # 重要:返回格式化的文本指令,让 AI 在回复中包含这些信息
        # 设备端会解析 AI 的回复来提取这些参数
        instruction = f"[REMINDER_SET:delay={delay_seconds},hour={hour},minute={minute},repeat={repeat},interval={interval},msg={message.strip()}]"

        return {
            "success": True,
            "instruction": instruction,
            "description": f"已设置提醒,将在{delay_seconds}秒后({trigger_time.strftime('%H:%M:%S')})提醒你:{message}"
        }

    except Exception as e:
        logger.error(f"❌ Error adding reminder: {e}")
        return {
            "success": False,
            "error": str(e)
        }

@mcp.tool()
def reminder_list() -> dict:
    """
    查询所有待触发的提醒

    返回:
        提醒列表
    """
    try:
        reminder_list = []
        now = datetime.now()

        for rid, reminder in reminders.items():
            remaining = (reminder['trigger_time'] - now).total_seconds()
            reminder_list.append({
                'id': rid,
                'message': reminder['message'],
                'delay_seconds': reminder['delay_seconds'],
                'hour': reminder.get('hour', -1),
                'minute': reminder.get('minute', -1),
                'repeat': reminder.get('repeat', 1),
                'interval': reminder.get('interval', 0),
                'create_time': reminder['create_time'].strftime('%Y-%m-%d %H:%M:%S'),
                'trigger_time': reminder['trigger_time'].strftime('%Y-%m-%d %H:%M:%S'),
                'remaining_seconds': max(0, int(remaining)),
                'status': reminder['status']
            })

        logger.info(f"📋 Listed {len(reminder_list)} reminders")

        return {
            "success": True,
            "count": len(reminder_list),
            "reminders": reminder_list
        }

    except Exception as e:
        logger.error(f"❌ Error listing reminders: {e}")
        return {
            "success": False,
            "error": str(e),
            "reminders": []
        }

@mcp.tool()
def reminder_cancel(reminder_id: int) -> dict:
    """
    取消指定的提醒

    参数:
        reminder_id: 提醒ID

    返回:
        操作结果
    """
    try:
        if reminder_id not in reminders:
            return {
                "success": False,
                "error": f"提醒ID {reminder_id} 不存在"
            }

        reminder = reminders[reminder_id]
        del reminders[reminder_id]

        logger.info(f"❌ Cancelled reminder: ID={reminder_id}, message='{reminder['message']}'")

        return {
            "success": True,
            "reminder_id": reminder_id,
            "message": reminder['message'],
            "description": f"已取消提醒:{reminder['message']}"
        }

    except Exception as e:
        logger.error(f"❌ Error cancelling reminder: {e}")
        return {
            "success": False,
            "error": str(e)
        }

# Start the server
if __name__ == "__main__":
    try:
        logger.info("🚀 Starting Reminder MCP server...")
        mcp.run(transport="stdio")
    except KeyboardInterrupt:
        logger.info("⏹ Received interrupt signal")
    except Exception as e:
        logger.error(f"❌ Server error: {e}")
        raise
