![[../images/Pasted image 20221204165946.png]]
TCP 连接终止时，主机 1 先发送 FIN 报文，主机 2 进入 CLOSE_WAIT 状态，并发送一个 ACK 应答，同时，主机 2 通过 read 调用获得 EOF，并将此结果通知应用程序进行主动关闭操作，发送 FIN 报文。主机 1 在接收到 FIN 报文后发送 ACK 应答，此时主机 1 进入 TIME_WAIT 状态。

主机 1 在 TIME_WAIT 停留持续时间是固定的，是最长分节生命期 MSL（maximum segment lifetime）的两倍，一般称之为 2MSL。

Linux 系统里有一个硬编码的字段，名称为 `TCP_TIMEWAIT_LEN`，其值为 60 秒。也就是说， **Linux 系统停留在 TIME_WAIT 的时间为固定的 60 秒。**
```cpp
#define TCP_TIMEWAIT_LEN (60*HZ) /* how long to wait to destroy TIME-        WAIT state, about 60 seconds   */  
```

过了这个时间之后，主机 1 就进入 CLOSED 状态。

**只有发起连接终止的一方会进入 TIME_WAIT 状态**。

2MSL 的时间是 **从主机 1 接收到 FIN 后发送 ACK 开始计时的**；如果在 TIME_WAIT 时间内，因为主机 1 的 ACK 没有传输到主机 2，主机 1 又接收到了主机 2 重发的 FIN 报文，那么 2MSL 时间将重新计时。因为 2MSL 的时间，目的是为了让旧连接的所有报文都能自然消亡，现在主机 1 重新发送了 ACK 报文，自然需要重新计时，以便防止这个 ACK 报文对新可能的连接化身造成干扰。

