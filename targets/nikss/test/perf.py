import json
import os
import pprint
import sys
import time

from backends.p4tools.modules.flay.targets.nikss.common import P4EbpfTest
from tools import testutils

trex_path = "/mnt/storage/Projekte/trex-core/scripts/automation/trex_control_plane/interactive"
sys.path.insert(0, trex_path)


scripts_path = "/mnt/storage/Projekte/trex-core/scripts/"
STL_PROFILES_PATH = os.path.join(scripts_path, 'stl')
EXT_LIBS_PATH = os.path.join(scripts_path, 'external_libs')
assert os.path.isdir(STL_PROFILES_PATH)
assert os.path.isdir(EXT_LIBS_PATH)

assert os.path.isdir(STL_PROFILES_PATH), 'Could not determine STL profiles path'
assert os.path.isdir(EXT_LIBS_PATH), 'Could not determine external_libs path'
from trex.stl.api import *


def rx_example(tx_port: int, rx_port: int, burst_size: int, pps: int) -> bool:

    print(
        f"\nGoing to inject {burst_size} packets on port {tx_port} - checking RX stats on port {rx_port}\n"
    )

    # create client
    c = STLClient()
    passed = True

    try:
        pkt = STLPktBuilder(
            pkt=Ether(src="00:11:22:33:44:55", dst="00:77:66:55:44:33")
            / IP(src="16.0.0.1", dst="48.0.0.1")
            / UDP(dport=12, sport=1025)
            / 'at_least_16_bytes_payload_needed'
        )
        total_pkts = burst_size
        s1 = STLStream(
            name='rx',
            packet=pkt,
            flow_stats=STLFlowLatencyStats(pg_id=5),
            mode=STLTXSingleBurst(total_pkts=total_pkts, pps=pps),
        )

        # connect to server
        c.connect()

        # prepare our ports
        c.reset(ports=[tx_port, rx_port])

        # add both streams to ports
        c.add_streams([s1], ports=[tx_port])

        print("\nInjecting {0} packets on port {1}\n".format(total_pkts, tx_port))

        rc = rx_iteration(c, tx_port, rx_port, total_pkts, pkt.get_pkt_len())
        if not rc:
            passed = False

    except STLError as e:
        passed = False
        print(e)

    finally:
        c.disconnect()

    if passed:
        print("\nTest passed :-)\n")
    else:
        print("\nTest failed :-(\n")
    return passed


