How to use the dummy:

1. start socat, which connect two virtual comm ports

$ socat -d -d pty,raw,echo=0 pty,raw,echo=0
2015/12/08 10:37:23 socat[8177] N PTY is /dev/pts/18
2015/12/08 10:37:23 socat[8177] N PTY is /dev/pts/19
2015/12/08 10:37:23 socat[8177] N starting data transfer loop with FDs [3,3] and [5,5]


Pick one of the two PTYs and start the dummy server on it:
$ ./dummy-server.py /dev/pts/18

Connect to the server via the other pts using the backend or a terminal client (putty, minicom etc.)
Note: the client must send CR at the end.
