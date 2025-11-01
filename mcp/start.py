#!/usr/bin/env python3
"""
启动 MCP 服务的便捷脚本
直接在代码中设置 MCP_ENDPOINT，无需配置环境变量
"""

import os
import sys

# 设置 MCP endpoint
MCP_ENDPOINT = "wss://api.xiaozhi.me/mcp/?token=eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjM2MTEyNywiYWdlbnRJZCI6OTQ0MjA1LCJlbmRwb2ludElkIjoiYWdlbnRfOTQ0MjA1IiwicHVycG9zZSI6Im1jcC1lbmRwb2ludCIsImlhdCI6MTc2MTg4NDM5MCwiZXhwIjoxNzkzNDQxOTkwfQ.klaAkEoblIDlO4rAKiSMxzvJ4w2Hrn9Q3Hr0Zpu0PTK1gJNH1uEYR0Ysg5v1iojZ5gYX-JpDS1u0WVgWyUgUKg"

# 设置环境变量
os.environ['MCP_ENDPOINT'] = MCP_ENDPOINT

print("=" * 60)
print("启动小智 MCP 服务")
print("=" * 60)
print()
print("已配置服务:")
print("  - Calculator (计算器)")
print("  - Reminder (定时提醒)")
print()
print(f"正在连接到: {MCP_ENDPOINT[:40]}...")
print()

# 执行 mcp_pipe.py
with open('mcp_pipe.py', 'r', encoding='utf-8') as f:
    exec(f.read())
