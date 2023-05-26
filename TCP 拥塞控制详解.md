> 本文主要介绍 TCP 拥塞控制算法，内容多来自网上各个大佬的博客及《TCP/IP 详解》一书，在此基础上进行梳理总结，与大家分享。因水平有限，内容多有不足之处， 敬请谅解。

### **一、TCP 首部格式**

在了解 TCP 的拥塞控制之前，先来看看 TCP 的首部格式和一些基本概念。

TCP 头部标准长度是 20 字节。包含源端口、目的端口、序列号、确认号、数据偏移、保留位、控制位、窗口大小、校验和、紧急指针、选项等。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjygib2UzwEG2dw1icWej6yDr6Plia2TY1vLbbSLroicXgJqKbRzibRDLTCexA/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)TCP 首部格式

#### 1.1 数据偏移（Data Offset）

该字段长 4 位，单位为 4 字节。表示为 TCP 首部的长度。所以 TCP 首部长度最多为 60 字节。

#### 1.2 控制位

目前的 TCP 控制位如下，其中 CWR 和 ECE 用于拥塞控制，ACK、RST、SYN、FIN 用于连接管理及数据传输。

```
CWR：用于 IP 首部的 ECN 字段。ECE 为 1 时，则通知对方已将拥塞窗口缩小。
ECE：在收到数据包的 IP 首部中 ECN 为 1 时将 TCP 首部中的 ECE 设置为 1，表示从对方到这边的网络有拥塞。
URG：紧急模式
ACK：确认
PSH：推送，接收方应尽快给应用程序传送这个数据。没用到
RST：该位为 1 表示 TCP 连接中出现异常必须强制断开连接。
SYN：初始化一个连接的同步序列号
FIN：该位为 1 表示今后不会有数据发送，希望断开连接。
```

#### 1.3 窗口大小（Window）

该字段长度位 16 位，即 TCP 数据包长度位 64KB。可以通过 **Options** 字段的 **WSOPT** 选项扩展到 1GB。

#### 1.4 选项（Options）

受 Data Offset 控制，长度最大为 40 字节。一般 Option 的格式为 TLV 结构：

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyicHgI9SH3Xe8XqPaVNtxbh7S2VecbH8tj044ncXfuG9Lo7PIEDXc71A/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

常见的 TCP Options 有，SACK 字段就位于该选项中。：

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyY1gia7ibpvpUnibML51c5skaZclxVN8auQsiaw7yRgFkyDrpwypVyw0xicw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)TCP Options

#### 1.5 SACK 选项

SACK 包括了两个 TCP 选项，一个选项用于标识是否支持 SACK，是在 TCP 连接建立时发送；另一种选项则包含了具体的 SACK 信息。

1. SACK_Permitted 选项，该选项只允许在 TCP 连接建立时，有 SYN 标志的包中设置，也即 TCP 握手的前两个包中，分别表示通信的两方各自是否支持 SACK。

```
TCP SACK-Permitted Option:
Kind: 4
Length: Variable
+----------+----------+
| Kind=4   | Length=2 |
+----------+----------+
```

1. SACK(选择性确认) 选项位于 Options 中。该选项参数告诉对方已经接收到并缓存的不连续的数据块，发送方可根据此信息检查究竟是哪些块丢失，从而发送相应的数据块。受 TCP 包长度限制，TCP 包头最多包含四组 SACK 字段。

```
TCP SACK Option:
Kind: 5
Length: Variable
                +--------+--------+
                                | Kind=5 | Length |
              +--------+--------+--------+--------+
              |      Left Edge Of lst Block       |
              +--------+--------+--------+--------+
              |     Right Edge Of lst Block       |
              +--------+--------+--------+--------+
              |                   .  .  .         |
              +--------+--------+--------+--------+
              |      Left Edge Of nth Block       |
              +--------+--------+--------+--------+
              |     Right Edge Of nth Block       |
          +--------+--------+--------+--------+
```

1. SACK 的工作原理

   如下图所示， 接收方收到 500-699 的数据包，但没有收到 300-499 的数据包就会回 SACK(500-700) 给发送端，表示收到 500-699 的数据。

![](images/Pasted%20image%2020230526215512.png)

### **二、滑动窗口和包守恒原则**

#### 2.1 滑动窗口

为了解决可靠传输以及包乱序的问题，TCP 引入滑动窗口的概念。在传输过程中，client 和 server 协商接收窗口 rwnd，再结合拥塞控制窗口 cwnd 计算滑动窗口 swnd。在 Linux 内核实现中，滑动窗口 cwnd 是以包为单位，所以在计算 swnd 时需要乘上 mss（最大分段大小）。
![](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkUu2X6z3TFEhncvXK2uuVs1LZk5rR8JHteUIWjFfxRNLEvdibfiaQsATVyUv0GyV4GP/0?wx_fmt=svg)

如下图所示滑动窗口包含 4 部分：

- 已收到 ack 确认的数据；
- 已发还没收到 ack 的；
- 在窗口中还没有发出的（接收方还有空间）；
- 窗口以外的数据（接收方没空间）。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyvz6qK5QDL8VqCucsHlF5wWG8zQKbU9s8ynM3GZwH7nDIu7xhX0CHEA/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)TCP滑动窗口

滑动后的示意图如下（收到 36 的 ack，并发出了 46-51 的数据）：

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyn202CPYXaTibxfyXEvDcypibSYYBCqS1aoIPFzCPoLMcLzOib100PPvlw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)TCP滑动后示意图

#### 2.2 包守恒原则

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjybPQLs6zb6V3NHd0cibcoCImI0AqkY87vms5ic7v7liarak0xNicwf0zWUw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)包守恒原则

TCP 维护一个发送窗口，估计当前网络链路上能容纳的数据包数量，希望在有数据可发的情况下，回来一个确认包就发出一个数据包，总是保持发送窗口那么多包在网络中流动。

传输的理想情况是要同时达到最大的吞吐量和最小的往返延迟，要达到这个目的，连接必须同时满足两个条件：

- 以链路瓶颈带宽 BtlBw 发包 （带宽利用率最高）
- 保证链路中没有缓存队列（延迟最低）

包守恒原则是拥塞控制的基础。

### **三、TCP 重传机制**

本文重点介绍 TCP 拥塞控制相关，传输流程不在该范围之内，有兴趣的同学可以查阅相关文档。不过 TCP 重传逻辑和拥塞控制中的`快速重传`有关，所以在真正介绍拥塞控制算法之前，先来了解下 TCP 重传逻辑。

#### 3.1 超时重传 [RFC2988]

RTT（Round Trip Time）由三部分组成：链路的传播时间（propagation delay）、末端系统的处理时间、路由器缓存中的排队和处理时间（queuing delay）。TCP 发送端得到了基于时间变化的 RTT 测量值，就能据此设置 RTO。



当一个重传报文段被再次重传时，则增大 RTO 的退避因子 。通常情况下 值为 1，多次重传 加倍增长为 2，4，8 等。通常 不能超过最大退避因子，Linux 下 RTO 不能超过 TCP_RTO_MAX（默认为 120s）。一旦收到相应的 ACK， 重置为 1。

下面介绍几种常用的 RTT 算法。

##### **3.1.1 rtt 经典算法 [RFC793]**

1）首先，先采样 RTT，记下最近几次的 RTT 值。2）然后做平滑计算 SRTT（ Smoothed RTT）。公式为：（其中的 α 取值在 0.8 到 0.9 之间，这个算法英文叫 Exponential weighted moving average，中文叫：加权移动平均）

![]( https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXk2mvA0mB1Ypk9dqyibR7FfiaV9tJFPGo3AhACC72ZFUyoHWfVIBKPVtSTqpoBa0GRe6/0?wx_fmt=svg )


3）开始计算 RTO。公式如下：
![](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkiarhwfF4RHiapLLpn28LtL9CjwjojUmbway5WPhNS4QLlDcfzPxNnHXib0XlkFMAyiap/0?wx_fmt=svg)


其中：

- UBOUND 是最大的 timeout 时间，上限值；
- LBOUND 是最小的 timeout 时间，下限值；
- β 值一般在 1.3 到 2.0 之间。

该算法的问题在于**重传时，是用重传的时间还是第一次发数据的时间和 ACK 回来的时间计算 RTT 样本值，另外，delay ack 的存在也让 rtt 不能精确测量**。

##### **3.1.2 rtt 标准算法（Jacobson / Karels 算法）**

该算法 [RFC6298] 特点是引入了最新的 RTT 的采样



和平滑过的的差值做参数来计算。公式如下： 1.计算平滑 RTT





2.计算平滑 RTT 和真实的差距（加权移动平均）



3.计算 RTO



4.考虑到时钟粒度，给 RTO 设置一个下界。



这里



为计时器粒度，1000ms 为整个 RTO 的下届值。因此 RTO 至少为 1s。在 Linux 下，α = 0.125，β = 0.25， μ = 1，∂ = 4 ——这就是算法中的“调得一手好参数”，nobody knows why, it just works…）（Linux 的源代码在：tcp_rtt_estimator）。



5.在首个 SYN 交换前，TCP 无法设置 RTO 初始值。根据 [RFC6298]，RTO 初始值为 1s，而初始 SYN 报文段采用的超时间隔为 3s。当计算出首个 RTT 测量结果 ，则按如下方法进行初始化：



##### **3.1.3 Karn 算法**

在 RTT 采样测量过程中，如果一个数据包初传后，RTO 超时重传，接着收到这个数据包的 ACK 报文，那么这个 ACK 报文是对应初传 TCP 报文还是对应重传 TCP 报文呢？这个问题就是重传二义性的问题。当没有使用 TSOPT 选项，单纯的 ACK 报文并不会指示对应初传包还是重传包，因此就会发生这个问题。TCP Karn 算法是对经典算法在重传二义性问题上的一种改进。该算法分两部分。1) 当出现超时重传，接收到重传数据的确认信息时不更新 RTT。即忽略了重传部分，避免重传二义性问题。2) 对该数据之后的包采取退避策略，仅当接收到未经重传的数据时，该 SRTT 才用于计算 RTO。

因为单纯忽略重传数据时，如果在某一时间，网络闪动，突然变慢了，产生了比较大的延时，这个延时导致要重转所有的包（因为之前的 RTO 很小），但因为重转的不算，RTO 就不会被更新，这是个灾难。而且一发生重传就对现有的 RTO 值翻倍的方式也很难估算出准确的 RTT。

3.1.4 TCP 时间戳选项（TSOPT）

根据 [RFC1323]，TSOPT 主要有两个用途，一个是 RTTM(round-trip time measurement)，即根据 ACK 报文中的这个选项测量往返时延；另外一个用途是 PAWS(protect against wrapped sequence numbers)，即防止同一个连接的系列号重叠。另外还有一些其他的用途，如 SYN-cookie、Eifel Detection Algorithm 等等。TSOPT 为 Options 选项中一种，格式如下：

```
   Kind: 8
   Length: 10 bytes
   +-------+-------+---------------------+---------------------+
   Kind=8 |  10   |   TS Value (TSval)  |TS Echo Reply (TSecr)|
   +-------+-------+---------------------+---------------------+
   1       1              4                     4
```

**RTT 测量（RTTM）**

当使用这个选项的时候，发送方在 TSval 处放置一个时间戳，接收方则会把这个时间通过 TSecr 返回来。因为接收端并不会处理这个 TSval 而只是直接从 TSecr 返回来，因此不需要双方时钟同步。这个时间戳一般是一个单调增的值，[RFC1323]建议这个时间戳每秒至少增加 1。其中在初始 SYN 包中因为发送方没有对方时间戳的信息，因此 TSecr 会以 0 填充，TSval 则填充自己的时间戳信息。

