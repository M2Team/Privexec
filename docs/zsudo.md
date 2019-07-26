# ZSUDO 设计规范（草案）

|组件|功能|运行级别|
|---|---|---|
|zsudo|提权客户端|标准权限|
|zsudo-srv|提权服务端|System 权限|


## ZSUDO 命令行参数设计

zsudo 命令行参数设计与 wsudo 类似，以 `-` `--` 开头的是命令行参数，否则为子命令的命令行参数，一旦出现子命令，接下来的参数全部需要传递给子命令。

|zsudo 命令行|子进程命令行|
|---|---|
|`zsudo -V curl --verbose -I https://nghttp2.org/`|`curl --verbose -I https://nghttp2.org/`|
|`zsudo curl -V`|`curl -V`|

zsudo 用法：

|参数|类型|默认值|备注|
|---|---|---|---|
|`-v,--version`|Boolean|N|输出 zsudo 版本信息并退出|
|`-h,--help`|Boolean|N|输出 zsudo 帮助信息并退出|
|`-w,--wait`|Boolean|启动的程序子系统为 GUI 时不等待，子系统为 CUI 时等待|zsudo 等待子进程退出|
|`-n,--new-console`|Boolean|N (仅对控制台有效)|为控制台应用启动新的控制台。|
|`-H,--hide`|Boolean|N|隐藏控制台应用的控制台|
|`-c, --cwd`|String|默认为 zsudo 当前目录|设置子进程目录|
|`-e,--env`|String|默认不设置|增加子进程环境变量|
|`-p, --prompt=prompt`|Charecter|`*`|密码掩码|

## ZSUDO 提权机制设计

ZSUDO 分为客户端和服务端，`zsudo` 命令为客户端 `cli`，`zsudo-srv` 为服务端。在用户安装 `zsudo-srv` 为 Windows 服务后，用户可以通过 `zsudo` 命令与 `zsudo-srv` 进行安全通讯，然后由 `zsudo-srv` 验证用户合法后启动管理员权限进程达到提权的目的，与 UAC 相比，存在一些不同，第一提权是完全是命令行的，不需要启动安全桌面，子进程可以继承现有的控制台或者终端。

### ZSUDO 安全机制

ZSUDO 客户端与服务端通讯是使用 [`Unix domain socket`](https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows/) 和选取的 TLS 库实现安全通讯。

[https://github.com/MicrosoftDocs/WSL/blob/live/wsl-samples/afunix-wsl-interop/windows_server/af_unix_win_server.cpp](https://github.com/MicrosoftDocs/WSL/blob/live/wsl-samples/afunix-wsl-interop/windows_server/af_unix_win_server.cpp)

TLS 库可选择： [`Schannel`](https://docs.microsoft.com/en-us/windows/win32/com/schannel)

其他参考库: [https://github.com/awslabs/aws-c-io](https://github.com/awslabs/aws-c-io)

<!--[https://github.com/tlswg/tlswg-wiki/blob/master/IMPLEMENTATIONS.md](https://github.com/tlswg/tlswg-wiki/blob/master/IMPLEMENTATIONS.md)
[picotls](https://github.com/h2o/picotls)-->
