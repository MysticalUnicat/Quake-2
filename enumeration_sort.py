from collections import defaultdict
import math

def get(input, x, y):
    return input[y][x]

width = 5
height = 5

def find_index(input, x, y):
    ox = x
    oy = y
    count = (x + 1) * (y + 1) - 1
    key = get(input, x, y)
    # print(("key", key, "x", x, "y", y))
    while x < width - 1 and y > 0:
        # print(("Q2 count", count))
        test = get(input, x + 1, y - 1)
        # print(("Q2 test", test, "x", x + 1, "y", y - 1))
        if key < test:
            y -= 1
        elif key == test:
            count += y
            x += 1
        else:
            count += y
            x += 1
    x = ox
    y = oy
    while x > 0 and y < height - 1:
        # print(("Q4 count", count))
        test = get(input, x - 1, y + 1)
        # print(("Q4 test", test, "x", x - 1, "y", y + 1))
        if key < test:
            x -= 1
        elif key == test:
            x -= 1
        else:
            count += x
            y += 1
    return count

def do_test(input):
    result = []
    histogram = defaultdict(int)
    pairs = {}

    for y in range(len(input)):
        row = []
        for x in range(len(input[0])):
            index = find_index(input, x, y)
            histogram[index] += 1
            row.append(index)
            pairs[(x, y)] = (get(input, x, y), index)
        result.append(row)

    if len(input) * len(input[0]) != len(histogram):
        for n in range(len(input) * len(input[0])):
            histogram[n] += 0

        print({(k,v) for k,v in histogram.items() if v != 1})
        print(pairs)

row_order = [
    [0,1,2,3,4],
    [5,6,7,8,9],
    [10,11,12,13,14],
    [15,16,17,18,19],
    [20,21,22,23,24]
]

column_order = [
    [0,5,10,15,20],
    [1,6,11,16,21],
    [2,7,12,17,22],
    [3,8,13,18,23],
    [4,9,14,19,24]
]

unorder = [
    [0,1,10,15,20],
    [2,6,11,16,21],
    [3,7,12,13,22],
    [4,8,17,18,23],
    [5,9,14,19,24]
]

duplicates = [
    [1, 3, 4, 8,10],
    [2, 4, 7, 9,13],
    [2, 5, 9,10,14], # this 14 sees one other self in Q4, required one move, test 10 adds 4        0
    [4, 5, 9,10,14], # this 14 sees one other self in Q4                                           1
    [5, 8,10,14,14]  # first 14 sees one self in Q2, no other in Q4. second 14 sees no other self  2 3
]

# print(find_index(duplicates, 1, 1))

do_test(row_order)
do_test(column_order)
do_test(unorder)
do_test(duplicates)