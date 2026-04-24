"""
     Author: Joachim Tsakanikas
    Created: 2025/10/30

Last editor: Joachim Tsakanikas
Last edited: 2025/11/03

"""

import csv

# import math
# import os

filepath: str = "/home/joachim/Downloads/20251029_James_Shirk/epelastic.txt"


def r5weights(filepath: str) -> None:
    """
    Print thin quartz weights with tile names for QwCombiner

    Parameters
    ----------
    filepath: str
            Path to file including file name and extension,
            e.g., "path/to/file.csv".

    Notes
    -----
    Each detector element is specified in the csv file by a six digit code. The
    first two digits are always either 15 or 11 and correspond to front flush (0xFF)
    and back flush (0xBF). Importantly, all the transition segments are back
    flush. The closed and open segments are front flush. Together, the first two
    pairs of numbers specify which segment the detector element is found in. The
    last pair of numbers specifies where the element is within the segment.

    In order to understand what the second pair of numbers signify, it is useful
    to know that the detector is separated azimuthally into 28 segments and radi-
    ally into 6 rings. The first segment is closed, the second is transition, the
    third is open, the fourth is transition, and the pattern repeats. In total,
    there are fourteen transition segments, and seven closed and open segments.
    The transition segments are numbered 1 through 14. Thus, the fourth trans-
    ition segment is specified by the code 1104__, where we have left off the
    last two digits. The closed segments are numbered 1 through 14 odd, and the
    open segments are numbered 1 through 14 even. Thus, the fourth closed and
    open segments are 1507__ and 1508__, respectively.

    The last pair of digits is straightforward to understand. The second-to-last
    digit is the ring number. The last digit is a modifier used to specify the
    element's position within the ring if there is more than one ring element.
    If there is only one element in the ring, the last digit is 0. Ring 5 is the
    only ring with multiple elements per segment. Ring 5 has three elements, and
    they are labelled "left", "center," and "right" from left to right in the
    same motion as the segments are numbered (counter-clockwise). "Left",
    "center", and "right" are numbered 1, 2, and 3, respectively.

    The detector elements are specified more simply in the mapfile by the prefix
    "TQ", standing for "thin quartz", followed by the segment number (two digits),
    an underscore, the letter 'r' and ring number, and sometimes a letter {'l',
    'c', or 'r'} for elements in ring 5.

    Examples:           csv code | map code
                    ---------|---------
                        150110 | tq01_r1    <-- 1st closed seg, ring 1
                        110851 | tq16_r5l   <-- 8th trans seg, ring 5, left
                        151220 | tq27_r2    <-- 6th open seg, ring 2

    """
    with open(filepath, "r", encoding="utf8") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=",", dialect="unix")
        print("\nCosine weights:")
        sum_cos: float = 0
        for row in reader:
            tile: str = row["detno"]
            flush: str = tile[:2]
            num = int(tile[2:4])
            segment: int
            if flush == "11":  # back flush <==> transition segment
                segment = 2 * num
            else:  # open or closed segment
                segment = 2 * num - 1
            ring: str = tile[4]
            pos: str = tile[5]
            weight = float(row[" mean_of_cos"])
            detector = "tq" + f"{segment:02}" + "_r" + ring
            match pos:
                case "1":
                    detector += "l"
                case "2":
                    detector += "c"
                case "3":
                    detector += "r"
                case _:
                    pass
            try:
                if ring == "5":
                    print(f"asym:{detector}, {weight:.4f}")
                    sum_cos += weight
            except KeyError:
                print("There was an error constructing the detector name.")
                return
    with open(filepath, "r", encoding="utf8") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=",", dialect="unix")  # reset reader
        print("\n\nSine weights:")
        sum_sin: float = 0
        for row in reader:
            tile = row["detno"]
            flush = tile[:2]
            num = int(tile[2:4])
            if flush == "11":  # back flush <==> transition segment
                segment = 2 * num
            else:  # open or closed segment
                segment = 2 * num - 1
            ring = tile[4]
            pos = tile[5]
            weight = float(row[" mean_of_sin"])
            detector = "tq" + f"{segment:02}" + "_r" + ring
            match pos:
                case "1":
                    detector += "l"
                case "2":
                    detector += "c"
                case "3":
                    detector += "r"
                case _:
                    pass
            try:
                if ring == "5":
                    print(f"asym:{detector}, {weight:.4f}")
                    sum_sin += weight
            except KeyError:
                print("There was an error constructing the detector name.")
                return
    print("\n")
    print(f"Sum of cosine weights: {sum_cos}")
    print(f"  Sum of sine weights: {sum_sin}")
    print("\n")