**防回绕序列号（PAWS）**

TCP 判断数据是新是旧的方法是检查数据的序列号是否位于 sun.una 到 sun.una + 的范围内，而序列号空间的总大小为 ，即约 4.29G。在万兆局域网中，4.29G 字节数据回绕只需几秒钟，这时 TCP 就无法准确判断数据的新旧。

PAWS 假设接收到的每个 TCP 包中的 TSval 都是随时间单调增的，基本思想就是如果接收到的一个 TCP 包中的 TSval 小于刚刚在这个连接上接收到的报文的 TSval，则可以认为这个报文是一个旧的重复包而丢掉。时间戳回绕的速度只与对端主机时钟频率有关。Linux 以本地时钟计数（jiffies）作为时间戳的值，假设时钟计数加 1 需要 1ms，则需要约 24.8 天才能回绕一半，只要报文的生存时间小于这个值的话判断新旧数据就不会出错。

**SYN Cookie 的选项信息**

TCP 开启 SYNCookie 功能时由于 Server 在收到 SYN 请求后不保存连接，故 SYN 包中携带的选项（WScale、SACK）无法保存，当 SYN Cookie 验证通过、新连接建立之后，这些选项都无法开启。

使用时间戳选项就可以解决上述问题。将 WScale 和 SACK 选项信息编码进 32 bit 的时间戳值中，建立连接时会收到 ACK 报文，将报文的时间戳选项的回显信息解码就可以还原 WScale 和 SACK 信息。

#### 3.2 Fast Retransmit（快速重传）

快速重传算法概括为：TCP 发送端在观测到至少 dupthresh（一般为 3） 个重复 ACK 后，即重传可能丢失的数据分组，而不需等待重传计时器超时。

##### **3.2.1 SACK 重传**

1. 未启用 SACK 时，TCP 重复 ACK 定义为收到连续相同的 ACK seq。[RFC5681]
2. 启用 SACK 时，携带 SACK 的 ACK 也被认为重复 ACK。[RFC6675]

如下图所示（绿色为已发送并且被 ack 的数据包，黄色表示已发送还未确认的数据包，浅绿色为被 Sack 确认的数据包，蓝色表示还未发送的数据包），设 dupthresh = 3，SACKed_count = 6，从 unAcked 包开始的 SACKed_count - dupthresh 个数据包，即 3 个数据包会被标记为 LOST。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjywhDU4Q2wN49dZIaN8IBqnNDNWBlTC0hstEeTGT1RW0MQMHx3W8Licpw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

![图片](data:image/svg+xml,%3C%3Fxml version='1.0' encoding='UTF-8'%3F%3E%3Csvg width='1px' height='1px' viewBox='0 0 1 1' version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'%3E%3Ctitle%3E%3C/title%3E%3Cg stroke='none' stroke-width='1' fill='none' fill-rule='evenodd' fill-opacity='0'%3E%3Cg transform='translate(-249.000000, -126.000000)' fill='%23FFFFFF'%3E%3Crect x='249' y='126' width='1' height='1'%3E%3C/rect%3E%3C/g%3E%3C/g%3E%3C/svg%3E)拥塞窗口状态

记分板状态如下，红色表示该数据包丢失：

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyAZZBTksI2fKQbf1q8Yd8j3Mo0FialvibCj7uSjmjhJtIibsNCnZThgtIg/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

![图片](data:image/svg+xml,%3C%3Fxml version='1.0' encoding='UTF-8'%3F%3E%3Csvg width='1px' height='1px' viewBox='0 0 1 1' version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'%3E%3Ctitle%3E%3C/title%3E%3Cg stroke='none' stroke-width='1' fill='none' fill-rule='evenodd' fill-opacity='0'%3E%3Cg transform='translate(-249.000000, -126.000000)' fill='%23FFFFFF'%3E%3Crect x='249' y='126' width='1' height='1'%3E%3C/rect%3E%3C/g%3E%3C/g%3E%3C/svg%3E)记分板状态

##### **3.2.2 FACK 重传**

FACK 是 SACK 的一个激进版本，它拥有标准 SACK 算法的一切性质，除此之外，它**假设网络不会使数据包乱序**，因此收到最大的被 SACK 的数据包之前，FACK 均认为是丢失的。FACK 模式下，重传时机为 **被 SACKed 的包数 + 空洞数 > dupthresh**。

如下图所示，设 dupthresh = 3，FACKed_count = 12，从 unACKed 包开始的 FACKed_count

- dupthresh 个数据包，即 9 个包会被标记为 LOST。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyzqnTXHC1dDzAYFqAShaKPWCI1kFwSxFviaDemVOe9DR7D2diaYHo6Gmg/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

拥塞窗口状态

记分板状态如下，红色表示该数据包丢失：

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyj1MkNljbkF5lwM0MsaVRibriaT8BnzH6lp4Lib7ibP3bxZgykicEZjiaZUaA/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)记分板状态

##### **3.2.3 RACK 重传**

**基本思路** 如果数据包 p1 在 p2 之前发送，没有收到 p1 的确认，当收到 p2 的 Sack 时，推断 p1 丢包。**算法简介** 每一个 skb 记录发送时间 xmit_time，传输控制块维护全局变量：rack.xmit_time，rack.reo_wnd。rack.xmit_time 是接收方已经收到的最新的那个 skb 的发送时间，rack.reo_wnd 是乱序的时间窗口大小。

1.每次收到新的 ACK 后，更新 reo_wnd，其中 rtt_min 为固定时间窗口的 rtt 最小值。

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkHgbIpQMlZvXIGIHb13YA3dEoXPdolwNn0nWT22diaib2u162NWvITIVLuQM540ZLiay/0?wx_fmt=svg)

2.每当收到一个 ACK 或者 SACK 的时候，更新 rack.xmit_time。再去遍历发送队列上已经发送但还没有收到确认的数据包（skb），如果满足如下条件，那么标记这个数据包丢了。

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXk06iakvdKawtDatcJLzvBYrsmmtsy8hZCB5sMpFObozB9TGnZMRaJtdTcndX7I25zg/0?wx_fmt=svg)

3.如果没有收到确认，那么就用定时器每个一段时间看看有哪些包丢了，如果满足如下条件，那么把这个 skb 标记为已经丢了：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkjlWg8lpAFeQZpUIWrPxeBk8HsR7HfMIh7bmMKhqN7yMKCHH924TVicBf1ibqa4fS4Y/0?wx_fmt=svg)

注：目前 linux 内核中只实现了第一种判断方法，定时器还没有实现，这样的话就还没有解决对于尾部包丢失的问题。

##### **3.2.4 乱序检测**

乱序检测的目的时探测网络是否发生重排导致的丢包，并以此来更新 dupthresh 值。只要能收到一个 ACK 或者 SACK，其序列号位于当前已经被 ACK 或 SACK 的数据包最大的序列号之前，就说明网络发生了重排造成了乱序，此时如果涉及的数据包大小大于当前能容忍的网络乱序值，即 dupthresh 值，就说明网络乱序加重了，此时应该更新 dupthresh 值。之所以保持 dupthresh 的值递增，是考虑其初始值 3 只是一个经验值，既然真实检测到乱序，如果其值比 3 小，并不能说明网络的乱序度估计偏大，同时 TCP 保守地递增乱序度，也是为了让快速重传的进入保持保守的姿态，从而增加友好性。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyBcw9paO1xlK490SIAjLhoy69fV2ohAljQ1slyGpsBj8wjyxb1tjx6A/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

一旦发现 dupthresh 值更新的情形，FACK 的假设便不成立，必须在连接内永久禁用 FACK 算法。

#### 3.3 Early Retransmit for TCP（ER 机制）

**要解决的问题:** 当无法收到足够的 dupack 时，TCP 标准的 Fast Retransmit 机制无法被触发，只能等待 RTO 超时才能进行丢包的重传。而 RTO 超时不管是时间等待代价，还是性能损耗代价都很大。

**解决方法:** 检测出无法收到足够 dupack 的场景，进而降低 dupack threshold 来触发快速重传。从而避免等待 RTO 超时重传，对性能造成较大的损耗。

总结出现 dupack 不够的情况：a. cwnd 较小 b. 发送窗口里大量的数据包都被丢失了 c.在数据发送的尾端发生丢包时

但是，上面各种极端的 case 有共同的特点：m. 无法产生足够的 dupack n.没有新的数据包可以发送进入网络，ER 机制就是在判断条件 m 和 n 都成立后，选择降低触发 Fast Retransmit 的阈值，来避免只能通过 RTO 超时重传的问题。

#### 3.4 TCP Tail Loss Probe（TLP 算法）

ER 算法解决了 dupack 较少时无法触发快速重传的问题，但当发生尾丢包时，由于尾包后没有更多数据包，也就无法触发 dupack。TLP 算法通过发送一个 loss probe 包，以产生足够的 SACK/FACK 数据包来触发重传。TLP 算法会在 TCP 还是 Open 态时设置一个 Probetimeout（PTO），当链路中有未被确认的数据包，同时在 PTO 时间内未收到任何 ACK，则会触发 PTO 超时处理机制。TLP 会选择传输序号最大的一个数据包作为 tail loss probe 包，这个序号最大的包可能是一个可以发送的新的数据包，也可能是一个重传包。TLP 通过这样一个 tail loss probe 包，如果能够收到相应的 ACK，则会触发 FR 机制，而不是 RTO 机制。

#### 3.5 伪超时与重传

在很多情况下，即使没有出现数据丢失也可能引发重传。这种不必要的重传称为伪重传，其主要造成原因是伪超时，即过早判定超时，其他因素如包失序、包重复，或 ACK 丢失也可能导致该现象。在实际 RTT 显著增长，超过当前 RTO 时，可能出现伪超时。在下层协议性能变化较大的环境中（如无线环境），这种情况出现得比较多。

TCP 为处理伪超时问题提出了许多方法。这些方法通常包含检测算法与响应算法。检测算法用于判断某个超时或基于计时器的重传是否真实，一旦认定出现伪超时则执行响应算法，用于撤销或减轻该超时带来的影响。检测算法包含 DSACK 、Eifel 检测算法、迁移 RTO 恢复算法(F-RTO) 三种。

##### **3.5.1 DSACK 扩展**

DSACK 的主要目的是判断何时的重传是不必要的，并了解网络中的其他事项。通过 DSACK 发送端至少可以推断是否发生了包失序、 ACK 丢失、包重复或伪重传。D-SACK 使用了 SACK 的第一个段来做标志， a. 如果 SACK 的第一个段的范围被 ACK 所覆盖，那么就是 D-SACK。b.如果 SACK 的第一个段的范围被 SACK 的第二个段覆盖，那么就是 D-SACK。RFC2883]没有具体规定发送端对 DSACK 怎样处理。[RFC3708]给出了一种实验算法，利用 DSACK 来检测伪重传，响应算法可采用 Eifel 响应算法。

##### **3.5.2 Eifel 检测算法 [RFC3522]**

实验性的 Eifel 检测算法利用了 TCP 的 TSOPT 来检测伪重传。在发生超时重传后，Eifel 算法等待接收下一个 ACK，若为针对第一次传输（即原始传输）的确认，则判定该重传是伪重传。

与 DSACK 的比较：利用 Eifel 检测算法能比仅采用 DSACK**更早检测到伪重传行为**，因为它判断伪重传的 ACK 是在启动丢失恢复之前生成的。相反， DSACK 只有在重复报文段到达接收端后才能发送，并且在 DSACK 返回至发送端后才能有所响应。及早检测伪重传更为有利，它能使发送端有效避免“回退 N”行为。

