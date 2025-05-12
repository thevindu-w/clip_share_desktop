import socket
import getopt
import os
import sys

PROTO_MIN = 1
PROTO_MAX = 3
BIND_ADDR = '127.0.0.1'
PORT = 4337
DISABLED_METHODS = []
COPIED_TEXT = None
IMAGE = None

options, _ = getopt.getopt(sys.argv[1:], "", ['proto-min=', 'proto-max=', 'bind=', 'port=', 'disabled-methods=', 'text=', 'image='])
for opt, arg in options:
    arg = arg.strip()
    if opt in ('--proto-min'):
        PROTO_MIN = int(arg)
    elif opt in ('--proto-max'):
        PROTO_MAX = int(arg)
    elif opt in ('--bind'):
        BIND_ADDR = arg
    elif opt in ('--port'):
        PORT = int(arg)
    elif opt in ('--disabled-methods'):
        DISABLED_METHODS = arg.split(',')
    elif opt in ('--text'):
        COPIED_TEXT = arg
    elif opt in ('--image'):
        IMAGE = arg

SERVER_DIR = os.path.dirname(__file__)

PROTO_OK = b'\x01'
PROTO_OBSOLETE = b'\x02'
PROTO_UNKNOWN = b'\x03'

STATUS_OK = b'\x01'
STATUS_NO_DATA = b'\x02'
STATUS_UNKNOWN_METHOD = b'\x03'
STATUS_METHOD_NOT_IMPLEMENTED = b'\x04'

server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_sock.bind((BIND_ADDR, PORT))
server_sock.listen(3)

def send_int(sock: socket.socket, value: int) -> None:
    b = value.to_bytes(8, 'big')
    sock.sendall(b)

def read_int(sock: socket.socket) -> int:
    b = sock.recv(8)
    assert len(b) == 8
    return int.from_bytes(b, 'big')

def send_data(sock: socket.socket, data: bytes) -> None:
    send_int(sock, len(data))
    sock.sendall(data)

def read_data(sock: socket.socket) -> bytes:
    size = read_int(sock)
    data = sock.recv(size)
    assert len(data) == size
    return data

def handle_get_text(sock: socket.socket) -> None:
    if COPIED_TEXT == None:
        sock.sendall(STATUS_NO_DATA)
        return
    sock.sendall(STATUS_OK)
    data = COPIED_TEXT.encode('utf-8')
    send_data(sock, data)
    print('Sent text')

def handle_send_text(sock: socket.socket) -> None:
    sock.sendall(STATUS_OK)
    data = read_data(sock)
    text = data.decode('utf-8')
    print(f'Received text: {text}')

def handle_get_image(sock: socket.socket) -> None:
    sock.sendall(STATUS_OK)
    if IMAGE == 'copied':
        img_file = 'image.png'
    else:
        img_file = 'screen.png'
    img_file = os.path.join(SERVER_DIR, '../files', img_file)
    with open(img_file, 'rb') as img:
        data = img.read()
    send_data(sock, data)
    print(f'Sent {IMAGE} image')

def handle_get_file(sock: socket.socket) -> None:
    sock.sendall(STATUS_METHOD_NOT_IMPLEMENTED)

def handle_send_file(sock: socket.socket):
    sock.sendall(STATUS_METHOD_NOT_IMPLEMENTED)

def handle_get_copied_image(sock: socket.socket) -> None:
    sock.sendall(STATUS_METHOD_NOT_IMPLEMENTED)

def handle_get_screenshot(sock: socket.socket) -> None:
    sock.sendall(STATUS_METHOD_NOT_IMPLEMENTED)

def handle_info(sock: socket.socket) -> None:
    sock.sendall(STATUS_METHOD_NOT_IMPLEMENTED)

def handle_protocol(sock: socket.socket, version: int) -> None:
    print(f"Using protocol version {version}")
    method = ord(sock.recv(1))
    print(f"Client requested method {method}")
    if method in DISABLED_METHODS:
        sock.sendall(STATUS_METHOD_NOT_IMPLEMENTED)
        return

    if version == 1:
        ALLOWED_METHODS = [1,2,3,4,5,125]
    elif version == 2 or version == 3:
        ALLOWED_METHODS = [1,2,3,4,5,6,7,125]
    if method not in ALLOWED_METHODS:
        sock.sendall(STATUS_UNKNOWN_METHOD)
        return

    if method == 1:
        handle_get_text(sock)
    elif method == 2:
        handle_send_text(sock)
    elif method == 3:
        handle_get_file(sock, version)
    elif method == 4:
        handle_send_file(sock, version)
    elif method == 5:
        handle_get_image(sock)
    elif method == 6:
        handle_get_copied_image(sock)
    elif method == 7:
        handle_get_screenshot(sock)
    elif method == 125:
        handle_info(sock)

def negotiate_protocol(sock: socket.socket) -> None:
    client_version = ord(sock.recv(1))
    if client_version < PROTO_MIN:
        print(f"Client version {client_version} is obsolete")
        sock.sendall(PROTO_OBSOLETE)
        return
    elif client_version > PROTO_MAX:
        print(f"Client version {client_version} is unknown")
        sock.sendall(PROTO_UNKNOWN)
        sock.sendall(bytes([PROTO_MAX]))
        response = sock.recv(1)
        if response == b'\x00':
            print("Client rejected the offered version")
            return
        elif response == bytes([PROTO_MAX]):
            print(f"Client accepted version {PROTO_MAX}")
            handle_protocol(sock, PROTO_MAX)
            return
        else:
            print("Client rejected the offered version with invalid response " + str(client_version))
            return
    else:
        print(f"Client version {client_version} is supported")
        sock.sendall(PROTO_OK)
        handle_protocol(sock, client_version)
        return

client_sock, _ = server_sock.accept()
client_sock.settimeout(0.05)
negotiate_protocol(client_sock)
client_sock.close()