def r2weights(filepath: str) -> None:
    with open(filepath, "r", encoding="utf8") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=",", dialect="unix")
        print("\nCosine weights:")
        sum_cos: float = 0
        for row in reader:
            tile: str = row["detno"]
            flush: str = tile[:2]
            num = int(tile[2:4])
            segment: int
            if flush == "11":  # back flush <==> transition segment
                segment = 2 * num
            else:  # open or closed segment
                segment = 2 * num - 1
            ring: str = tile[4]
            pos: str = tile[5]
            weight = float(row[" mean_of_cos"])
            detector = "tq" + f"{segment:02}" + "_r" + ring
            match pos:
                case "1":
                    detector += "l"
                case "2":
                    detector += "c"
                case "3":
                    detector += "r"
                case _:
                    pass
            try:
                if ring == "2":
                    print(f"asym:{detector}, {weight:.4f}")
                    sum_cos += weight
            except KeyError:
                print("There was an error constructing the detector name.")
                return
    with open(filepath, "r", encoding="utf8") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=",", dialect="unix")  # reset reader
        print("\n\nSine weights:")
        sum_sin: float = 0
        for row in reader:
            tile = row["detno"]
            flush = tile[:2]
            num = int(tile[2:4])
            if flush == "11":  # back flush <==> transition segment
                segment = 2 * num
            else:  # open or closed segment
                segment = 2 * num - 1
            ring = tile[4]
            pos = tile[5]
            weight = float(row[" mean_of_sin"])
            detector = "tq" + f"{segment:02}" + "_r" + ring
            match pos:
                case "1":
                    detector += "l"
                case "2":
                    detector += "c"
                case "3":
                    detector += "r"
                case _:
                    pass
            try:
                if ring == "2":
                    print(f"asym:{detector}, {weight:.4f}")
                    sum_sin += weight
            except KeyError:
                print("There was an error constructing the detector name.")
                return
    print("\n")
    print(f"Sum of cosine weights: {sum_cos}")
    print(f"  Sum of sine weights: {sum_sin}")
    print("\n")


def r5otcweights(filepath: str, otc: str) -> None:
    with open(filepath, "r", encoding="utf8") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=",", dialect="unix")
        print("\nCosine weights:")
        sum_cos: float = 0
        for row in reader:
            tile: str = row["detno"]
            flush: str = tile[:2]
            num = int(tile[2:4])
            segment: int
            flavor: str
            if flush == "11":  # back flush <==> transition segment
                segment = 2 * num
                flavor = "t"
            else:  # open or closed segment
                segment = 2 * num - 1
                if num % 2 != 0:
                    flavor = "c"
                else:
                    flavor = "o"
            ring: str = tile[4]
            pos: str = tile[5]
            weight = float(row[" mean_of_cos"])
            detector = "tq" + f"{segment:02}" + "_r" + ring
            match pos:
                case "1":
                    detector += "l"
                case "2":
                    detector += "c"
                case "3":
                    detector += "r"
                case _:
                    pass
            try:
                if ring == "5" and flavor == otc:
                    print(f"asym:{detector}, {weight:.4f}")
                    sum_cos += weight
            except KeyError:
                print("There was an error constructing the detector name.")
                return
    with open(filepath, "r", encoding="utf8") as csvfile:
        reader = csv.DictReader(csvfile, delimiter=",", dialect="unix")  # reset reader
        print("\n\nSine weights:")
        sum_sin: float = 0
        for row in reader:
            tile = row["detno"]
            flush = tile[:2]
            num = int(tile[2:4])
            if flush == "11":  # back flush <==> transition segment
                segment = 2 * num
                flavor = "t"
            else:  # open or closed segment
                segment = 2 * num - 1
                if num % 2 != 0:
                    flavor = "c"
                else:
                    flavor = "o"
            ring = tile[4]
            pos = tile[5]
            weight = float(row[" mean_of_sin"])
            detector = "tq" + f"{segment:02}" + "_r" + ring
            match pos:
                case "1":
                    detector += "l"
                case "2":
                    detector += "c"
                case "3":
                    detector += "r"
                case _:
                    pass
            try:
                if ring == "5" and flavor == otc:
                    print(f"asym:{detector}, {weight:.4f}")
                    sum_sin += weight
            except KeyError:
                print("There was an error constructing the detector name.")
                return
    print("\n")
    print(f"Sum of cosine weights: {sum_cos}")
    print(f"  Sum of sine weights: {sum_sin}")
    print("\n")
