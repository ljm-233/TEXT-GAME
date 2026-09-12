# TEXT-GAME
our object

time.cpp 负责同步时间 时间接口

log.cpp 创造日志 调用时间接口

main.cpp 主程序

application.cpp 存放主类对象

做个容器解决依赖注入

使用方法

克隆（下载）

建议先创建一个名为coding的文件夹

# 进入你想存放项目的目录
cd ~/coding

# 克隆仓库（推荐 SSH，一次配置永久免密）
git clone git@github.com:ljm-233/TEXT-GAME.git

# 或者用 HTTPS（需要输入 Token）
git clone https://github.com/ljm-233/TEXT-GAME.git

# 进入项目
cd TEXT-GAME

推送（上传）

# 查看改了哪些文件
git status

# 添加所有改动到暂存区
git add .

# 提交，并写清楚改了什么
git commit -m "改了什么写什么"

# 推送到远程 main 分支
git push

# 首次推送如果还没绑定远程分支
git push -u origin main

多人协作时，推送前先拉取

# 拉取远程最新代码并变基
git pull --rebase        

# 如果有冲突，解决后：
git add .

git rebase --continue

git push