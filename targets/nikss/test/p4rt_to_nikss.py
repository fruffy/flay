#!/usr/bin/env python3

from pathlib import Path
from google.protobuf import text_format
from p4.config.v1 import p4info_pb2 as p4info
from p4.v1 import p4runtime_pb2 as p4rt
from collections import Counter


def parse_p4info(file_path : Path) -> p4info.P4Info:
    """Parse a P4Info file into a P4Info object."""

    with file_path.open("r") as f:
        message = p4info.P4Info()
        text_format.Parse(f.read(), message)
    return message


class P4rtToNikss:
    def __init__(self, p4info: Path, pipe_id: int) -> None:
        self.p4info = parse_p4info(p4info)
        self.pipe_id = pipe_id
        self.p4info_obj_name_map :dict = {}
        self.p4info_obj_id_map :dict = {}
        self.import_p4info_names()

    def hex_binary_to_int(self, hex_binary : bytes) -> int:
        """Converts a hex binary string (e.g., b'\x00\x00\x00\x00\x00\x00') to an integer."""
        return int.from_bytes(hex_binary, byteorder='big')

    # In order to make writing tests easier, we accept any suffix that uniquely
    # identifies the object among p4info objects of the same type.
    def import_p4info_names(self) -> None:
        suffix_count : Counter = Counter()
        for obj_type in [
            "tables",
            "action_profiles",
            "actions",
            "counters",
            "direct_counters",
            "controller_packet_metadata",
            "meters",
            "direct_meters",
        ]:
            for obj in getattr(self.p4info, obj_type):
                pre = obj.preamble
                suffix = None
                for s in reversed(pre.name.split(".")):
                    suffix = s if suffix is None else s + "." + suffix
                    key = (obj_type, suffix)
                    self.p4info_obj_name_map[key] = obj
                    suffix_count[key] += 1
                self.p4info_obj_id_map[(obj_type, pre.id)] = obj

        for key, c in list(suffix_count.items()):
            if c > 1:
                del self.p4info_obj_name_map[key]

    def translate_key(self, match: p4rt.FieldMatch) -> str:
        match_str = "key "
        if match.HasField("exact"):
            val = self.hex_binary_to_int(match.exact.value)
            match_str += f"{val} "
        elif match.HasField("lpm"):
            val = self.hex_binary_to_int(match.lpm.value)
            mask = self.hex_binary_to_int(match.lpm.mask)
            match_str += f"{val}/{mask} "
        elif match.HasField("range"):
            val = self.hex_binary_to_int(match.range.value)
            mask = self.hex_binary_to_int(match.range.mask)
            match_str += f"{val}-{mask} "
        elif match.HasField("ternary"):
            val = self.hex_binary_to_int(match.ternary.value)
            mask = self.hex_binary_to_int(match.ternary.mask)
            match_str += f"{val}^{mask} "
        else:
            raise NotImplementedError("Unknown match type")
        return match_str

    def translate_table_write_request(
        self, table_entry: p4rt.TableEntry, message_type: p4rt.Update.Type
    ) -> str:
        table_obj = self.p4info_obj_id_map[("tables", table_entry.table_id)]
        table_name = table_obj.preamble.name.replace(".", "_")
        print(table_name)
        cmd = f"nikss-ctl table add pipe {self.pipe_id} {table_name} "
        action_entry = table_entry.action.action
        action_obj = self.p4info_obj_id_map[("actions", action_entry.action_id)]
        action_name = action_obj.preamble.name.replace(".", "_")
        cmd += f"action name {action_name} "
        for match in table_entry.match:
            cmd += self.translate_key(match)
        cmd += "data "
        for param in action_entry.params:
            cmd += f"{self.hex_binary_to_int(param.value)} "
        return cmd

    def translate_write_request(self, p4rt_write_request: p4rt.WriteRequest) -> list[str]:
        commands = []
        for update in p4rt_write_request.updates:
            entity = update.entity
            if entity.HasField("table_entry"):
               commands.append(self.translate_table_write_request(entity.table_entry, update.type))
            else:
                raise NotImplementedError("Unknown entity type")
        return commands

test_proto: str = r"""
updates {
    entity {
      # Table ingress.toggle_check
      table_entry {
        table_id: 49220983
        priority: 1
        # Match field dst_eth
        match {
          field_id: 1
          ternary {
            value: "\x00\x00\x00\x00\x00\x01"
            mask: "\xFF\xFF\xFF\xFF\xFF\xFF"
          }
        }
        # Action ingress.check
        action {
          action {
            action_id: 26996539
            params {
              param_id: 1
              value: "\x01"
            }
        }
      }
    }
  },
  type: INSERT
}
"""


if __name__ == "__main__":
    p4rt_nikss = P4rtToNikss(Path("simple.p4info.txtpb"), 0)
    parsed_proto: p4rt.WriteRequest = text_format.Parse(test_proto, p4rt.WriteRequest())
    result = p4rt_nikss.translate_write_request(parsed_proto)
    print(result)
