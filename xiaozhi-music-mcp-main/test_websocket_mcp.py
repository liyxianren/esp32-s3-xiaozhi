#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试免费音乐MCP WebSocket服务器
"""

import asyncio
import json
import websockets
import logging

# 配置日志
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

async def test_mcp_websocket_server():
    """测试MCP WebSocket服务器"""
    uri = "ws://localhost:8765"
    
    try:
        async with websockets.connect(uri) as websocket:
            logger.info("已连接到MCP WebSocket服务器")
            
            # 测试1: 初始化
            init_message = {
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {
                        "roots": {"listChanged": True},
                        "sampling": {}
                    },
                    "clientInfo": {
                        "name": "test-client",
                        "version": "1.0.0"
                    }
                }
            }
            
            await websocket.send(json.dumps(init_message))
            response = await websocket.recv()
            init_result = json.loads(response)
            logger.info(f"初始化响应: {init_result}")
            
            # 测试2: 获取工具列表
            tools_message = {
                "jsonrpc": "2.0",
                "id": 2,
                "method": "tools/list"
            }
            
            await websocket.send(json.dumps(tools_message))
            response = await websocket.recv()
            tools_result = json.loads(response)
            logger.info(f"工具列表: {len(tools_result.get('result', {}).get('tools', []))} 个工具")
            
            # 测试3: 搜索音乐
            search_message = {
                "jsonrpc": "2.0",
                "id": 3,
                "method": "tools/call",
                "params": {
                    "name": "search_music",
                    "arguments": {
                        "query": "周杰伦",
                        "limit": 5
                    }
                }
            }
            
            await websocket.send(json.dumps(search_message))
            response = await websocket.recv()
            search_result = json.loads(response)
            logger.info("搜索音乐测试完成")
            
            # 测试4: 播放音乐
            play_message = {
                "jsonrpc": "2.0",
                "id": 4,
                "method": "tools/call",
                "params": {
                    "name": "play_music",
                    "arguments": {
                        "song_id": "1",
                        "song_name": "青花瓷",
                        "artist": "周杰伦"
                    }
                }
            }
            
            await websocket.send(json.dumps(play_message))
            response = await websocket.recv()
            play_result = json.loads(response)
            logger.info("播放音乐测试完成")
            
            # 测试5: 设置音量
            volume_message = {
                "jsonrpc": "2.0",
                "id": 5,
                "method": "tools/call",
                "params": {
                    "name": "set_volume",
                    "arguments": {
                        "volume": 80
                    }
                }
            }
            
            await websocket.send(json.dumps(volume_message))
            response = await websocket.recv()
            volume_result = json.loads(response)
            logger.info("设置音量测试完成")
            
            # 测试6: 获取资源列表
            resources_message = {
                "jsonrpc": "2.0",
                "id": 6,
                "method": "resources/list"
            }
            
            await websocket.send(json.dumps(resources_message))
            response = await websocket.recv()
            resources_result = json.loads(response)
            logger.info(f"资源列表: {len(resources_result.get('result', {}).get('resources', []))} 个资源")
            
            print("\n=== 测试结果 ===")
            print("✅ 所有测试通过!")
            print("\n=== 服务器信息 ===")
            print(f"WebSocket地址: {uri}")
            print(f"协议版本: {init_result.get('result', {}).get('protocolVersion', 'unknown')}")
            print(f"服务器名称: {init_result.get('result', {}).get('serverInfo', {}).get('name', 'unknown')}")
            print(f"服务器版本: {init_result.get('result', {}).get('serverInfo', {}).get('version', 'unknown')}")
            
            print("\n=== 可用工具 ===")
            tools = tools_result.get('result', {}).get('tools', [])
            for tool in tools:
                print(f"- {tool['name']}: {tool['description']}")
            
            print("\n=== 可用资源 ===")
            resources = resources_result.get('result', {}).get('resources', [])
            for resource in resources:
                print(f"- {resource['name']}: {resource['description']}")
            
            print("\n=== 小智AI配置 ===")
            print("请在小智AI控制台的MCP接入点中添加以下配置:")
            print(f"接入点URL: {uri}")
            print("传输协议: WebSocket")
            print("配置文件: xiaozhi_websocket_mcp_config.json")
            
    except ConnectionRefusedError:
        print("❌ 无法连接到WebSocket服务器")
        print("请确保WebSocket MCP服务器正在运行:")
        print("python3 music_mcp_websocket_server.py")
        return False
    except Exception as e:
        logger.error(f"测试失败: {e}")
        return False
    
    return True

async def main():
    """主函数"""
    print("开始测试免费音乐MCP WebSocket服务器...")
    print("确保WebSocket服务器正在运行在 ws://localhost:8765")
    print()
    
    success = await test_mcp_websocket_server()
    
    if success:
        print("\n🎉 WebSocket MCP服务器测试成功!")
        print("\n📋 下一步操作:")
        print("1. 保持WebSocket服务器运行")
        print("2. 在小智AI控制台添加MCP接入点")
        print("3. 使用语音与小智AI音响交互")
    else:
        print("\n❌ WebSocket MCP服务器测试失败")
        print("请检查服务器是否正常启动")

if __name__ == "__main__":
    asyncio.run(main())