##### **3.5.3 迁移 RTO 恢复算法(F-RTO)**

前移 RTO 恢复（Forward-RTO Recovery，F-RTO）[RFC5682]是检测伪重传的标准算法。它不需要任何 TCP 选项，因此只要在发送端实现该方法后，即使针对不支持 TSOPT 的接收端也能有效地工作。该算法**只检测由重传计时器超时引发的伪重传**，对之前提到的其他原因引起的伪重传则无法判断。

F-RTO 的工作原理如下：1. F-RTO 会修改 TCP 的行为，在超时重传后收到第一个 ACK 时，TCP 会发送新（非重传）数据，之后再响应后一个到达的 ACK。2.如果其中有一个为重复 ACK，则认为此次重传没问题。3. 如果这两个都不是重复 ACK，则表示该重传是伪重传。4.重复 ACK 是在接收端收到失序的报文段产生的。这种方法比较直观。如果新数据的传输得到了相应的 ACK，就使得接收端窗口前移。如果新数据的发送导致了重复 ACK，那么接收端至少有一个或更多的空缺。这两种情况下，接收新数据都不会影响整体数据的传输性能。

### **四、拥塞状态机**

TCP 通过拥塞状态机来决定收到 ACK 时 cwnd 的行为（增长或者降低）。TCP 拥塞状态机有 `Open`,`Disorder`,`Recovery`,`Lost`和`CWR`五种状态。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyKozDmFbRqZnNlStUO5jIuSSxUcvB7spwKFlVFicn5icxAc3moDfI5REw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)TCP拥塞控制状态机

#### 4.1 Open

当网络中没有发生丢包，也就不需要重传，sender 按照`慢启动`或者`拥塞避免算法`处理到来的 ACK。

#### 4.2 Disorder

当 sender 检测到 dupack 或者 SACK，将会转移到 Disorder 状态，当处在这个这个状态中时，cwnd 将维持不变。每收到一个 dupack 或 SACK，发送方将发送一个新包。

#### 4.3 CWR

当 sender 收到 ACK 包含显示拥塞通知（ECN），这个 ECN 由路由器写在 IP 头中，告诉 TCP sender 网络拥塞，sender 不会立马降低 cwnd，而是根据`快速恢复算法`进行降窗，直到减为之前的一半。当 cwnd 正在减小 cwnd，网络中有没有重传包时，这个状态就叫 CWR，CWR 可以被 Recovery 或者 Loss 中断。

#### 4.4 Recovery

当 sender 因为`快速重传机制`触发丢包时，sender 会重传第一个未被 ACK 的包，并进入 Recovery 状态。在 Recovery 状态期间，cwnd 的处理同 CWR 大致一样，要么重传标记了 lost 的包，要么根据保守原则发送新包。直到网络中所有的包都被 ACK，才会退出 Recovery 进入 Open 状态，Recovery 状态可以被 loss 状态打断。

1. 经典 Reno 模式（非 SACK 模式）下， 时退出 Recovery 状态。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjy7Yb1dQlLYmP3FGrABQavlAbth3Az5SQyv4tdQykf8C6TrLxXajTIHg/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

Reno 模式退出Recovery状态

如上图，数据包 A、B、C 可能没有丢失只是被延迟接收，然而没有 SACK 机制下无法判断是 A、B、C 的重传触发的 3 次重复 ACK 还是新数据 1、2、3 丢失 1（或者 1、2 或者 1、2、3）导致的重复 ACK，两种情况均会马上把拥塞状态机带入到 Recovery 状态，然而前一种是不必要的。如果在 SND_UNA== SND_HIGH 即转为 Open 态，那么当发生上述 1、2、3 的延迟到达后，紧接着的 Recovery 状态会再次将拥塞窗口减半，最终降低到一个极低值。

1. SACK/FACK 模式下， 时退出 Recovery 状态。 因为即使发生经典 Reno 模式下的 A、B、C 失序到达，由于存在 SACK 信息，状态机会将此三个重复 ACK 记为三个重复的 DSACK，在 SACK 模式下，判断是否进入 Recovery 状态的条件是被 SACK 的数据包数量大于 dupthresh，而 DSACK 不被计入到 SACKed 数量中。FACK 模式下只影响进入 Recovery 状态的时机，其核心仍建立在 SACK 之上，所以不影响退出的时机。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjydf1j7CwACgib8oNlgob9icnwgudicltIWLhKsIEWNtkgZj3vmTVDVez5Q/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

SACK/FACK模式退出Recovery状态

#### 4.5 Loss

当 RTO 后，TCPsender 进入 Loss 状态，所有在网络中的包被标记为 lost，cwnd 重置为 1，通过 slow start 重新增加 cwnd。Loss 与 Recovery 状态的不同点在于，cwnd 会重置为 1，但是 Recovery 状态不会，Recovery 状态下拥塞控制通过`快速恢复`算法逐步降低 cwnd 至 sshthresh。Loss 状态不能被其它任何状态中断，只有当网络中所有的包被成功 ACK 后，才能重新进入 Open 状态。

### **五、拥塞控制**

拥塞的发生是因为路由器缓存溢出，拥塞会导致丢包，但丢包不一定触发拥塞。拥塞控制是快速传输的基础。一个拥塞控制算法一般包括`慢启动算法`、`拥塞避免算法`、`快速重传算法`、`快速恢复算法`四部分。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjySa0wwiad3DudI8Bd64TcFaRLAJAvwySibicxZkNLOFVp87KaNClELich6w/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)拥塞窗口示意图

#### 5.1 慢启动算法

不同拥塞算法慢启动的逻辑有所不同，经典的 NewReno 慢启动的算法如下：

1. 连接建好的开始先初始化 cwnd = 10，表明可以传 10 个 MSS 大小的数据。
2. 每当收到一个 ACK，cwnd 加 1。这样每当过了一个 RTT，cwnd 翻倍，呈指数上升。
3. 还有一个 ssthresh（slow start threshold），是一个上限。当 cwnd >=ssthresh 时，就会进入“拥塞避免算法”。

