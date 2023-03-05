REST 是 Representational State Transfer 的缩写，如果一个架构符合 REST 原则，就称它为 RESTful 架构

RESTful 架构可以充分的利用 HTTP 协议的各种功能，是 HTTP 协议的最佳实践

RESTful API 是一种软件架构风格、设计风格，可以让软件更加清晰，更简洁，更有层次，可维护性更好

## RESTful API 请求设计
```
请求 = 动词 + 宾语
```
+ 动词使用五种 HTTP 方法，对应 CRUD 操作。
+ 宾语 URL 应该全部使用名词复数，可以有例外，比如搜索可以使用更加直观的 search 。
+ 过滤信息（Filtering） 如果记录数量很多，API 应该提供参数，过滤返回结果。 ?limit=10 指定返回记录的数量 ?offset=10 指定返回记录的开始位置。
![](../images/Pasted%20image%2020230305212231.png)
## RESTful API 响应设计
客户端的每一次请求，服务器都必须给出回应。回应包括 HTTP 状态码和数据两部分。

+ 客户端请求时，要明确告诉服务器，接受 JSON 格式，请求的 HTTP 头的 ACCEPT 属性要设成 application/json
+ 服务端返回的数据，不应该是纯文本，而应该是一个 JSON 对象。服务器回应的 HTTP 头的 Content-Type 属性要设为 application/json
+ 错误处理如果状态码是4xx，就应该向用户返回出错信息。一般来说，返回的信息中将 error 作为键名，出错信息作为键值即可。 {error: "Invalid API key"}
+ 认证 RESTful API 应该是无状态，每个请求应该带有一些认证凭证。推荐使用 JWT 认证，并且使用 SSL
+ Hypermedia 即返回结果中提供链接，连向其他 API 方法，使得用户不查文档，也知道下一步应该做什么

>  [RESTful API 一种流行的 API 设计风格](https://restfulapi.cn/)