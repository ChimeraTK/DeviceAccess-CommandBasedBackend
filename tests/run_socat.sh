echo "interface port: /tmp/virtual-tty"
echo "backend port: /tmp/virtual-tty-back"
socat -d -d pty,raw,echo=0,link=/tmp/virtual-tty pty,raw,echo=0,link=/tmp/virtual-tty-back