Linux 3.0 后采用了 [Google 的论文《An Argument for Increasing TCP’s Initial Congestion Window》](https://static.googleusercontent.com/media/research.google.com/zh-CN//pubs/archive/36640.pdf)的建议——把 cwnd 初始化成了 10 个 MSS。而 Linux 3.0 以前，比如 2.6，Linux 采用了[RFC3390] 的建议，cwnd 跟 MSS 的值来变，如果 MSS < 1095，则 cwnd = 4；如果 MSS >2190，则 cwnd = 2；其它情况下，则是 3。

#### 5.2 拥塞避免算法

当 cwnd 增长到 sshthresh 时，就会进入“拥塞避免算法”。拥塞避免算法下 cwnd 成线性增长，即每经过一个往返时间 RTT 就把发送方的拥塞窗口 cwnd 加 1，而不是加倍。这样就可以避免拥塞窗口快速增长的问题。

```
每收到一个 ack 时 cwnd 的变化：
cwnd = cwnd + 1 / cwnd
```

#### 5.3 快速重传算法

快速重传算法主要用于丢包检测，以便能更快重传数据包，更早的调整拥塞状态机状态，从而达到持续升窗的目的。具体重传策略见第三节 `重传机制`。

#### 5.4 快速恢复算法

当检测到丢包时，TCP 会触发快速重传并进入降窗状态。该状态下 cwnd 会通过快速恢复算法降至一个合理值。从历史发展来看，分为四个个阶段。

##### **5.4.1 BSD 初始版本**

1. 收到 3 次重复 ACK，ssthresh 设为 cwnd/2，cwnd = cwnd / 2 + 3;
2. 每收到一个重复 ACK，窗口值加 1;
3. 收到非重复 ACK，窗口设为 ssthresh，退出

优点：在快速恢复期间，可以尽可能多的发送数据缺点：由于快速恢复未完成，尽可能多发送可能会加重拥塞。#### 5.4.2 [RFC3517]版本 1) 收到 3 次重复 ACK，ssthresh 设为 cwnd/2，cwnd = cwnd / 2 + 3; 2)**每收到一个重复 ACK，窗口值加 1/cwnd**; 3) 收到非重复 ACK，窗口设为 ssthresh，退出。

优点：在快速恢复期间，可以尽少量的发送数据（有利于拥塞恢复），且在快速恢复时间段的最后阶段，突发有利于抢带宽。

缺点：快速恢复末期的突发不利于公平性。

##### **5.4.2 Linux rate halving 算法**

Linux 上并没有按照 [RFC3517] 标准实现，而是做了一些修改并运用到内核中。

1.收到 3 次重复 ACK，**sshthresh 设置为 cwnd/2，窗口维持不变**。2.每收到两个 ACK（不管是否重复），窗口值减 1：cwnd = cwnd - 1。3.新窗口值取 cwnd = MIN(cwnd, in_flight+1)。4.直到退出快速恢复状态，cwnd = MIN(cwnd, ssthresh)。

优点：在快速恢复期间，取消窗口陡降过程，可以更平滑的发送数据 缺点：降窗策略没有考虑 PIPE 的容量特征，考虑一下两点：

a.如果快速恢复没有完成，窗口将持续下降下去 b.如果一次性 ACK/SACK 了大量数据，in_flight 会陡然减少，窗口还是会陡降，这不符合算法预期。

##### **5.4.3 prr 算法 [RFC6937]**

PRR 算法是最新 Linux 默认推荐的快速恢复算法。prr 算法是一种按比例降窗的算法，其最终效果是：

1.在快速恢复过程中，拥塞窗口非常平滑地向 ssthresh 收敛；2.在快速恢复结束后，拥塞窗口处在 ssthresh 附近

PRR 降窗算法实时监控以下的变量：in_flight：它是窗口的一个度量，in_flight 的值任何时候都不能大于拥塞窗口的大小。

prr_delivered：本次收到 ACK 进入降窗函数的时候，一共被 ACK 或者 SACK 的数据段数量。它度量了本次从网络中清空了哪些数据段，从而影响 in_flight。

prr_out：进入快速恢复状态后已经被发送了多少数据包。在 transmit 例程和 retransmit 例程中递增。

to_be_out：当前还可以再发多少数据包。

**算法原理**根据**数据包守恒原则**，能够发送的数据包总量是本次接收到的 ACK 中确认的数据包的总量，然而处在拥塞状态要执行的并不是这个守恒发送的过程，而是降窗的过程，因此需要在被 ACK 的数据包数量和可以发送的数据包数量之间打一个折扣，PRR 希望达到的目标是：

<svg xmlns="http://www.w3.org/2000/svg" role="img" focusable="false" viewBox="0 -750 13107.6 1000" aria-hidden="true" style="color: rgb(62, 71, 83);vertical-align: -0.566ex;width: 29.655ex;height: 2.262ex;max-width: 300% !important;"><g stroke="currentColor" fill="currentColor" stroke-width="0" transform="matrix(1 0 0 -1 0 0)"><g data-mml-node="math"><g data-mml-node="mi"><path data-c="73" d="M131 289Q131 321 147 354T203 415T300 442Q362 442 390 415T419 355Q419 323 402 308T364 292Q351 292 340 300T328 326Q328 342 337 354T354 372T367 378Q368 378 368 379Q368 382 361 388T336 399T297 405Q249 405 227 379T204 326Q204 301 223 291T278 274T330 259Q396 230 396 163Q396 135 385 107T352 51T289 7T195 -10Q118 -10 86 19T53 87Q53 126 74 143T118 160Q133 160 146 151T160 120Q160 94 142 76T111 58Q109 57 108 57T107 55Q108 52 115 47T146 34T201 27Q237 27 263 38T301 66T318 97T323 122Q323 150 302 164T254 181T195 196T148 231Q131 256 131 289Z"></path></g><g data-mml-node="mtext" transform="translate(469, 0)"><path data-c="73" d="M295 316Q295 356 268 385T190 414Q154 414 128 401Q98 382 98 349Q97 344 98 336T114 312T157 287Q175 282 201 278T245 269T277 256Q294 248 310 236T342 195T359 133Q359 71 321 31T198 -10H190Q138 -10 94 26L86 19L77 10Q71 4 65 -1L54 -11H46H42Q39 -11 33 -5V74V132Q33 153 35 157T45 162H54Q66 162 70 158T75 146T82 119T101 77Q136 26 198 26Q295 26 295 104Q295 133 277 151Q257 175 194 187T111 210Q75 227 54 256T33 318Q33 357 50 384T93 424T143 442T187 447H198Q238 447 268 432L283 424L292 431Q302 440 314 448H322H326Q329 448 335 442V310L329 304H301Q295 310 295 316Z"></path><path data-c="74" d="M27 422Q80 426 109 478T141 600V615H181V431H316V385H181V241Q182 116 182 100T189 68Q203 29 238 29Q282 29 292 100Q293 108 293 146V181H333V146V134Q333 57 291 17Q264 -10 221 -10Q187 -10 162 2T124 33T105 68T98 100Q97 107 97 248V385H18V422H27Z" transform="translate(394, 0)"></path></g><g data-mml-node="mi" transform="translate(1252, 0)"><path data-c="68" d="M137 683Q138 683 209 688T282 694Q294 694 294 685Q294 674 258 534Q220 386 220 383Q220 381 227 388Q288 442 357 442Q411 442 444 415T478 336Q478 285 440 178T402 50Q403 36 407 31T422 26Q450 26 474 56T513 138Q516 149 519 151T535 153Q555 153 555 145Q555 144 551 130Q535 71 500 33Q466 -10 419 -10H414Q367 -10 346 17T325 74Q325 90 361 192T398 345Q398 404 354 404H349Q266 404 205 306L198 293L164 158Q132 28 127 16Q114 -11 83 -11Q69 -11 59 -2T48 16Q48 30 121 320L195 616Q195 629 188 632T149 637H128Q122 643 122 645T124 664Q129 683 137 683Z"></path></g><g data-mml-node="mtext" transform="translate(1828, 0)"><path data-c="72" d="M36 46H50Q89 46 97 60V68Q97 77 97 91T98 122T98 161T98 203Q98 234 98 269T98 328L97 351Q94 370 83 376T38 385H20V408Q20 431 22 431L32 432Q42 433 60 434T96 436Q112 437 131 438T160 441T171 442H174V373Q213 441 271 441H277Q322 441 343 419T364 373Q364 352 351 337T313 322Q288 322 276 338T263 372Q263 381 265 388T270 400T273 405Q271 407 250 401Q234 393 226 386Q179 341 179 207V154Q179 141 179 127T179 101T180 81T180 66V61Q181 59 183 57T188 54T193 51T200 49T207 48T216 47T225 47T235 46T245 46H276V0H267Q249 3 140 3Q37 3 28 0H20V46H36Z"></path><path data-c="65" d="M28 218Q28 273 48 318T98 391T163 433T229 448Q282 448 320 430T378 380T406 316T415 245Q415 238 408 231H126V216Q126 68 226 36Q246 30 270 30Q312 30 342 62Q359 79 369 104L379 128Q382 131 395 131H398Q415 131 415 121Q415 117 412 108Q393 53 349 21T250 -11Q155 -11 92 58T28 218ZM333 275Q322 403 238 411H236Q228 411 220 410T195 402T166 381T143 340T127 274V267H333V275Z" transform="translate(392, 0)"></path><path data-c="73" d="M295 316Q295 356 268 385T190 414Q154 414 128 401Q98 382 98 349Q97 344 98 336T114 312T157 287Q175 282 201 278T245 269T277 256Q294 248 310 236T342 195T359 133Q359 71 321 31T198 -10H190Q138 -10 94 26L86 19L77 10Q71 4 65 -1L54 -11H46H42Q39 -11 33 -5V74V132Q33 153 35 157T45 162H54Q66 162 70 158T75 146T82 119T101 77Q136 26 198 26Q295 26 295 104Q295 133 277 151Q257 175 194 187T111 210Q75 227 54 256T33 318Q33 357 50 384T93 424T143 442T187 447H198Q238 447 268 432L283 424L292 431Q302 440 314 448H322H326Q329 448 335 442V310L329 304H301Q295 310 295 316Z" transform="translate(836, 0)"></path></g><g data-mml-node="mi" transform="translate(3058, 0)"><path data-c="68" d="M137 683Q138 683 209 688T282 694Q294 694 294 685Q294 674 258 534Q220 386 220 383Q220 381 227 388Q288 442 357 442Q411 442 444 415T478 336Q478 285 440 178T402 50Q403 36 407 31T422 26Q450 26 474 56T513 138Q516 149 519 151T535 153Q555 153 555 145Q555 144 551 130Q535 71 500 33Q466 -10 419 -10H414Q367 -10 346 17T325 74Q325 90 361 192T398 345Q398 404 354 404H349Q266 404 205 306L198 293L164 158Q132 28 127 16Q114 -11 83 -11Q69 -11 59 -2T48 16Q48 30 121 320L195 616Q195 629 188 632T149 637H128Q122 643 122 645T124 664Q129 683 137 683Z"></path></g><g data-mml-node="TeXAtom" data-mjx-texclass="ORD" transform="translate(3634, 0)"><g data-mml-node="mo"><path data-c="2F" d="M423 750Q432 750 438 744T444 730Q444 725 271 248T92 -240Q85 -250 75 -250Q68 -250 62 -245T56 -231Q56 -221 230 257T407 740Q411 750 423 750Z"></path></g></g><g data-mml-node="mtext" transform="translate(4134, 0)"><path data-c="6F" d="M28 214Q28 309 93 378T250 448Q340 448 405 380T471 215Q471 120 407 55T250 -10Q153 -10 91 57T28 214ZM250 30Q372 30 372 193V225V250Q372 272 371 288T364 326T348 362T317 390T268 410Q263 411 252 411Q222 411 195 399Q152 377 139 338T126 246V226Q126 130 145 91Q177 30 250 30Z"></path><path data-c="6C" d="M42 46H56Q95 46 103 60V68Q103 77 103 91T103 124T104 167T104 217T104 272T104 329Q104 366 104 407T104 482T104 542T103 586T103 603Q100 622 89 628T44 637H26V660Q26 683 28 683L38 684Q48 685 67 686T104 688Q121 689 141 690T171 693T182 694H185V379Q185 62 186 60Q190 52 198 49Q219 46 247 46H263V0H255L232 1Q209 2 183 2T145 3T107 3T57 1L34 0H26V46H42Z" transform="translate(500, 0)"></path><path data-c="64" d="M376 495Q376 511 376 535T377 568Q377 613 367 624T316 637H298V660Q298 683 300 683L310 684Q320 685 339 686T376 688Q393 689 413 690T443 693T454 694H457V390Q457 84 458 81Q461 61 472 55T517 46H535V0Q533 0 459 -5T380 -11H373V44L365 37Q307 -11 235 -11Q158 -11 96 50T34 215Q34 315 97 378T244 442Q319 442 376 393V495ZM373 342Q328 405 260 405Q211 405 173 369Q146 341 139 305T131 211Q131 155 138 120T173 59Q203 26 251 26Q322 26 373 103V342Z" transform="translate(778, 0)"></path></g><g data-mml-node="mi" transform="translate(5468, 0)"><path data-c="5F" d="M0 -62V-25H499V-62H0Z"></path></g><g data-mml-node="mtext" transform="translate(5968, 0)"><path data-c="63" d="M370 305T349 305T313 320T297 358Q297 381 312 396Q317 401 317 402T307 404Q281 408 258 408Q209 408 178 376Q131 329 131 219Q131 137 162 90Q203 29 272 29Q313 29 338 55T374 117Q376 125 379 127T395 129H409Q415 123 415 120Q415 116 411 104T395 71T366 33T318 2T249 -11Q163 -11 99 53T34 214Q34 318 99 383T250 448T370 421T404 357Q404 334 387 320Z"></path><path data-c="77" d="M90 368Q84 378 76 380T40 385H18V431H24L43 430Q62 430 84 429T116 428Q206 428 221 431H229V385H215Q177 383 177 368Q177 367 221 239L265 113L339 328L333 345Q323 374 316 379Q308 384 278 385H258V431H264Q270 428 348 428Q439 428 454 431H461V385H452Q404 385 404 369Q404 366 418 324T449 234T481 143L496 100L537 219Q579 341 579 347Q579 363 564 373T530 385H522V431H529Q541 428 624 428Q692 428 698 431H703V385H697Q696 385 691 385T682 384Q635 377 619 334L559 161Q546 124 528 71Q508 12 503 1T487 -11H479Q460 -11 456 -4Q455 -3 407 133L361 267Q359 263 266 -4Q261 -11 243 -11H238Q225 -11 220 -3L90 368Z" transform="translate(444, 0)"></path><path data-c="6E" d="M41 46H55Q94 46 102 60V68Q102 77 102 91T102 122T103 161T103 203Q103 234 103 269T102 328V351Q99 370 88 376T43 385H25V408Q25 431 27 431L37 432Q47 433 65 434T102 436Q119 437 138 438T167 441T178 442H181V402Q181 364 182 364T187 369T199 384T218 402T247 421T285 437Q305 442 336 442Q450 438 463 329Q464 322 464 190V104Q464 66 466 59T477 49Q498 46 526 46H542V0H534L510 1Q487 2 460 2T422 3Q319 3 310 0H302V46H318Q379 46 379 62Q380 64 380 200Q379 335 378 343Q372 371 358 385T334 402T308 404Q263 404 229 370Q202 343 195 315T187 232V168V108Q187 78 188 68T191 55T200 49Q221 46 249 46H265V0H257L234 1Q210 2 183 2T145 3Q42 3 33 0H25V46H41Z" transform="translate(1166, 0)"></path><path data-c="64" d="M376 495Q376 511 376 535T377 568Q377 613 367 624T316 637H298V660Q298 683 300 683L310 684Q320 685 339 686T376 688Q393 689 413 690T443 693T454 694H457V390Q457 84 458 81Q461 61 472 55T517 46H535V0Q533 0 459 -5T380 -11H373V44L365 37Q307 -11 235 -11Q158 -11 96 50T34 215Q34 315 97 378T244 442Q319 442 376 393V495ZM373 342Q328 405 260 405Q211 405 173 369Q146 341 139 305T131 211Q131 155 138 120T173 59Q203 26 251 26Q322 26 373 103V342Z" transform="translate(1722, 0)"></path></g><g data-mml-node="mo" transform="translate(8523.8, 0)"><path data-c="3D" d="M56 347Q56 360 70 367H707Q722 359 722 347Q722 336 708 328L390 327H72Q56 332 56 347ZM56 153Q56 168 72 173H708Q722 163 722 153Q722 140 707 133H70Q56 140 56 153Z"></path><path data-c="3D" d="M56 347Q56 360 70 367H707Q722 359 722 347Q722 336 708 328L390 327H72Q56 332 56 347ZM56 153Q56 168 72 173H708Q722 163 722 153Q722 140 707 133H70Q56 140 56 153Z" transform="translate(778, 0)"></path></g><g data-mml-node="TeXAtom" data-mjx-texclass="ORD" transform="translate(10357.6, 0)"><g data-mml-node="mo"><path data-c="2F" d="M423 750Q432 750 438 744T444 730Q444 725 271 248T92 -240Q85 -250 75 -250Q68 -250 62 -245T56 -231Q56 -221 230 257T407 740Q411 750 423 750Z"></path></g></g><g data-mml-node="mtext" transform="translate(10857.6, 0)"><path data-c="41" d="M255 0Q240 3 140 3Q48 3 39 0H32V46H47Q119 49 139 88Q140 91 192 245T295 553T348 708Q351 716 366 716H376Q396 715 400 709Q402 707 508 390L617 67Q624 54 636 51T687 46H717V0H708Q699 3 581 3Q458 3 437 0H427V46H440Q510 46 510 64Q510 66 486 138L462 209H229L209 150Q189 91 189 85Q189 72 209 59T259 46H264V0H255ZM447 255L345 557L244 256Q244 255 345 255H447Z"></path><path data-c="43" d="M56 342Q56 428 89 500T174 615T283 681T391 705Q394 705 400 705T408 704Q499 704 569 636L582 624L612 663Q639 700 643 704Q644 704 647 704T653 705H657Q660 705 666 699V419L660 413H626Q620 419 619 430Q610 512 571 572T476 651Q457 658 426 658Q322 658 252 588Q173 509 173 342Q173 221 211 151Q232 111 263 84T328 45T384 29T428 24Q517 24 571 93T626 244Q626 251 632 257H660L666 251V236Q661 133 590 56T403 -21Q262 -21 159 83T56 342Z" transform="translate(750, 0)"></path><path data-c="4B" d="M128 622Q121 629 117 631T101 634T58 637H25V683H36Q57 680 180 680Q315 680 324 683H335V637H313Q235 637 233 620Q232 618 232 462L233 307L379 449Q425 494 479 546Q518 584 524 591T531 607V608Q531 630 503 636Q501 636 498 636T493 637H489V683H499Q517 680 630 680Q704 680 716 683H722V637H708Q633 633 589 597Q584 592 495 506T406 419T515 254T631 80Q644 60 662 54T715 46H736V0H728Q719 3 615 3Q493 3 472 0H461V46H469Q515 46 515 72Q515 78 512 84L336 351Q332 348 278 296L232 251V156Q232 62 235 58Q243 47 302 46H335V0H324Q303 3 180 3Q45 3 36 0H25V46H58Q100 47 109 49T128 61V622Z" transform="translate(1472, 0)"></path></g></g></g></svg>

进一步

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkxBKgxWSCSDcmgnujP10rxOME26XHeaY1F8TEibaqoKIBibgGOdTwI2eNjGicv2VpI6p/0?wx_fmt=svg)

