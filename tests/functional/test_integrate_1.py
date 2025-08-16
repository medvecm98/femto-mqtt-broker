import logging
import socket
import ssl
import time


from ..common import mqtt_server


def test_malformed_packet(mqtt_server):
    """
    Send malformed packet (invalid remaining length encoding), expect connection tear down.
    """
    socket_v4 = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    socket_v4.connect(("localhost", mqtt_server.port))
    # valid CONNECT packet
    connect_packet = bytearray(
        [
            0x10,
            0x19,
            0x00,
            0x04,
            0x4D,
            0x51,
            0x54,
            0x54,
            0x04,
            0x02,
            0x00,
            0x3C,
            0x00,
            0x0D,
            0x6D,
            0x61,
            0x6E,
            0x79,
            0x5F,
            0x63,
            0x6F,
            0x6E,
            0x6E,
            0x65,
            0x63,
            0x74,
            0x73,
        ]
    )
    # Change the remaining length byte to indicate there is a continuation byte without inserting it.
    connect_packet[1] = 0x80
    socket_v4.send(connect_packet)
    # recv() will return 0 on orderly shutdown. This seems to be manifested in Python with b''.
    try:
        assert socket_v4.recv(1) == b""
    except ConnectionResetError:
        #
        # RST segment is fine, even though the spec MQTT-4.8.0-1 does not say
        # how to close the connection in such case.
        #
        pass