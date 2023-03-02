import random
import math

def grade(input):
    print(input)

    size = int(math.ceil(math.sqrt(len(input))))
    print(size)

    output = [0 for _ in range(size * size)]

    for row_index in range(size):
        key_base = row_index * size
        key_stride = 1
        val_base = row_index
        val_stride = size

        


    return output

input = [random.randint(0, 15) for _ in range(16)]
grade(input)