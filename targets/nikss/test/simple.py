from backends.p4tools.modules.flay.targets.nikss.common import P4EbpfTest
from ptf import testutils as ptfutils # type: ignore

PORT0 = 0
PORT1 = 1
PORT2 = 2
PORT3 = 3
PORT4 = 4
PORT5 = 5
ALL_PORTS = [PORT0, PORT1, PORT2, PORT3]


class SimpleTest(P4EbpfTest):

    def runTest(self) -> None:
        # check no connectivity if switching rules are not installed
        pkt = b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        ptfutils.send_packet(self, PORT0, pkt)
        ptfutils.verify_packet(self, pkt, PORT1)
