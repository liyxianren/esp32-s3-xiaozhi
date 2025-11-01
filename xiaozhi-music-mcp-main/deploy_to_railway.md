# 🚀 一键部署到Railway.app

## 快速部署按钮

点击下面的按钮即可一键部署到Railway：

[![Deploy on Railway](https://railway.app/button.svg)](https://railway.app/template/xiaozhi-music-mcp)

## 手动部署步骤

### 1. 准备GitHub仓库

```bash
# 运行设置脚本
./setup_github.sh

# 在GitHub创建仓库后，推送代码
git remote add origin https://github.com/你的用户名/xiaozhi-music-mcp.git
git branch -M main
git push -u origin main
```

### 2. 部署到Railway

1. **访问 [Railway.app](https://railway.app)**
2. **点击 "Start a New Project"**
3. **选择 "Deploy from GitHub repo"**
4. **授权GitHub并选择您的仓库**
5. **Railway自动检测并部署**

### 3. 获取WebSocket地址

部署完成后：
1. **点击您的项目**
2. **进入 "Settings" → "Domains"**
3. **复制生成的域名**
4. **WebSocket地址格式：** `wss://your-domain.up.railway.app`

### 4. 更新小智AI配置

将原来的配置：
```json
{
  "endpoint": "ws://localhost:8765"
}
```

更新为：
```json
{
  "endpoint": "wss://your-domain.up.railway.app"
}
```

## 🎉 完成！

现在您的音乐服务器将24小时在线运行，无需本地电脑开机！

## 💡 提示

- Railway免费计划提供每月500小时运行时间
- 支持自动重启和健康检查
- 提供免费的HTTPS域名
- 零配置，自动检测Python项目