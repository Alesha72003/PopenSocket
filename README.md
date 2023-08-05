# PopenSocket
This module for python3 create socket-like object for communicate with child process on Windows using WinAPI.

Module use named pipes to read child stdout, because anonymous pipes not have async API.

## Peculiarity:
- Timeout used only for reading, writting always sync.

- Maybe this object can be used in `select()`

- Recieve only from `stdout`, `stderr` redirected to `stderr` parent process

Example of use:
```python
from PopenSocket import PopenSocket
from socket import timeout # import from socket exception (used for compability)

sock = PopenSocket("C:\Windows\System32\cmd.exe", "/c echo 1&ping -n 10 127.0.0.1 > null&echo 2") # ping for sleep
sock.settimeout(1) # 1 second timeout
while True:
  try:
    print(sock.recv(1024))
  except timeout:
    print("timeout")
  except ConnectionAbortedError:
    print("END")
    break
```

Expected output:
```
b'1\r\n'
timeout
timeout
timeout
timeout
timeout
timeout
timeout
timeout
timeout
b'2\r\n'
b''
END
```

## API
### Constructor

```
PopenSocket(pathToExecutable, args)
```
### Methods
`settimeout(seconds)`

`recv(sizeToRead)` *Warning* Readed message may be smaller than `sizeToRead`, like in standart socket.

`send(bytesToWrite)`

`close()` - Kills child process

### Exceptions

`socket.timeout` - timeout

`ConnectionAbortedError` - child close stdout or child done