即：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkkOVtAAAiaTYEBG89W5nRMNcFBVdAUdxRPmVNic9y28FQdsONwEbDqbwVRaCNKibSFR6/0?wx_fmt=svg)

以此来将目标窗口收敛于 ssthresh。刚进入快速恢复的时候的时候，窗口尚未下降，在收敛结束之前，下面的不等式是成立的：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkibrJibyChbJU9rLysApicJsWKcRrtvwuxkoaDrfPGR91WcnX7ib4GibbWAtBClzHFoeDB/0?wx_fmt=svg)

因此：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXke3LNgNBiauapSx4HP48uuWGkPiaj71usWpXIicib6rr2v97D7Ah4FFicMNiaATtYyMZguF/0?wx_fmt=svg)

考虑到数据包的守恒，设

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkjlf1XtOVfgic0Selq0rYTkBamVoch1icpSPjT9bMvWqLTmUjtRT0pmFo6SuMF4OMrc/0?wx_fmt=svg)

这意味着在收敛结束前，我们可以多发送 extra 这么多的数据包。

**降窗流程** 根据上述原理可以得到 prr 算法降窗流程如下：

1.收到多次（默认为 3）重复 ACK，根据回调函数更新 ssthresh (reno 算法为 cwnd/2)，窗口维持不变;

2.每收到 ACK（不管是否重复），按上述算法计算 Extra; 3. 新窗口值取 cwnd = in_flight + Extra; 4. 直到退出快速恢复，cwnd = ssthresh;

优点：在快速恢复期间，取消了窗口陡降过程，可以更平滑发送数据，且降窗速率与 PIPE 容量相关。缺点：不利于在拥塞恢复到正常时突发流量的发送。

#### 5.5 记分板算法

记分板算法是为了统计网络中正在传输的包数量，即`tcp_packets_in_flight`。只有当 cwnd > tcp_packets_in_flight 时，TCP 才允许发送重传包或者新数据包到网络中。`tcp_packets_in_flight`和`packets_out`, `sacked_out`, `retrans_out`,`lost_out`有关。其中`packets_out`表示发出去的包数量，`sacked_out`为`sack`的包数量，`retrans_out`为重传的包数量，`lost_out`为`loss`的包数量，这里的`loss`包含`rto`,`FR`和`RACK`等机制判断出来的丢失包。

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkACiakhEatumoPgTlpI8sVOrEyoC4AiaK9V6osrAMSvxk8icTh4LHC6vMh2bIWmJHPEA/0?wx_fmt=svg)

为了可以正确统计这些数据，内核给每个 tcp 包(`tcp_skb_cb`)添加了`sacked`字段标记该数据包当前的状态。

```
    __u8        sacked;     /* State flags for SACK.    */
#define TCPCB_SACKED_ACKED  0x01    /* SKB ACK'd by a SACK block    */
#define TCPCB_SACKED_RETRANS    0x02    /* SKB retransmitted        */
#define TCPCB_LOST      0x04    /* SKB is lost          */
#define TCPCB_TAGBITS       0x07    /* All tag bits         */
#define TCPCB_REPAIRED      0x10    /* SKB repaired (no skb_mstamp_ns)  */
#define TCPCB_EVER_RETRANS  0x80    /* Ever retransmitted frame */
#define TCPCB_RETRANS       (TCPCB_SACKED_RETRANS|TCPCB_EVER_RETRANS| \
                TCPCB_REPAIRED)
```

需要在意的有`TCPCB_SACKED_ACKED`（被 SACK 块 ACK'd）,`TCPCB_SACKED_RETRANS`(重传),`TCPCB_LOST`（丢包），其状态转换图如下：

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyKkhDtkDn9X1vSUnNyPDgBichydtvCGZK7EhGhtsdWJHnuln9HnKGHJQ/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)sack 处理记分板

记分板状态转换逻辑如下：

1. 首先判定丢包，打`L` tag，lost_out++，即 `L`
2. 如果需要重传，打`R`tag，retrans_out++，即 `L|R`
3. 如果再次丢包，取消`R`tag，retrans_out–，lost_out 维持不变，go to step2，此时标记位为 `L`
4. 当 SACKED 时，取消`L|R`，retrans_out–，lost_out–，此时为 `S`
5. 当 snd_una 向右更新时，packet_out–

#### 5.6 拥塞窗口校验

在 [RFC2861] 中，区分了 TCP 连接数据传输的三种状态：

- network-limited：TCP 的数据传输受限于拥塞窗口而不能发送更多的数据。
- application-limited：TCP 的数据传输速率受限于应用层的数据写入速率，并没有到达拥塞窗口上限。
- idle：发送端没有额外的数据等待发送，当数据发送间隔超过一个 RTO 的时候就认为是 ilde 态。

cwnd 代表了对网络拥塞状态的一个评估，拥塞控制要根据 ACK 来更新 cwnd 的前提条件是，当前的数据发送速率真实的反映了 cwnd 的状况，也就是说当前传输状态是 network-limited。假如 tcp 隔了很长时间没有发送数据包，即进入 idle，那么当前真实的网络拥塞状态很可能就会与 cwnd 反映的网络状况有差距。另外在 application-limited 的场景下，受限数据的 ACK 报文还可能把 cwnd 增长到一个异常大的值，显然是不合理的。基于上面提到的两个问题，[RFC2861]引入了拥塞窗口校验(CWV，Congestion Window Validation)算法。

算法如下：当需要发送新数据时，首先看距离上次发送操作是否超过一个 RTO，如果超过，则 1. 更新 sshthresh 值，设为 max(ssthresh, (3/4) * cwnd)。2.每经过一个空闲 RTT 时间，cwnd 值减半，但不小于 1 SMSS。

对于应用受限阶段（非空闲阶段），执行相似的操作：1. 已使用的窗口大小记为 。2. 更新 ssthresh 值，设为 max(ssthresh, (3/4) * cwnd)。

1. cwnd 设为 cwnd 和 的平均值。

上述操作均减小了 cwnd，但 ssthresh 维护了 cwnd 的先前值。避免空闲阶段可能发生的大数据量注入，可以减轻对有限的路由缓存的压力，从而减少丢包情况的产生。注意 CWV 减小了 cwnd 值，但没有减小 ssthresh，因此采用这种算法的通常结果是，在长时间发送暂停后，发送方会进入慢启动阶段。Linux TCP 实现了 CWV 算法并默认启用。

### **六、常见的拥塞算法**

#### 6.1 New Reno 算法

New Reno 算法包含第五节中介绍的慢启动算法、拥塞避免算法、快速重传算法和 prr 算法。在此就不赘述。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjy5E85gWoJSSfguaFSxtKHqhTb32o4q8GR6D6q5YgkJvN3ppSFd6ZGSg/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)Reno cwnd 图

#### 6.2 CUBIC 算法

CUBIC 算法和 Reno 算法区别主要在于慢启动和拥塞避免两个阶段。因为 Reno 算法进入拥塞避免后每经过一个 RTT 窗口才加 1，拥塞窗口增长太慢，导致在高速网络下不能充分利用网络带宽。所以为了解决这个问题，BIC 和 CUBIC 算法逐步被提了出来。

(BIC 算法基于二分查找，实现复杂，不看了。)

cubic 算法源码见 `tcp_cubic` 文件。

##### **6.2.1 CUBIC 算法原理**

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjy6AkG6hA4GibJj8pyEHhqppz7h3AiaQuExU1qlEicfdJZEvuV0IzqrMW5Q/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)cubic 窗口增长函数

CUBIC 的窗口增长函数是一个三次函数，非常类似于 BIC-TCP 的窗口增长函数。CUBIC 的详细运行过程如下，当出现丢包事件时，CUBIC 同 BIC-TCP 一样，会记录这时的拥塞窗口大小作为 ，接着通过因子 执行拥塞窗口的乘法减小，这里 是一个窗口降低常数，并进行正常的 TCP 快速恢复和重传。从快速恢复阶段进入拥塞避免后，使用三次函数的凹轮廓增加窗口。三次函数被设置在 处达到稳定点，然后使用三次函数的凸轮廓开始探索新的最大窗口。

CUBIC 的窗口增长函数公式如下所示：



其中，*W(t)*代表在时刻 *t* 的窗口大小，C 是一个 CUBIC 的常量参数，t 是从上次丢包后到现在的时间，以秒为单位。K 是上述函数在没有进一步丢包的情况下将 W 增加到



经历的时间，其计算公式如下：





其中 也是常量（默认为 0.2）。

##### **6.2.2 Wmax 的更新**

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXk1iaGLE4s6rCTnDlekhoMLOgay4Wzg3fGhCV3FBr0WmHfFvbfUGiaWEBqbK1Fj3uzcl/0?wx_fmt=svg)

每次丢包后，CUBIC-TCP 会开启一个新的时段，并取 作为当前 饱和点，记录在 bic_origin_point 字段中，源码如下：

