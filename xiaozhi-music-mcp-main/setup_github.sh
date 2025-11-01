#!/bin/bash

# 免费音乐MCP服务器 - GitHub仓库设置脚本

echo "🚀 开始设置GitHub仓库..."

# 检查是否已经是git仓库
if [ ! -d ".git" ]; then
    echo "📁 初始化Git仓库..."
    git init
else
    echo "✅ Git仓库已存在"
fi

# 创建.gitignore文件
echo "📝 创建.gitignore文件..."
cat > .gitignore << EOF
# Python
__pycache__/
*.py[cod]
*$py.class
*.so
.Python
build/
develop-eggs/
dist/
downloads/
eggs/
.eggs/
lib/
lib64/
parts/
sdist/
var/
wheels/
*.egg-info/
.installed.cfg
*.egg

# Virtual Environment
venv/
env/
ENV/

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# OS
.DS_Store
Thumbs.db

# Logs
*.log
EOF

# 添加所有文件
echo "📦 添加文件到Git..."
git add .

# 提交
echo "💾 提交代码..."
git commit -m "🎵 初始提交：免费音乐MCP服务器

✨ 功能特性：
- WebSocket MCP协议支持
- 音乐搜索、播放、控制
- 播放列表管理
- 云端部署支持
- 完全免费

🚀 支持平台：
- Railway.app
- Render.com
- Fly.io
- Docker部署"

echo ""
echo "🎉 Git仓库设置完成！"
echo ""
echo "📋 下一步操作："
echo "1. 在GitHub上创建新仓库（建议名称：xiaozhi-music-mcp）"
echo "2. 复制仓库URL"
echo "3. 运行以下命令推送代码："
echo ""
echo "   git remote add origin <你的仓库URL>"
echo "   git branch -M main"
echo "   git push -u origin main"
echo ""
echo "4. 然后按照 CLOUD_DEPLOYMENT_GUIDE.md 进行云端部署"
echo ""
echo "🌟 推荐使用Railway.app进行部署，最简单快捷！"