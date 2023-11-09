import struct
from datasketches import PyObjectSerDe


# https://github.com/apache/datasketches-cpp/issues/405
class PyMixedItemsSerDe(PyObjectSerDe):
    def get_size(self, item):
        item_type = type_map(type(item).__name__)
        if item_type == "str":
            # type (1 char), length (4 bytes), string
            return int(5 + len(item))
        else:
            # type (1 char), value (8 bytes, whether int or float)
            return int(9)

    def to_bytes(self, item):
        item_type = type_map(type(item).__name__)
        b = bytearray()
        if item_type == "str":
            b.extend(b"s")
            b.extend(len(item).to_bytes(4, "little"))
            b.extend(map(ord, item))
        elif item_type == "int":
            b.extend(b"q")
            b.extend(struct.pack("<q", item))
        elif item_type == "float":
            b.extend(b"d")
            b.extend(struct.pack("<d", item))
        else:
            raise Exception(
                f"Only str, int, and float are supported. Found {item_type}"
            )
        return bytes(b)

    def from_bytes(self, data: bytes, offset: int):
        item_type = chr(data[offset])
        if item_type == "s":
            num_chars = int.from_bytes(data[offset + 1 : offset + 4], "little")
            if num_chars < 0 or num_chars > offset + len(data):
                raise IndexError(
                    f"num_chars read must be non-negative and not larger than the buffer. Found {num_chars}"
                )
            val = data[offset + 5 : offset + 5 + num_chars].decode()
            return (val, 5 + num_chars)
        elif item_type == "q":
            val = struct.unpack_from("<q", data, offset + 1)[0]
            return (val, 9)
        elif item_type == "d":
            val = struct.unpack_from("<d", data, offset + 1)[0]
            return (val, 9)
        else:
            raise Exception("Unknown item type found")


def type_map(item_type: str):
    # int64, int32 -> int
    if "int" in item_type:
        return "int"
    # float64, float32 -> float
    elif "float" in item_type:
        return "float"
    else:
        return item_type
