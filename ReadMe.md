# Privexec

Execute some command

## Alias

If you want add a alias to Privexec, Please modify Privexec.json on your Privexec.exe location.

```json
{
    "Alias": [
        {
            "Name": "Edit Hosts",
            "Command": "Notepad %windir%\\System32\\Drivers\\etc\\hosts"
        },
        {
            "Name": "PowerShell",
            "Command": "powershell"
        },
        {
            "Name": "PowerShell ISE",
            "Command": "powershell_ise"
        },
        {
            "Name": "Windows Debugger",
            "Command": "\"%ProgramFiles(x86)%\\Windows Kits\\10\\Debuggers\\x64\\windbg.exe\""
        }
    ]
}
```

wsudo alias ? working in progress


## Screenshot

![image](images/admin.png)

wsudo:

![image](images/wsudo.png)

## LICENSE

This project use MIT License, and JSON use https://github.com/nlohmann/json , some API use NSudo.