import os
import struct
import sys

def get_memory_maps(pid):
    """Retrieve the list of memory maps for the given PID."""
    maps_file = f'/proc/{pid}/maps'
    with open(maps_file, 'r') as file:
        return file.readlines()

def parse_maps(maps):
    ranges = []
    for line in maps:
        parts = line.split()
        description = parts[-1]
        addr_range = parts[0].split('-')
        start_addr = int(addr_range[0], 16)
        end_addr = int(addr_range[1], 16)
        ranges.append((start_addr, end_addr, description))
    return ranges

def parse_pagemap(pid, ranges):
    pagemap_file = f'/proc/{pid}/pagemap'
    page_size = os.sysconf('SC_PAGE_SIZE')
    page_present = 1 << 63

    status = dict()

    with open(pagemap_file, 'rb') as file:
        for start_addr, end_addr, _ in ranges:
            # print(start_addr, end_addr)
            for addr in range(start_addr, end_addr, page_size):
                # Calculate the offset in the pagemap file
                offset = (addr // page_size) * 8
                file.seek(offset)
                data = file.read(8)
                if len(data) == 8:
                    entry = struct.unpack('Q', data)[0]
                    present = entry & page_present
                    status[addr] = int(present > 0)
    return status

def print_hist(bit_vector, bucket_size=2):
    density_chars = {
        0.8: '█',  # High density
        0.5: '▒',  # Medium density
        0.1: '░',  # Low density
        0: '·',    # No density
    }
    for i in range(0, len(bit_vector), bucket_size):
        bucket = bit_vector[i:i + bucket_size]
        density = sum(bucket) / bucket_size
        for threshold, char in sorted(density_chars.items(), reverse=True):
            if density > threshold:
                print(char, end='')
                break
        else:
            print(' ', end='')
    print()                    
            
if __name__ == '__main__':
    # Try to read the pagemaps of a victim
    # Try with:
    #    cprograms/gpt1 & echo $!
    if len(sys.argv) < 2:
        print("Usage: python pagemaps.py <PID>")
        sys.exit(1)

    pid = int(sys.argv[1])

    maps = get_memory_maps(pid)
    ranges = parse_maps(maps)

    status = parse_pagemap(pid, ranges)

    print('number of maps:', len(ranges))
    print('total pages:', len(parse_pagemap(pid, ranges)))

    # Optionally print text version
    if 'OUTPUT' in os.environ:
        for start,end,desc in ranges:
            if '[vsyscall]' in desc: continue
            amt = sum(status[k] for k in range(start,end,4096))
            print(hex(start),hex(end),'present:', amt, 'of', (end-start)//4096, desc)

    # Print the histogram
    print_hist(list(parse_pagemap(pid, ranges).values()))
