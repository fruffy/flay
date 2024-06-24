#!/usr/bin/env python3

# from protofuzz import protofuzz

import argparse
import random

import bfruntime_pb2 as bfrt
from google.protobuf import text_format

PARSER = argparse.ArgumentParser()
PARSER.add_argument(
    "-o",
    "--output-file",
    dest="output_file",
    default="generated_update.txtpb",
    help="The name of the output text protobuf file.",
)
PARSER.add_argument(
    "-n",
    "--num-permutations",
    dest="num_permutations",
    default=1,
    type=int,
    help="The number of permutations to generate.",
)


# Parse options and process argv
ARGS, ARGV = PARSER.parse_known_args()


def random_int_for_bytes(num_bytes: int) -> int:
    """Generates a random integer that fits in a specified number of bytes."""
    num_bytes = abs(num_bytes)
    max_value = (1 << (num_bytes * 8)) - 1
    return random.randint(0, max_value)


def mutate_stream_value(stream_value: bytes) -> bytes:
    mutated_integer = random_int_for_bytes(len(stream_value))
    print(f"Mutated integer: {mutated_integer} Bytes: {len(stream_value)}")
    return mutated_integer.to_bytes(len(stream_value), "little")


def generated_mutated_table_entry(table_entry: bfrt.Update) -> bfrt.Update:
    mutated_table_entry = bfrt.Update()
    mutated_table_entry.CopyFrom(table_entry)
    for field in table_entry.entity.table_entry.key.fields:
        match_type = field.WhichOneof("match_type")
        if match_type == "exact":
            field.exact.value = mutate_stream_value(field.exact.value)
        elif match_type == "ternary":
            field.ternary.value = mutate_stream_value(field.ternary.value)
            field.ternary.mask = mutate_stream_value(field.ternary.mask)
        elif match_type == "lpm":
            field.lpm.value = mutate_stream_value(field.lpm.mask)
            field.lpm.prefix_len = mutate_stream_value(field.lpm.prefix)
        elif match_type == "range":
            field.range.low = mutate_stream_value(field.range.low)
            field.range.high = mutate_stream_value(field.range.high)
        elif match_type == "optional":
            field.optional.value = mutate_stream_value(field.optional.value)
        else:
            raise Exception(f"Unknown field type: {field}")
    for field in table_entry.entity.table_entry.data.fields:
        field.stream = mutate_stream_value(field.stream)
    return mutated_table_entry


if __name__ == "__main__":

    protobuf_data = b"""
    type: INSERT
    entity {
      table_entry {
        table_id: 38270129
        key {
          fields {
            field_id: 4
            ternary {
              value: "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
              mask: "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
            }
          }
          fields {
            field_id: 3
            ternary {
              value: "\002\001\002"
              mask: "\000\000\000\000"
            }
          }
          fields {
            field_id: 1
            exact {
              value: "\000"
            }
          }
          fields {
            field_id: 2
            exact {
              value: "\000"
            }
          }
        }
        data {
          action_id: 18771031
          fields {
            field_id: 2
            stream: "\002\000\000\001\001\001"
          }
          fields {
            field_id: 3
            stream: "\000"
          }
          fields {
            field_id: 1
            stream: "\001\010"
          }
        }
      }
    }
    """

    update = bfrt.Update()
    try:
        text_format.Parse(protobuf_data, update)
    except Exception as e:
        raise Exception(f"Error parsing text Protobuf message: {e}") from e

    generated_protos = set({update.entity.table_entry.key.SerializeToString()})

    write_request = bfrt.WriteRequest()

    for i in range(ARGS.num_permutations):
        mutated_table_entry = generated_mutated_table_entry(update)
        while mutated_table_entry.entity.table_entry.key.SerializeToString() in generated_protos:
            mutated_table_entry = generated_mutated_table_entry(update)
        generated_protos.add(mutated_table_entry.entity.table_entry.key.SerializeToString())
        write_request.updates.append(mutated_table_entry)

    with open(ARGS.output_file, "w+", encoding="utf8") as proto_file:
        proto_file.write(text_format.MessageToString(write_request))
