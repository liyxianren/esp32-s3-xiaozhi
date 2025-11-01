#!/usr/bin/env python3
"""
测试 MCP reminder 工具的返回格式
模拟 AI 调用 reminder_add 工具,查看返回结果
"""

import subprocess
import json
import sys

def test_reminder_add():
    """测试 reminder_add 工具"""
    print("=" * 60)
    print("测试 MCP reminder_add 工具")
    print("=" * 60)

    # 模拟 MCP 调用 reminder_add 工具
    # 场景 1: 倒计时提醒 - "10秒后提醒我吃药"
    test_cases = [
        {
            "name": "倒计时提醒 - 10秒后提醒我吃药",
            "params": {
                "delay_seconds": 10,
                "message": "吃药"
            }
        },
        {
            "name": "定时提醒 - 晚上8点提醒我睡觉",
            "params": {
                "delay_seconds": 0,  # 不重要
                "message": "睡觉",
                "hour": 20,
                "minute": 0
            }
        },
        {
            "name": "重复提醒 - 每天早上7点叫我起床",
            "params": {
                "delay_seconds": 0,
                "message": "起床",
                "hour": 7,
                "minute": 0,
                "repeat": 10000,
                "interval": 86400
            }
        }
    ]

    for i, test in enumerate(test_cases, 1):
        print(f"\n{'=' * 60}")
        print(f"测试场景 {i}: {test['name']}")
        print(f"{'=' * 60}")
        print(f"输入参数: {json.dumps(test['params'], ensure_ascii=False, indent=2)}")

        # 构造 MCP 请求
        mcp_request = {
            "jsonrpc": "2.0",
            "id": i,
            "method": "tools/call",
            "params": {
                "name": "reminder_add",
                "arguments": test['params']
            }
        }

        print(f"\nMCP 请求:")
        print(json.dumps(mcp_request, ensure_ascii=False, indent=2))

        # 调用 MCP 工具 (通过 stdin/stdout)
        try:
            proc = subprocess.Popen(
                [sys.executable, "-m", "reminder"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                cwd="c:\\Users\\Administrator\\Desktop\\xiaozhi-esp32-2.0.3\\mcp"
            )

            # 发送 initialize 请求
            init_request = {
                "jsonrpc": "2.0",
                "id": 0,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {},
                    "clientInfo": {"name": "test", "version": "1.0.0"}
                }
            }

            proc.stdin.write(json.dumps(init_request) + "\n")
            proc.stdin.flush()

            # 读取 initialize 响应
            init_response = proc.stdout.readline()
            print(f"\n初始化响应: {init_response[:100]}...")

            # 发送 tools/call 请求
            proc.stdin.write(json.dumps(mcp_request) + "\n")
            proc.stdin.flush()

            # 读取响应
            response_line = proc.stdout.readline()

            try:
                response = json.loads(response_line)
                print(f"\nMCP 响应:")
                print(json.dumps(response, ensure_ascii=False, indent=2))

                # 提取返回内容
                if "result" in response and "content" in response["result"]:
                    content = response["result"]["content"]
                    if len(content) > 0 and "text" in content[0]:
                        result_data = json.loads(content[0]["text"])
                        print(f"\n返回结果:")
                        print(json.dumps(result_data, ensure_ascii=False, indent=2))

                        # 检查关键字段
                        print(f"\n关键字段检查:")
                        print(f"  ✓ success: {result_data.get('success')}")
                        print(f"  ✓ reminder_id: {result_data.get('reminder_id')}")
                        print(f"  ✓ delay_seconds: {result_data.get('delay_seconds')}")
                        print(f"  ✓ hour: {result_data.get('hour')}")
                        print(f"  ✓ minute: {result_data.get('minute')}")
                        print(f"  ✓ repeat: {result_data.get('repeat')}")
                        print(f"  ✓ interval: {result_data.get('interval')}")
                        print(f"  ✓ message: {result_data.get('message')}")

            except json.JSONDecodeError as e:
                print(f"\n解析响应失败: {e}")
                print(f"原始响应: {response_line}")

            proc.terminate()

        except Exception as e:
            print(f"\n调用失败: {e}")

        print("")

if __name__ == "__main__":
    test_reminder_add()
