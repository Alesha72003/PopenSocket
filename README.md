# PopenSocket
This module for python3 creates socket-like object for communication with child process on Windows using WinAPI.

Anonymous pipes don't have async API, so module uses named pipes to read child's stdout.

## Installation

Copy dll from release dir to working dir, or to libs dir (`<python.exe dir>/Lib`)

## Peculiarity:
- Timeout is used only for reading, writting is always synchronous.

- This object should be able to be used in `select()`

- Recieve only from `stdout`

  `stderr` redirected to `stderr` parent process

Example of useage:
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

## Build

VS Studio compiler is required for build
```
python.exe .\setup.py build
```

Result will be available in `build\libs`

## Conclusion
<details>
  <summary>My personal advice</summary>
  While I was researching this topic I found a forum post from 2005. In that post senior C developer described a problem of absence Async API for anonymous pipe in windows api. This problem has not been solved until now.
  
  My advice: Use Linux instead of Windows. In Linux this functionallity was supported "from the box"
</details>
