# files_server_system
前言：<br>
=
项目项目使用c++实现了一个分布式文件系统服务器和相关客户端，提供完善的文件上传、下载、分享及转存等服务功能。项目基于HTTP协议实现了客户端与服务端的通信服务，通信数据格式采用multipart/from-data及Json格式，采用MySQL数据库和Redis数据库作为后台数据存储。支持大文件上传、断点续传等功能。<br>

项目架构
--
![1527001368556](https://github.com/jxzeng/files_server_system/assets/46019245/96edde5a-92d5-4fe0-a246-e13672f90018)  

1、服务端  
-
* Nginx：
  * 1、作为web服务器(-解析http协议)，处理静态数据请求；
  * 2、反向代理服务器，实现负载均衡；
* FastCGI：
  * 处理服务器动态请求，仓库的FastCGI文件夹中提供了5个C++编写的FastCGI程序，包括有登录验证、注册、文件处理等。
* MySQL/Redis：
  * 存储文件及用户的属性信息;  
  * Redis是为了提高程序效率，减轻mysql压力，存储服务器经常要从关系型数据中读取的数据;
* FastDFS：分布式文件系统

2、客户端
-
>> 采用c/s架构：选择HTTP协议作为客户端与服务端的通信协议，方便后续代码复用。

简单介绍：
FastDFS
-
>> 一款开源的分布式文件系统,考虑了冗余备份、负载均衡、线性扩容等机制，注重高可用、高性能等功能。
>> FastDFS具有三个角色：追踪器 ( tracker )、存储节点（storage ）、客户端（client）
fdfs三个角色之间的关系：
上传  
![fdfs-file-upload](https://github.com/jxzeng/files_server_system/assets/46019245/7c231525-41b1-4c15-a9ea-ad9a96c5f444)

下载  
![fdfs-file-down](https://github.com/jxzeng/files_server_system/assets/46019245/11f9aa42-695b-48d7-b5fe-3d0b06f4cf26)

Nginx：
-
![4](https://github.com/jxzeng/files_server_system/assets/46019245/e931a41f-60e3-42d2-a8e0-79294f982e84)

FastCGI：
-  
>> 客户端会将数据提交给服务器，而Nginx服务器无法处理动态数据请求，故针对动态数据，通常采用CGI（通用网关接口）技术来处理。CGI（Common Gateway Interface/CGI）描述了客户端和服务器程序之间传输数据的一种标准，可以让一个客户端，从网页浏览器向执行在网络服务器上的程序请求数据。而fastCGI，快速通用网关接口（Fast Common Gateway Interface／FastCGI）是通用网关接口（CGI）的改进，描述了客户端和服务器程序之间传输数据的一种标准。FastCGI致力于减少Web服务器与CGI程式之间互动的开销，从而使服务器可以同时处理更多的Web请求。与为每个请求创建一个新的进程不同，FastCGI使用持续的进程来处理一连串的请求。这些进程由FastCGI进程管理器管理，而不是web服务器。fastCGI与CGI的区别: CGI 就是所谓的短生存期应用程序，FastCGI 就是所谓的长生存期应用程序。FastCGI像是一个常驻(long-live)型的CGI，它可以一直执行着，不会每次都要花费时间去fork一次。通俗来讲，CGI是来一个请求就fork一个进程，而fastCGI只会fork一个进程，多个请求都使用同一个进程。  
![1535119675577](https://github.com/jxzeng/files_server_system/assets/46019245/fe7b67f5-408b-4dfd-8f1c-bfe955e61374)
![191104434855332](https://github.com/jxzeng/files_server_system/assets/46019245/cf4e5167-96ce-47ad-8466-8a123d7b5da1)

断点续传：
-  
为什么要使用分片上传        
* 文件过大：一个文件过大，几个G或者几十G的大文件就非常适合分片上传  
* 网络不稳定：网络时好时坏或者比较慢的情况  
* 断点续传：传了一部分后暂停想继续传的时候，分片上传也是个不错的选择  
项目是通过在服务端创建一个上传临时文件，上传前，服务端返回临时文件的大小作为客户端上传的开始偏移量，客户端从偏移量开始分块进行上传至服务端。

用户登录、注册流程图：
-
![屏幕截图 2023-08-27 184218](https://github.com/jxzeng/files_server_system/assets/46019245/6e06efdc-b253-46da-a193-e7138772ebaf)

文件上传流程图：
-

![未命名文件](https://github.com/jxzeng/files_server_system/assets/46019245/46137573-d8af-44ed-b721-08914d8168ae)

文件下载流程图：
--
![未命名文件 (1)](https://github.com/jxzeng/files_server_system/assets/46019245/e4c21f5e-666e-4712-bedd-e27dfe694e4a)


客户端：
-  
![屏幕截图 2023-08-27 190303](https://github.com/jxzeng/files_server_system/assets/46019245/46fcc3dd-1519-45ca-a46c-d990f2b0e6b3)
![屏幕截图 2023-08-27 190705](https://github.com/jxzeng/files_server_system/assets/46019245/55ad391d-2197-44a4-b556-3ded11185ddf)
![屏幕截图 2023-08-27 190809](https://github.com/jxzeng/files_server_system/assets/46019245/68c0972a-d390-4088-a93b-dd263cfd5d76)
![屏幕截图 2023-08-27 190832](https://github.com/jxzeng/files_server_system/assets/46019245/00d88a7b-ce69-4486-bf9d-2a61a5962036)
![屏幕截图 2023-08-27 190957](https://github.com/jxzeng/files_server_system/assets/46019245/ffc4f0ba-7c40-4280-ae5b-d8a85c172534)






