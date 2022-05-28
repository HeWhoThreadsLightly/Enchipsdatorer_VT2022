#!/usr/bin/env python3

from PIL import Image
import numpy as np
import os
import sys, getopt
import math

helpMessage = '''
    immageConverter.py -f <filename> -h <sprite horizontal px> -v <sprite vertical px>
        -s save sprites
    example: immageConverter.py -f Codepage-437.bmp -h 9 -v 16
    immageConverter.py --help for this message
'''

def main(argv):
    file = ""
    hpx = -1
    vpx = -1
    charOnline = 40
    saveSprites = 0
    #read comand line arguments
    #-f Codepage-437.bmp -h 9 -v 16 -s
    try:
        opts, arg = getopt.getopt(argv, "sf:h:v:",["help"])
    except getopt.GetoptError:
        print("error reading arguments")
        print(helpMessage)
        sys.exit(2)
    for opt, arg in opts:
        if opt == "--help":
            print("usage:")
            print(helpMessage)
            exit()
        elif opt == '-f':
            file = arg
        elif opt == '-h':
            hpx = int(arg)
        elif opt == '-v':
            vpx = int(arg)
        elif opt == '-s':
            saveSprites = 1
    if file == "" or hpx == -1 or vpx == -1:
        print("error interpreting arguments")
        print(file, hpx, vpx)
        print(helpMessage)
        sys.exit()
    
    filename, file_extention = os.path.splitext(file)
    im = Image.open(file)
    #im.show()
    a = np.asarray(im)

    #print immage splitting information
    print(filename, "resolution", len(a[0]), len(a))
    sprites = len(a)/vpx * len(a[0])/hpx
    spritesLine = len(a[0])/hpx
    spriteLinesTotal = len(a)/vpx
    print(spritesLine, "sprites per line", spriteLinesTotal, "lines of sprites", sprites, "sprites total")

    if not (sprites.is_integer() and spritesLine.is_integer() and spriteLinesTotal.is_integer()):
        print("sprites properties have to be integers")
        sys.exit(2)
    sprites = int(sprites)
    spritesRow = int(spritesLine)

    #convert 2d immage of sprites to 1d immage of sprites
    arr = np.zeros(shape=(int(len(a) * len(a[0])/hpx), hpx), dtype=bool)
    arrsp = np.zeros(shape=(vpx, hpx), dtype=bool)
    print(len(arr), len(arr[0]))
    for s in range(sprites):
        for v in range(vpx):
            for h in range(hpx):
                arr[s*vpx+v][h] = a[int(math.floor(s/spritesLine))*vpx+v][(int(s%spritesLine))*hpx+h]
                arrsp[v][h] = a[int(math.floor(s/spritesLine))*vpx+v][(int(s%spritesLine))*hpx+h]
                #print("arr[", s*vpx+v, "][", h, "] = a[", int(math.floor(s/spritesLine))+v, "][", (int(s%spritesLine))*hpx+h, "]")
        if saveSprites:
            nsp = Image.fromarray(arrsp)
            nsp.save("sprites/" + filename + "_sprite_" + str(s) + file_extention);
        #if s != 0 and s%32 == 0:#debugging
        #    ni = Image.fromarray(arr)
        #    ni.show()
        #    ni.save(filename + "_columated" + file_extention);
        #    input("continue")

    #display columated image
    ni = Image.fromarray(arr)
    ni.show()
    ni.save(filename + "_columated" + file_extention);

    #write c file
    outc = open(filename + ".c", "w")
    cname = filename.replace('-', '_')

    outc.write("const uint32_t " + cname + "_char_width = " + str(hpx) + ";\n")
    outc.write("const uint32_t " + cname + "_char_hight = " + str(vpx) + ";\n")
    outc.write("const uint32_t " + cname + "_char_count = " + str(sprites) + ";\n\n")
    outc.write("const char " + cname + "[] = {\n")

    #convert bit array to a byte array
    bytearr = np.packbits(np.uint8(arr.flatten()))
    s = ""
    for b in bytearr[0:-1]:#print all bytes exept the last
        s += '{:#010b}, '.format(b)
        if len(s) > charOnline:
            outc.write("\t" + s + "\n")
            s = ""

    if len(bytearr) != 0:#print the last byte in the array with termination
        s += '{:#010b}'.format(bytearr[-1])
        outc.write("\t" + s + "};\n\n")
    outc.close()

if __name__ == "__main__":
   main(sys.argv[1:])