```
static inline void bictcp_update(struct bictcp *ca, u32 cwnd, u32 acked)
{
    // ...
    if (ca->epoch_start == 0) {  //丢包后，开启一个新的时段
        ca->epoch_start = tcp_jiffies32;    /* record beginning */
        ca->ack_cnt = acked;            /* start counting */
        ca->tcp_cwnd = cwnd;            /* syn with cubic */

        //取max(last_max_cwnd , cwnd)作为当前Wmax饱和点
        if (ca->last_max_cwnd <= cwnd) {
            ca->bic_K = 0;
            ca->bic_origin_point = cwnd;
        } else {
            /* Compute new K based on
             * (wmax-cwnd) * (srtt>>3 / HZ) / c * 2^(3*bictcp_HZ)
             */
            ca->bic_K = cubic_root(cube_factor
                           * (ca->last_max_cwnd - cwnd));
            ca->bic_origin_point = ca->last_max_cwnd;
        }
    }
    //...
}
```

##### **6.2.3 hystart 混合启动算法**

标准的慢启动在 BDP 网络环境下表现不好，不好的原因主要有两个：

1. 标准慢启动的拥塞窗口指数式的增长方式过于激进容易导致大量丢包，丢包恢复性能损耗太大。在 ssthreshold 值设置的过高时，慢启动一段时间后，cwnd 的指数增长会显得过快。有可能在上一个 RTT，cwnd 刚好等于 BDP；下一个 RTT，cwnd 就等于 2BDP 了。这样就可能导致多出的一个 BDP 的数据包被丢弃。这类丢包属于突发丢包（burst packet losses）。
2. 被优化过的慢启动机制，丢包时在数据包重传恢复的时候碰巧试图去减小服务器的负载，导致数据包恢复慢。

总结这些原因都是因为慢启动过程过于盲目，不能及时的预测拥塞，导致了大量丢包，所以混合慢启动机制的主要作用是在慢启动阶段试图找到“合理”的退出慢启动进入拥塞避免状态点（safe exit point）。

Hystart 算法通过 ACK train 信息判断一个 Safe Exit Point for Slow Start.同时为了避免计算带宽，通过一个巧妙的转换(具体建议看论文)，只要判断时间差是否超过 Forward one-way delay()来决定是否退出慢启动。

Hystart 算法通过以下两个条件来判断是否应该退出慢启动 1.一个窗口内的数据包的总传输间隔是否超过 2. 数据包的 RTT sample(默认 8 个样本)是否出现较大幅度的增长。如果前面 8 个样本中的最小 rtt 大于全局最小 rtt 与阈值的和，那么表示网络出现了拥塞，应立马进入拥塞避免阶段

#### 6.3 BBR 算法

BBR 全称 bottleneck bandwidth and round-trip propagation time。基于包丢失检测的 Reno、NewReno 或者 cubic 为代表，其主要问题有 Buffer bloat 和长肥管道两种。和这些算法不同，bbr 算法会时间窗口内的最大带宽 max_bw 和最小 RTT min_rtt，并以此计算发送速率和拥塞窗口。

##### **6.3.1 BBR 算法原理**

如下图所示，当没有足够的数据来填满管道时，RTprop 决定了流的行为；当有足够的数据填满时，那就变成了 BtlBw 来决定。这两条约束交汇在点 **inflight =BtlBw\*RTprop，也就是管道的 BDP（带宽与时延的乘积）**。当管道被填满时，那些超过的部分（inflight-BDP）就会在瓶颈链路中制造了一个队列，从而导致了 RTT 的增大。当数据继续增加直到填满了缓存时，多余的报文就会被丢弃了。拥塞就是发生在 BDP 点的右边，而拥塞控制算法就是来控制流的平均工作点离 BDP 点有多远。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyzcvvztPQSdbcibDzIHT7SQutqPeHGeMQm6kPRYGOF8licic8BCyibtNM4g/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)发送速率和RTT vs inflight

RTProp : round-trip propagation time BtlBW : bottleneck bandwidth

基于丢包的拥塞控制算法工作在 bandwidth-limited 区域的右边界区域，尽管这种算法可以达到最大的传输速率，但是它是以高延迟和高丢包率作为代价的。在存储介质较为小的时候，缓存大小只比 BDP 大一点，此时这种算法的时延并不会很高。然而，当存储介质变得便宜之后，交换机的缓存大小已经是 ISP 链路 BDP 的很多很多倍了，这导致了 bufferbloat，从而导致了 RTT 从毫秒级升到了秒级。

当一个连接满足以下两个条件时，它可以在达到最高的吞吐量的同时保持最低时延：

1)速率平衡：瓶颈带宽的数据到达速率与 BtlBw 相等；

2)填满管道：所有的在外数据（inflight data）与 BDP（带宽与时延的乘积）相等

bbr 算法关于拥塞窗口的核心就是计算 BtlBW 和 RTprop，根据这两者值计算 BDP。BtlBw 和 RTprop 可能是动态变化的，所以需要实时地对它们进行估计。

**计算 RTprop**

目前 TCP 为了检测丢包，必须实时地跟踪 RTT 的大小。在任意的时间 t，

<svg xmlns="http://www.w3.org/2000/svg" role="img" focusable="false" viewBox="0 -683 9419.8 899" aria-hidden="true" style="color: rgb(62, 71, 83);vertical-align: -0.489ex;width: 21.312ex;height: 2.034ex;max-width: 300% !important;"><g stroke="currentColor" fill="currentColor" stroke-width="0" transform="matrix(1 0 0 -1 0 0)"><g data-mml-node="math"><g data-mml-node="mtext"><path data-c="52" d="M130 622Q123 629 119 631T103 634T60 637H27V683H202H236H300Q376 683 417 677T500 648Q595 600 609 517Q610 512 610 501Q610 468 594 439T556 392T511 361T472 343L456 338Q459 335 467 332Q497 316 516 298T545 254T559 211T568 155T578 94Q588 46 602 31T640 16H645Q660 16 674 32T692 87Q692 98 696 101T712 105T728 103T732 90Q732 59 716 27T672 -16Q656 -22 630 -22Q481 -16 458 90Q456 101 456 163T449 246Q430 304 373 320L363 322L297 323H231V192L232 61Q238 51 249 49T301 46H334V0H323Q302 3 181 3Q59 3 38 0H27V46H60Q102 47 111 49T130 61V622ZM491 499V509Q491 527 490 539T481 570T462 601T424 623T362 636Q360 636 340 636T304 637H283Q238 637 234 628Q231 624 231 492V360H289Q390 360 434 378T489 456Q491 467 491 499Z"></path><path data-c="54" d="M36 443Q37 448 46 558T55 671V677H666V671Q667 666 676 556T685 443V437H645V443Q645 445 642 478T631 544T610 593Q593 614 555 625Q534 630 478 630H451H443Q417 630 414 618Q413 616 413 339V63Q420 53 439 50T528 46H558V0H545L361 3Q186 1 177 0H164V46H194Q264 46 283 49T309 63V339V550Q309 620 304 625T271 630H244H224Q154 630 119 601Q101 585 93 554T81 486T76 443V437H36V443Z" transform="translate(736, 0)"></path></g><g data-mml-node="msub" transform="translate(1458, 0)"><g data-mml-node="mi"><path data-c="54" d="M40 437Q21 437 21 445Q21 450 37 501T71 602L88 651Q93 669 101 677H569H659Q691 677 697 676T704 667Q704 661 687 553T668 444Q668 437 649 437Q640 437 637 437T631 442L629 445Q629 451 635 490T641 551Q641 586 628 604T573 629Q568 630 515 631Q469 631 457 630T439 622Q438 621 368 343T298 60Q298 48 386 46Q418 46 427 45T436 36Q436 31 433 22Q429 4 424 1L422 0Q419 0 415 0Q410 0 363 1T228 2Q99 2 64 0H49Q43 6 43 9T45 27Q49 40 55 46H83H94Q174 46 189 55Q190 56 191 56Q196 59 201 76T241 233Q258 301 269 344Q339 619 339 625Q339 630 310 630H279Q212 630 191 624Q146 614 121 583T67 467Q60 445 57 441T43 437H40Z"></path></g><g data-mml-node="TeXAtom" transform="translate(584, -150) scale(0.707)" data-mjx-texclass="ORD"><g data-mml-node="mi"><path data-c="74" d="M26 385Q19 392 19 395Q19 399 22 411T27 425Q29 430 36 430T87 431H140L159 511Q162 522 166 540T173 566T179 586T187 603T197 615T211 624T229 626Q247 625 254 615T261 596Q261 589 252 549T232 470L222 433Q222 431 272 431H323Q330 424 330 420Q330 398 317 385H210L174 240Q135 80 135 68Q135 26 162 26Q197 26 230 60T283 144Q285 150 288 151T303 153H307Q322 153 322 145Q322 142 319 133Q314 117 301 95T267 48T216 6T155 -11Q125 -11 98 4T59 56Q57 64 57 83V101L92 241Q127 382 128 383Q128 385 77 385H26Z"></path></g></g></g><g data-mml-node="mo" transform="translate(2625, 0)"><path data-c="3D" d="M56 347Q56 360 70 367H707Q722 359 722 347Q722 336 708 328L390 327H72Q56 332 56 347ZM56 153Q56 168 72 173H708Q722 163 722 153Q722 140 707 133H70Q56 140 56 153Z"></path></g><g data-mml-node="mtext" transform="translate(3680.8, 0)"><path data-c="52" d="M130 622Q123 629 119 631T103 634T60 637H27V683H202H236H300Q376 683 417 677T500 648Q595 600 609 517Q610 512 610 501Q610 468 594 439T556 392T511 361T472 343L456 338Q459 335 467 332Q497 316 516 298T545 254T559 211T568 155T578 94Q588 46 602 31T640 16H645Q660 16 674 32T692 87Q692 98 696 101T712 105T728 103T732 90Q732 59 716 27T672 -16Q656 -22 630 -22Q481 -16 458 90Q456 101 456 163T449 246Q430 304 373 320L363 322L297 323H231V192L232 61Q238 51 249 49T301 46H334V0H323Q302 3 181 3Q59 3 38 0H27V46H60Q102 47 111 49T130 61V622ZM491 499V509Q491 527 490 539T481 570T462 601T424 623T362 636Q360 636 340 636T304 637H283Q238 637 234 628Q231 624 231 492V360H289Q390 360 434 378T489 456Q491 467 491 499Z"></path><path data-c="54" d="M36 443Q37 448 46 558T55 671V677H666V671Q667 666 676 556T685 443V437H645V443Q645 445 642 478T631 544T610 593Q593 614 555 625Q534 630 478 630H451H443Q417 630 414 618Q413 616 413 339V63Q420 53 439 50T528 46H558V0H545L361 3Q186 1 177 0H164V46H194Q264 46 283 49T309 63V339V550Q309 620 304 625T271 630H244H224Q154 630 119 601Q101 585 93 554T81 486T76 443V437H36V443Z" transform="translate(736, 0)"></path><path data-c="70" d="M36 -148H50Q89 -148 97 -134V-126Q97 -119 97 -107T97 -77T98 -38T98 6T98 55T98 106Q98 140 98 177T98 243T98 296T97 335T97 351Q94 370 83 376T38 385H20V408Q20 431 22 431L32 432Q42 433 61 434T98 436Q115 437 135 438T165 441T176 442H179V416L180 390L188 397Q247 441 326 441Q407 441 464 377T522 216Q522 115 457 52T310 -11Q242 -11 190 33L182 40V-45V-101Q182 -128 184 -134T195 -145Q216 -148 244 -148H260V-194H252L228 -193Q205 -192 178 -192T140 -191Q37 -191 28 -194H20V-148H36ZM424 218Q424 292 390 347T305 402Q234 402 182 337V98Q222 26 294 26Q345 26 384 80T424 218Z" transform="translate(1458, 0)"></path><path data-c="72" d="M36 46H50Q89 46 97 60V68Q97 77 97 91T98 122T98 161T98 203Q98 234 98 269T98 328L97 351Q94 370 83 376T38 385H20V408Q20 431 22 431L32 432Q42 433 60 434T96 436Q112 437 131 438T160 441T171 442H174V373Q213 441 271 441H277Q322 441 343 419T364 373Q364 352 351 337T313 322Q288 322 276 338T263 372Q263 381 265 388T270 400T273 405Q271 407 250 401Q234 393 226 386Q179 341 179 207V154Q179 141 179 127T179 101T180 81T180 66V61Q181 59 183 57T188 54T193 51T200 49T207 48T216 47T225 47T235 46T245 46H276V0H267Q249 3 140 3Q37 3 28 0H20V46H36Z" transform="translate(2014, 0)"></path><path data-c="6F" d="M28 214Q28 309 93 378T250 448Q340 448 405 380T471 215Q471 120 407 55T250 -10Q153 -10 91 57T28 214ZM250 30Q372 30 372 193V225V250Q372 272 371 288T364 326T348 362T317 390T268 410Q263 411 252 411Q222 411 195 399Q152 377 139 338T126 246V226Q126 130 145 91Q177 30 250 30Z" transform="translate(2406, 0)"></path></g><g data-mml-node="msub" transform="translate(6586.8, 0)"><g data-mml-node="mi"><path data-c="70" d="M23 287Q24 290 25 295T30 317T40 348T55 381T75 411T101 433T134 442Q209 442 230 378L240 387Q302 442 358 442Q423 442 460 395T497 281Q497 173 421 82T249 -10Q227 -10 210 -4Q199 1 187 11T168 28L161 36Q160 35 139 -51T118 -138Q118 -144 126 -145T163 -148H188Q194 -155 194 -157T191 -175Q188 -187 185 -190T172 -194Q170 -194 161 -194T127 -193T65 -192Q-5 -192 -24 -194H-32Q-39 -187 -39 -183Q-37 -156 -26 -148H-6Q28 -147 33 -136Q36 -130 94 103T155 350Q156 355 156 364Q156 405 131 405Q109 405 94 377T71 316T59 280Q57 278 43 278H29Q23 284 23 287ZM178 102Q200 26 252 26Q282 26 310 49T356 107Q374 141 392 215T411 325V331Q411 405 350 405Q339 405 328 402T306 393T286 380T269 365T254 350T243 336T235 326L232 322Q232 321 229 308T218 264T204 212Q178 106 178 102Z"></path></g><g data-mml-node="TeXAtom" transform="translate(503, -150) scale(0.707)" data-mjx-texclass="ORD"><g data-mml-node="mi"><path data-c="74" d="M26 385Q19 392 19 395Q19 399 22 411T27 425Q29 430 36 430T87 431H140L159 511Q162 522 166 540T173 566T179 586T187 603T197 615T211 624T229 626Q247 625 254 615T261 596Q261 589 252 549T232 470L222 433Q222 431 272 431H323Q330 424 330 420Q330 398 317 385H210L174 240Q135 80 135 68Q135 26 162 26Q197 26 230 60T283 144Q285 150 288 151T303 153H307Q322 153 322 145Q322 142 319 133Q314 117 301 95T267 48T216 6T155 -11Q125 -11 98 4T59 56Q57 64 57 83V101L92 241Q127 382 128 383Q128 385 77 385H26Z"></path></g></g></g><g data-mml-node="mo" transform="translate(7617.3, 0)"><path data-c="2B" d="M56 237T56 250T70 270H369V420L370 570Q380 583 389 583Q402 583 409 568V270H707Q722 262 722 250T707 230H409V-68Q401 -82 391 -82H389H387Q375 -82 369 -68V230H70Q56 237 56 250Z"></path></g><g data-mml-node="msub" transform="translate(8617.5, 0)"><g data-mml-node="mi"><path data-c="3B7" d="M21 287Q22 290 23 295T28 317T38 348T53 381T73 411T99 433T132 442Q156 442 175 435T205 417T221 395T229 376L231 369Q231 367 232 367L243 378Q304 442 382 442Q436 442 469 415T503 336V326Q503 302 439 53Q381 -182 377 -189Q364 -216 332 -216Q319 -216 310 -208T299 -186Q299 -177 358 57L420 307Q423 322 423 345Q423 404 379 404H374Q288 404 229 303L222 291L189 157Q156 26 151 16Q138 -11 108 -11Q95 -11 87 -5T76 7T74 17Q74 30 114 189T154 366Q154 405 128 405Q107 405 92 377T68 316T57 280Q55 278 41 278H27Q21 284 21 287Z"></path></g><g data-mml-node="TeXAtom" transform="translate(497, -150) scale(0.707)" data-mjx-texclass="ORD"><g data-mml-node="mi"><path data-c="74" d="M26 385Q19 392 19 395Q19 399 22 411T27 425Q29 430 36 430T87 431H140L159 511Q162 522 166 540T173 566T179 586T187 603T197 615T211 624T229 626Q247 625 254 615T261 596Q261 589 252 549T232 470L222 433Q222 431 272 431H323Q330 424 330 420Q330 398 317 385H210L174 240Q135 80 135 68Q135 26 162 26Q197 26 230 60T283 144Q285 150 288 151T303 153H307Q322 153 322 145Q322 142 319 133Q314 117 301 95T267 48T216 6T155 -11Q125 -11 98 4T59 56Q57 64 57 83V101L92 241Q127 382 128 383Q128 385 77 385H26Z"></path></g></g></g></g></g></svg>

