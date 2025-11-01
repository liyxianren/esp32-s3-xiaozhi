#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple test for MCP reminder tool - output JSON only
"""

import sys
import os

# Add mcp directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'mcp'))

from reminder import reminder_add

# Test cases
tests = [
    {"delay_seconds": 10, "message": "eat medicine"},
    {"delay_seconds": 0, "message": "sleep", "hour": 20, "minute": 0},
    {"delay_seconds": 0, "message": "wake up", "hour": 7, "minute": 0, "repeat": 10000, "interval": 86400}
]

print("=" * 60)
print("MCP reminder_add tool test")
print("=" * 60)

for i, test in enumerate(tests, 1):
    print(f"\nTest {i}:")
    print(f"Input: {test}")

    result = reminder_add(**test)

    print(f"Output: {result}")
    print(f"  success: {result.get('success')}")
    print(f"  reminder_id: {result.get('reminder_id')}")
    print(f"  delay_seconds: {result.get('delay_seconds')}")
    print(f"  hour: {result.get('hour')}")
    print(f"  minute: {result.get('minute')}")
    print(f"  repeat: {result.get('repeat')}")
    print(f"  interval: {result.get('interval')}")
    print(f"  message: {result.get('message')}")
    print("")
