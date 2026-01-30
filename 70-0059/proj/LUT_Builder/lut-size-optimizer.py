import math
import sys

class lut_header:
    header_entry_size : int
    header_preamble_size : int
    header_entry_count : int

    def __init__(self, header_entry_size : int, header_preamble_size : int):
        self.header_entry_size = header_entry_size
        self.header_preamble_size = header_preamble_size
        self.header_entry_count = 0

    def get_pages(self, page_size : int) -> int:
        return math.ceil(float(self.header_preamble_size + self.header_entry_count*self.header_entry_size)/page_size)

    def min_size(self) -> int:
        return max(self.header_preamble_size, self.header_entry_size)
    
    def max_size(self) -> int:
        return self.header_preamble_size + self.header_entry_size*self.header_entry_count

class lut_entry:
    count : int
    entry_size : int

    def __init__(self, count : int, entry_size : int):
        self.count = count
        self.entry_size = entry_size

    def get_pages(self, page_size : int) -> int:
        return math.ceil(float(self.count*self.entry_size)/page_size)
    
    def min_size(self) -> int:
        return self.entry_size

    def max_size(self) -> int:
        return self.entry_size * self.count

def get_page_count(page_size : int, list : list[lut_entry | lut_header]) -> int:
    a : int = 0

    for i in list:
        a += i.get_pages(page_size)

    return a


# Entry format:
# 1: EFS Page Size
# 2: Size of header entry
# 3: Size of header preamble
# 4: Size of LUT item 1
# 5: Number of LUT item 1
# 6: Size of LUT item 2
# 7: Number of LUT item 2
# ...
if __name__ == "__main__":
    items = []

    EFS_PAGE_SIZE = int(sys.argv[1])

    h = lut_header(
        int(sys.argv[2]),
        int(sys.argv[3])
    )

    M = len(sys.argv) - 1
    M = M - M % 2 + 2 

    h.header_entry_count = (M - 3) / 2

    for i in range(4, M, 2):
        size = int(sys.argv[i])
        num = int(sys.argv[i + 1])
        items.append(lut_entry(
            num,
            size
        ))
    items.append(h)

    MIN_VALUE = max([i.min_size() for i in items]) or 0
    MAX_VALUE = max([i.max_size() for i in items]) or 0

    print("LUT Page Size (bytes), Total Size (bytes), Total LUT Pages, Total EFS Pages")
    for i in range(MIN_VALUE, MAX_VALUE + 1):
        k = get_page_count(i, items)
        print("%d,%d,%d,%d" % (i, i*k, k, math.ceil(float(k)/EFS_PAGE_SIZE)))