其中

<svg xmlns="http://www.w3.org/2000/svg" role="img" focusable="false" viewBox="0 -442 802.3 658" aria-hidden="true" style="color: rgb(62, 71, 83);vertical-align: -0.489ex;width: 1.815ex;height: 1.489ex;max-width: 300% !important;"><g stroke="currentColor" fill="currentColor" stroke-width="0" transform="matrix(1 0 0 -1 0 0)"><g data-mml-node="math"><g data-mml-node="msub"><g data-mml-node="mi"><path data-c="3B7" d="M21 287Q22 290 23 295T28 317T38 348T53 381T73 411T99 433T132 442Q156 442 175 435T205 417T221 395T229 376L231 369Q231 367 232 367L243 378Q304 442 382 442Q436 442 469 415T503 336V326Q503 302 439 53Q381 -182 377 -189Q364 -216 332 -216Q319 -216 310 -208T299 -186Q299 -177 358 57L420 307Q423 322 423 345Q423 404 379 404H374Q288 404 229 303L222 291L189 157Q156 26 151 16Q138 -11 108 -11Q95 -11 87 -5T76 7T74 17Q74 30 114 189T154 366Q154 405 128 405Q107 405 92 377T68 316T57 280Q55 278 41 278H27Q21 284 21 287Z"></path></g><g data-mml-node="TeXAtom" transform="translate(497, -150) scale(0.707)" data-mjx-texclass="ORD"><g data-mml-node="mi"><path data-c="74" d="M26 385Q19 392 19 395Q19 399 22 411T27 425Q29 430 36 430T87 431H140L159 511Q162 522 166 540T173 566T179 586T187 603T197 615T211 624T229 626Q247 625 254 615T261 596Q261 589 252 549T232 470L222 433Q222 431 272 431H323Q330 424 330 420Q330 398 317 385H210L174 240Q135 80 135 68Q135 26 162 26Q197 26 230 60T283 144Q285 150 288 151T303 153H307Q322 153 322 145Q322 142 319 133Q314 117 301 95T267 48T216 6T155 -11Q125 -11 98 4T59 56Q57 64 57 83V101L92 241Q127 382 128 383Q128 385 77 385H26Z"></path></g></g></g></g></g></svg>

表示“噪音”。造成噪声的因素主要有：链路队列，接收方的时延 ACK 配置，ACK 聚合等因素等待。RTprop 是路径的物理特性，并且只有路径变化才会改变。由于一般来说路径变化的时间尺度远远大于 RTprop，所以 RTprop 可以由以下公式进行估计：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkibFCyUiadk6n3VnUzzmLESSXUBYibMTswJ8OBicic2WuSXfqRBYibodEKapuRKzAtofVfh/0?wx_fmt=svg)



即，在一个时间窗口中对 RTT 取最小值。一般将该窗口大小设置为几十秒至几分钟。

**计算 BtlBW**

bottleneck bandwidth 的估计不像 RTT 那样方便，没有一种 TCPspec 要求实现算法来跟踪估计 bottleneck 带宽，但是，可以通过跟踪发送速率来估计 bottleneck 带宽。当发送方收到一个 ACK 报文时，它可以计算出该报文的 RTT，并且从发出报文到收到 ack 报文这段时间的 data Inflight。这段时间内的平均发送速率就可以以此计算出来：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkyRp6S164sSWW7vDCZxic80X2RQoVEXYn2bZnY2q3jvAzRCWiaExibHwTvXHwmKRTlqA/0?wx_fmt=svg)

这个计算出的速率必定小于 bottleneck 速率（因为 delta delivered 是确定的，但是 deltat 会较大）。因此，BtlBw 可以根据以下公式进行估计。

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXke6GSiaf24YuN3ssHKDQTsY8HKes0pkdMuYv9XMlmQ5wugR2ibH3H8XISCKkg0n0RCe/0?wx_fmt=svg)

其中，时间窗口大小的值一般为 6~10 个 RTT。

TCP 必须记录每个报文的离开时间从而计算 RTT。BBR 必须额外记录已经发送的数据大小，使得在收到每一个 ACK 之后，计算 RTT 及发送速率的值，最后得到 RTprop 和 BtlBw 的估计值。

##### **6.3.2 pacing_rate 和 cwnd**

bbr 算法输出 pacing_rate 和 cwnd 两个数据。pacing_rate 决定发包速率，cwnd 为窗口大小。每一次 ACK 都会根据当前的模式计算 pacing_rate 和 cwnd。注意在计算 pacing_rate 和 cwnd 时有 pacing_gain 和 cwnd_gain 两个参数，

```
bottleneck_bandwidth = windowed_max(delivered / elapsed, 10 round trips)
min_rtt = windowed_min(rtt, 10 seconds)

pacing_rate = pacing_gain * bottleneck_bandwidth
cwnd = max(cwnd_gain * bottleneck_bandwidth * min_rtt, 4)
```

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyQVc1ZzA7Frzl1uZkaNlhFFQ0JmgLaPwEfWGMv7hPFZpiacNib40SvjdQ/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)BBR 和 Pacing rate

##### **6.3.3 BBR 状态机**

BBR 算法也是基于状态机。状态机有`STARTUP`、`DRAIN`、`PROBE_BW`、`PROBE_RTT`四种状态。不同状态下**pacing_gain** 和 **cwnd_gain** 的取值会有所不同。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyKd74Z2iauhz3lZa4NxsOGEnficfQNlwMQbZEDUcYF5agPMxSfpASliaPw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)

bbr 状态机

`STARTUP`：初始状态，该状态下类似于传统拥塞控制的慢启动阶段。该状态下`pacing_gain`和`cwnd_gain`为**2/ln(2)+1**。因为这是最小的能够达到 Reno 或者 CUBIC 算法启动速度的值。

```
/* We use a high_gain value of 2/ln(2) because it's the smallest pacing gain
 * that will allow a smoothly increasing pacing rate that will double each RTT
 * and send the same number of packets per RTT that an un-paced, slow-starting
 * Reno or CUBIC flow would:
 */
static const int bbr_high_gain  = BBR_UNIT * 2885 / 1000 + 1;
```

