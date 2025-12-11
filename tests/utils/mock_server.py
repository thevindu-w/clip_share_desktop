import getopt
import os
import socket
import ssl
import sys
import time

TLS_ENABLED = False
PROTO_MIN = 1
PROTO_MAX = 3
BIND_ADDR = '127.0.0.1'
PORT = 4337
TLS_PORT = 4338
DISABLED_METHODS = []
COPIED_TEXT = None
FILES_COPIED = False
IMAGE = None

options, _ = getopt.getopt(sys.argv[1:], "", ['tls=', 'proto-min=', 'proto-max=', 'bind=', 'port=', 'disabled-methods=', 'text=', 'image=', 'files='])
for opt, arg in options:
    arg = arg.strip()
    if opt == '--tls':
        TLS_ENABLED = True
        PORT = TLS_PORT
    elif opt == '--proto-min':
        PROTO_MIN = int(arg)
    elif opt == '--proto-max':
        PROTO_MAX = int(arg)
    elif opt == '--bind':
        BIND_ADDR = arg
    elif opt == '--port':
        PORT = int(arg)
        TLS_PORT = PORT
    elif opt == '--disabled-methods':
        DISABLED_METHODS = arg.split(',')
    elif opt == '--text':
        COPIED_TEXT = arg
    elif opt == '--image':
        IMAGE = arg
    elif opt == '--files':
        FILES_COPIED = True

FILES_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'files'))
TLS_CERT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'tmp'))

PROTO_OK = b'\x01'
PROTO_OBSOLETE = b'\x02'
PROTO_UNKNOWN = b'\x03'

STATUS_OK = b'\x01'
STATUS_NO_DATA = b'\x02'
STATUS_UNKNOWN_METHOD = b'\x03'
STATUS_METHOD_NOT_IMPLEMENTED = b'\x04'

def start_server():
    global server_sock
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((BIND_ADDR, PORT))
    server_sock.listen(3)

def init_ssl():
    global context
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile=os.path.join(TLS_CERT_DIR, 'testServer.crt'), keyfile=os.path.join(TLS_CERT_DIR, 'testServer.key'))
    context.load_verify_locations(os.path.join(TLS_CERT_DIR, 'testCA.crt'))
    context.verify_mode = ssl.CERT_REQUIRED

def send_int(sock: socket.socket, value: int) -> None:
    b = value.to_bytes(8, 'big')
    sock.sendall(b)

def read_int(sock: socket.socket) -> int:
    b = sock.recv(8)
    assert len(b) == 8
    val = int.from_bytes(b, 'big')
    if val >= 1<<32: val -= 1<<64
    return val

def send_data(sock: socket.socket, data: bytes) -> None:
    send_int(sock, len(data))
    sock.sendall(data)

def read_data(sock: socket.socket, size: int = None) -> bytes:
    if size == None:
        size = read_int(sock)
    data = sock.recv(size)
    assert len(data) == size
    return data

def send_file(sock: socket.socket, path: str) -> None:
    path = os.path.relpath(path, '.')
    print(f'Sending {path}')
    send_data(sock, path.encode('utf-8'))
    if os.path.isdir(path):
        send_int(sock, (2**64)-1)
        print('Sent dir')
        return
    with open(path, 'rb') as f:
        send_data(sock, f.read())
    print('Sent file')

def handle_get_text(sock: socket.socket) -> None:
    if COPIED_TEXT == None:
        sock.sendall(STATUS_NO_DATA)
        print("No copied text")
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
    img_file = os.path.join(FILES_DIR, img_file)
    with open(img_file, 'rb') as img:
        data = img.read()
    send_data(sock, data)
    print(f'Sent {IMAGE} image')

def handle_get_file(sock: socket.socket, version: int) -> None:
    if not FILES_COPIED:
        sock.sendall(STATUS_NO_DATA)
        print("No copied files")
        return
    sock.sendall(STATUS_OK)
    if version == 1:
        path = 'files_v1'
    elif version == 2:
        path = 'files_v2'
    elif version == 3:
        path = 'files_v3'
    os.chdir(os.path.join(FILES_DIR, path))
    file_cnt = 0
    for _, dirs, files in os.walk('.'):
        files = list(filter(lambda f: f[0] != '.', files))
        file_cnt += len(files)
        if version == 3 and len(files) == 0 and len(dirs) == 0:
            file_cnt += 1
    print(f'Sending {file_cnt} files')
    send_int(sock, file_cnt)
    for root, dirs, files in os.walk('.'):
        files = list(filter(lambda f: f[0] != '.', files))
        files.sort()
        dirs.sort()
        for f in files:
            send_file(sock, os.path.join(root, f))
        if version == 3 and len(files) == 0 and len(dirs) == 0:
            send_file(sock, root)

def handle_send_file(sock: socket.socket, version: int) -> None:
    sock.sendall(STATUS_OK)
    if version == 1:
        file_cnt = 1
    else:
        file_cnt = read_int(sock)
    if file_cnt <= 0:
        print(f'Invalid file count {file_cnt}')
        return
    received_list = []
    for _ in range(file_cnt):
        received_list.append([])
        fname = read_data(sock).decode('utf-8')
        received_list[-1].append(f'Received file name {fname}')
        file_sz = read_int(sock)
        received_list[-1].append(f'Received file size {file_sz}')
        if version < 3 and file_sz < 0:
            received_list[-1].append(f'Invalid file size {file_sz}')
            break
        if file_sz == -1:
            os.makedirs(fname)
            continue
        parent = os.path.dirname(fname)
        if parent:
            os.makedirs(os.path.dirname(fname), exist_ok=True)
        with open(fname, 'xb') as f:
            f.write(read_data(sock, file_sz))
    received_list.sort() # to keep the same order to compare with the expected output
    for messages in received_list:
        for message in messages:
            print(message)

def handle_get_copied_image(sock: socket.socket) -> None:
    if IMAGE != 'copied':
        sock.sendall(STATUS_NO_DATA)
        print("No copied image")
        return
    sock.sendall(STATUS_OK)
    img_file = os.path.join(FILES_DIR, 'image.png')
    with open(img_file, 'rb') as img:
        data = img.read()
    send_data(sock, data)
    print('Sent copied image')

def handle_get_screenshot(sock: socket.socket) -> None:
    sock.sendall(STATUS_OK)
    disp = read_int(sock)
    if disp not in (0, 1):
        sock.sendall(STATUS_NO_DATA)
        print(f"Not existing display: {disp}")
        return
    sock.sendall(STATUS_OK)
    img_file = os.path.join(FILES_DIR, 'screen.png')
    with open(img_file, 'rb') as img:
        data = img.read()
    send_data(sock, data)
    print(f'Sent screenshot of display {disp}')

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

try:
    if (TLS_ENABLED): init_ssl()
    start_server()
except:
    server_sock.close()
    temp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    temp_sock.connect((BIND_ADDR, PORT))
    temp_sock.send(b'\x00')
    temp_sock.close()
    time.sleep(0.05)
    start_server()

client_sock, _ = server_sock.accept()
server_sock.close()
client_sock.settimeout(0.05)
if TLS_ENABLED:
    client_sock = context.wrap_socket(client_sock, server_side=True)
negotiate_protocol(client_sock)
try:
    client_sock.recv(1) # wait for client to receive all data
except:
    pass
client_sock.close()
