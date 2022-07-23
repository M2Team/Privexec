# Changelog

## 2018-12-28

1.  Privexec/WSUDO support `Less Privileged AppContainer`
2.  Privexec AppContainer supports more capabilities to choose from

## 2018-12-22

1.  Privexec (GUI) AppContainer support Restricted Capabilities.  
`Recommended Capabilities` can be selected at the same time as `Appmanifest`, the `Capabilities `will be merged
2.  Privexec (GUI) Support Dropfiles. Appx edit box accept `*.xml;*.appmanifest` if it is activated. Command edit box accept other files.

## 2022-07-22

1. wsudo -U will create a process without privilege escalation. If it is currently running under the System account, wsudo -U cannot start the Store App directly, but it can start the Store App in the launched shell.（在系统权限下，wsudo -U 无法直接启动 Store App，但是可以在启动的 shell 中启动 Store App。）
2. wsudo add -B --basic as default (permission inheritance mode)（wsudo 添加 `-B` `--basic` 作为默认执行模式，即继承父进程权限，而 -U 则总是启动未提权的进程。）