`DRAIN`：该状态为排除状态。在**STARTUP**状态下三轮没有涨幅超过 25%时会进入该状态。该状态下**BtlBw**会根据**bbr_drain_gain**逐渐降低，直到 inflight 降到 BDP 为止。

```
/* The pacing gain of 1/high_gain in BBR_DRAIN is calculated to typically drain
 * the queue created in BBR_STARTUP in a single round:
 */
static const int bbr_drain_gain = BBR_UNIT * 1000 / 2885;
```

`PROBE_BW`：该状态下会照常计算当前的 bw，即瞬时带宽。然而在计算 pacing rate 以及 cwnd 时，并不会像在 STARTUP 状态时那样用一个很大的增益系数去扩张 pacing rate 和 cwnd，而是顺序的在[5/4，3/4，1，1，1，1，1，1]中选一个，感官上 bw 就在其停止增长的地方上下徘徊了。

```
/* The gain for deriving steady-state cwnd tolerates delayed/stretched ACKs: */
static const int bbr_cwnd_gain  = BBR_UNIT * 2;
/* The pacing_gain values for the PROBE_BW gain cycle, to discover/share bw: */
static const int bbr_pacing_gain[] = {
    BBR_UNIT * 5 / 4,   /* probe for more available bw */
    BBR_UNIT * 3 / 4,   /* drain queue and/or yield bw to other flows */
    BBR_UNIT, BBR_UNIT, BBR_UNIT,   /* cruise at 1.0*bw to utilize pipe, */
    BBR_UNIT, BBR_UNIT, BBR_UNIT    /* without creating excess queue... */
};
```

`PROBE_RTT`：当 PROBE_BW 检测到连续 10s 没有更新 min rtt 时就会进入该状态。该状态的目标是保持 BBR 的公平性并定期排空瓶颈队列，以收敛到真实的 min_rtt。进入该模式时，BBR 会将 cwnd 的上限设置为 4 个数据包。在 flight pkg <= 4 后开始进行 rtt 探测，探测时间为 200ms，探测结束后 BBR 便会记录 min rtt，并离开 PROBE_RTT 进入相应的模式。代码见`bbr_update_min_rtt`函数。

**Q：为什么 PROBE_BW 阶段 bbr_cwnd_gain 为 2？** 保证极端情况下，按照 pacing_rate 发送的数据包全部丢包时也有数据继续发送，不会产生空窗期。

**Q：为什么在探测最小 RTT 的时候最少要保持 4 个数据包**4 个包的窗口是合理的，infilght 分别是：刚发出的包，已经到达接收端等待延迟应答的包，马上到达的应答了 2 个包的 ACK。一共 4 个，只有 1 个在链路上，另外 1 个在对端主机里，另外 2 个在 ACK 里。路上只有 1 个包。

##### **6.3.4 BBR 算法表现**

BBR 将它的大部分时间的在外发送数据都保持为一个 BDP 大小，并且发送速率保持在估计得 BtlBw 值，这将会最小化时延。但是这会把网络中的瓶颈链路移动到 BBR 发送方本身，所以 BBR 无法察觉 BtlBw 是否上升了。所以，BBR 周期性的在一个 RTprop 时间内将 pacing_gain 设为一个大于 1 的值，这将会增加发送速率和在外报文。如果 BtlBw 没有改变，那么这意味着 BBR 在网络中制造了队列，增大了 RTT，而 deliveryRate 仍然没有改变。（这个队列将会在下个 RTprop 周期被 BBR 使用小于 1 的 pacing_gain 来消除）。如果 BtlBw 增大了，那么 deliveryRate 增大了，并且 BBR 会立即更新 BtlBw 的估计值，从而增大了发送速率。通过这种机制，BBR 可以以指数速度非常快地收敛到瓶颈链路。

如下图所示，在 1 条 10Mbps，40ms 的流在 20s 稳定运行之后将 BtlBw 提高了 1 倍（20Mbps），然后在第 40s 又将 BtlBw 恢复至 20Mbps。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyvVmRW9uupM9V2icVx2wasoT3llddXRzzSQlFzasUwNYthiaqAYpJvDMQ/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)bbr 带宽变化

下图展示了 1 个 10Mbps，40ms 的 BBR 流在一开始的 1 秒内，发送方（绿线）和接收方（蓝线）的过程。红线表示的是同样条件下的 CUBIC 发送。垂直的灰线表示了 BBR 状态的转换。下方图片展示了两种连接的 RTT 在这段时间的变化。注意，只有收到了 ACK（蓝线）之后才能确定出 RTT，所以在时间上有点偏移。图中标注了 BBR 何时学习到 RTT 和如何反应。

![图片](data:image/svg+xml,%3C%3Fxml version='1.0' encoding='UTF-8'%3F%3E%3Csvg width='1px' height='1px' viewBox='0 0 1 1' version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'%3E%3Ctitle%3E%3C/title%3E%3Cg stroke='none' stroke-width='1' fill='none' fill-rule='evenodd' fill-opacity='0'%3E%3Cg transform='translate(-249.000000, -126.000000)' fill='%23FFFFFF'%3E%3Crect x='249' y='126' width='1' height='1'%3E%3C/rect%3E%3C/g%3E%3C/g%3E%3C/svg%3E)10Mbps、40ms链路上BBR流的第一秒

下图展示了在上图中展示的 BBR 和 CUBIC 流在开始 8 秒的行为。CUBIC（红线）填满了缓存之后，周期性地在 70%~100%的带宽范围内波动。然而 BBR（绿线）在启动过程结束后，就非常稳定地运行，并且不会产生任何队列。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjy0yabCkbZfLrTTiaicWu1wuGpnnTs0rTewxtzBq73Wzx2aLIAwjMQdVCQ/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)在10Mbps、40ms链路上的BBR流和CUBIC流的前8秒对比

下图展示了在一条 100Mbps，100ms 的链路上，BBR 和 CUBIC 在 60 秒内的吞吐量与随机丢包率（从 0.001%~50%）的关系。在丢包率只有 0.1%的时候，CUBIC 的吞吐量就已经下降了 10 倍，并且在丢包率为 1%的时候就几乎炸了。而理论上的最大吞吐量是链路速率乘以（1-丢包率）。BBR 在丢包率为 5%以下时还能基本维持在最大吞吐量附近，在 15%丢包率的时候虽然有所下降但还是不错。

![图片](https://mmbiz.qpic.cn/mmbiz_png/j3gficicyOvav1vibaD50nQndyJEApcqhjyvClbL4EziaazibelBUngWZriaoeC6O6mSFSnePjanImWNRW2kTAuRBplw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1)BBC和CUBIC的吞吐量与丢包率的关系

#### 6.4 Westwood 算法

TCP Westwood 算法简称 TCPW，和 bbr 算法类似是基于带宽估计的一种拥塞控制算法。TCPW 采用和 Reno 相同的慢启动算法、拥塞避免算法。区别在于当检测到丢包时，根据带宽值来设置拥塞窗口、慢启动阈值。

##### **6.4.1 如何测量带宽 bw_est**

和 bbr 算法不同，tcpw 带宽计算相当粗糙。tcpw 每经过一个 RTT 测量一次带宽。假设经过时间为 delta，该时间内发送完成的数据量为 bk，则采样值为 bk / delta。然后和 rtt 一样，带宽采样值会经过一个平滑处理算出最终的带宽值。

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXk0wjlnCk56peGVRw6QQ9zWXwD46970p2wib6cuwzQvicbECY7RsfnzshHictqJiasFWibX/0?wx_fmt=svg)

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkzs6gr0Y4yDO2qqmB7avFozJuHjmuLJQyW1o8ibelSgybYLt8G5ibx7YluQH7ekWXLB/0?wx_fmt=svg)

##### **6.4.2 如何确认单位时间的发送量 bk**

tcpw 采用一种粗糙的估算方式。在收到回包后，会根据当前的 snd_una 和之前的 snd_una 之间的差值来估算被 ACK 的字节数，即关于 SACK 的信息会被丢失。具体逻辑见`westwood_acked_count`函数。

```
static inline u32 westwood_acked_count(struct sock *sk)
{
    const struct tcp_sock *tp = tcp_sk(sk);
    struct westwood *w = inet_csk_ca(sk);

    w->cumul_ack = tp->snd_una - w->snd_una;

    /* If cumul_ack is 0 this is a dupack since it's not moving
     * tp->snd_una.
     */
    if (!w->cumul_ack) {
        w->accounted += tp->mss_cache;
        w->cumul_ack = tp->mss_cache;
    }

    if (w->cumul_ack > tp->mss_cache) {
        /* Partial or delayed ack */
        if (w->accounted >= w->cumul_ack) {
            w->accounted -= w->cumul_ack;
            w->cumul_ack = tp->mss_cache;
        } else {
            w->cumul_ack -= w->accounted;
            w->accounted = 0;
        }
    }

    w->snd_una = tp->snd_una;

    return w->cumul_ack;
}
```

##### **6.4.3 计算 ssthresh**

计算 ssthresh 公式很简单：

![img](https://mmbiz.qlogo.cn/mmbiz_svg/Iic9WLWEQMg1XYRcGSzPSxaM3tSmhDCXkNp90aliaImIPia5M5W8N5PTryHhQF6aASX2Z1Klib5v81EfnPJ3jDGYmhiaONibukDcKJ/0?wx_fmt=svg)

源码如下：

```
/*
 * TCP Westwood
 * Here limit is evaluated as Bw estimation*RTTmin (for obtaining it
 * in packets we use mss_cache). Rttmin is guaranteed to be >= 2
 * so avoids ever returning 0.
 */
static u32 tcp_westwood_bw_rttmin(const struct sock *sk)
{
    const struct tcp_sock *tp = tcp_sk(sk);
    const struct westwood *w = inet_csk_ca(sk);

    return max_t(u32, (w->bw_est * w->rtt_min) / tp->mss_cache, 2);
}
```

### **七、参考文档**

- [TCP 的那些事儿（上）](https://coolshell.cn/articles/11564.html) [TCP 的那些事儿（下）](https://coolshell.cn/articles/11609.html)
- [TCP 系列 08—连接管理—7、TCP 常见选项(option)](https://www.cnblogs.com/lshs/p/6038494.html)
- [TCP timestamp](https://perthcharles.github.io/2015/08/27/timestamp-intro/)
- [Early Retransmit forTCP](http://perthcharles.github.io/2015/10/31/wiki-network-tcp-early-retrans/)
- [TCP Tail Loss Probe(TLP)](http://perthcharles.github.io/2015/10/31/wiki-network-tcp-tlp/)
- [TCP 重点系列之拥塞状态机](https://allen-kevin.github.io/2017/04/19/TCP重点系列之拥塞状态机/)
- [Congestion Control in Linux TCP](https://pdfs.semanticscholar.org/0e9c/968d09ab2e53e24c4dca5b2d67c7f7140f8e.pdf)
- [TCP 拥塞控制图解](https://blog.csdn.net/dog250/article/details/51287078)
- [TCP 进入快速恢复时的窗口下降算法](https://blog.csdn.net/dog250/article/details/51404615)
- [tcp 中 RACK 算法](https://blog.csdn.net/noma_hwang/article/details/53906565)
- [TCP 系列 23—重传—13、RACK 重传](https://www.cnblogs.com/lshs/p/6038592.html)
- [TCP 系列 18—重传—8、FACK 及 SACK reneging 下的重传](https://www.cnblogs.com/lshs/p/6038562.html)