# RX one iteration
def rx_iteration(c: STLClient, tx_port: int, rx_port: int, total_pkts: int, pkt_len: int) -> bool:

    c.clear_stats()

    c.start(ports=[tx_port])
    pgids = c.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))

    for i in range(1, 8):
        time.sleep(1)
        stats = c.get_pgid_stats(pgids['latency'])
        flow_stats = stats['flow_stats'].get(5)
        rx_pps = flow_stats['rx_pps'][rx_port]
        tx_pps = flow_stats['tx_pps'][tx_port]
        rx_bps = flow_stats['rx_bps'][rx_port]
        tx_bps = flow_stats['tx_bps'][tx_port]
        rx_bps_l1 = flow_stats['rx_bps_l1'][rx_port]
        tx_bps_l1 = flow_stats['tx_bps_l1'][tx_port]
        print(
            "rx_pps:{0} tx_pps:{1}, rx_bps:{2}/{3} tx_bps:{4}/{5}".format(
                rx_pps, tx_pps, rx_bps, rx_bps_l1, tx_bps, tx_bps_l1
            )
        )
    c.wait_on_traffic(ports=[tx_port])
    stats = c.get_pgid_stats(pgids['latency'])
    flow_stats = stats['flow_stats'].get(5)
    global_lat_stats = stats['latency']
    lat_stats = global_lat_stats.get(5)
    if not flow_stats:
        print("no flow stats available")
        return False
    if not lat_stats:
        print("no latency stats available")
        return False

    tx_pkts = flow_stats['tx_pkts'].get(tx_port, 0)
    tx_bytes = flow_stats['tx_bytes'].get(tx_port, 0)
    rx_pkts = flow_stats['rx_pkts'].get(rx_port, 0)
    drops = lat_stats['err_cntrs']['dropped']
    ooo = lat_stats['err_cntrs']['out_of_order']
    dup = lat_stats['err_cntrs']['dup']
    sth = lat_stats['err_cntrs']['seq_too_high']
    stl = lat_stats['err_cntrs']['seq_too_low']
    old_flow = global_lat_stats['global']['old_flow']
    bad_hdr = global_lat_stats['global']['bad_hdr']
    lat = lat_stats['latency']
    jitter = lat['jitter']
    avg = lat['average']
    tot_max = lat['total_max']
    tot_min = lat['total_min']
    last_max = lat['last_max']
    hist = lat['histogram']

    if c.get_warnings():
        print("\n\n*** test had warnings ****\n\n")
        for w in c.get_warnings():
            print(w)
        return False

    print(
        f'Error counters: dropped:{drops}, ooo:{ooo} dup:{dup} seq too high:{sth} seq too low:{stl}'
    )
    if old_flow:
        print(f'Packets arriving too late after flow stopped: {old_flow}')
    if bad_hdr:
        print(f'Latency packets with corrupted info: {bad_hdr}')
    print('Latency info:')
    print(f"  Maximum latency(usec): {tot_max}")
    print(f"  Minimum latency(usec): {tot_min}")
    print(f"  Maximum latency in last sampling period (usec): {last_max}")
    print(f"  Average latency(usec): {avg}")
    print(f"  Jitter(usec): {jitter}")
    print("  Latency distribution histogram:")
    l = list(hist.keys())  # need to listify in order to be able to sort them.
    l.sort()
    for sample in l:
        range_start = sample
        if range_start == 0:
            range_end = 10
        else:
            range_end = range_start + pow(10, (len(str(range_start)) - 1))
        val = hist[sample]
        print(
            "    Packets with latency between {0} and {1}:{2} ".format(range_start, range_end, val)
        )

    if tx_pkts != total_pkts:
        print("TX pkts mismatch - got: {0}, expected: {1}".format(tx_pkts, total_pkts))
        pprint.pprint(flow_stats)
        return False
    else:
        print("TX pkts match   - {0}".format(tx_pkts))

    if tx_bytes != (total_pkts * (pkt_len + 4)):  # +4 for ethernet CRC
        print(
            "TX bytes mismatch - got: {0}, expected: {1}".format(tx_bytes, (total_pkts * pkt_len))
        )
        pprint.pprint(flow_stats)
        return False
    else:
        print("TX bytes match  - {0}".format(tx_bytes))

    if rx_pkts != total_pkts:
        print("RX pkts mismatch - got: {0}, expected: {1}".format(rx_pkts, total_pkts))
        pprint.pprint(flow_stats)
        return False
    else:
        print("RX pkts match   - {0}".format(rx_pkts))
    return True


class PerfTest(P4EbpfTest):

    def runTest(self) -> None:
        # testutils.exec_process("wireshark & ", shell=True)
        # time.sleep(3)
        self.trex_proc = testutils.exec_process(
            "cd /mnt/storage/Projekte/trex-core/scripts && ./t-rex-64 --software -i --cfg /mnt/storage/Projekte/gauntlet/modules/p4c/backends/p4tools/modules/flay/targets/nikss/switch_cfg.yaml >/dev/null 2>&1 &",
            shell=True,
            capture_output=False,
        )
        if self.trex_proc is None:
            print("Failed to start trex")
            return
        time.sleep(2)
        self.assertTrue(
            rx_example(tx_port=0, rx_port=1, burst_size=10000000, pps=1000000), "Failed"
        )
        stats = testutils.exec_process(
            "/mnt/storage/Projekte/gauntlet/modules/p4c/backends/p4tools/modules/flay/targets/nikss/bpftool/src/bpftool prog show name xdp_ingress_func -j"
        )
        if stats.output:
            stats_obj = json.loads(stats.output)
            testutils.log.info(stats_obj)
            print(
                "Nanoseconds spent per packet: %s"
                % {stats_obj["run_time_ns"] / stats_obj["run_cnt"]}
            )
