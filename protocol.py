import argparse
from logging import log
import logging
import os
import time
import serial
from tqdm import tqdm

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--img', help='kernel image file', required=True)
parser.add_argument('-d', '--dev', help='device fd', required=True)

args = parser.parse_args()

def main():
    if not os.path.exists(args.dev):
        logging.error('tty does not exist!')
        return
    if not os.path.exists(args.img):
        logging.error('kernel image does not exist!')
        return
    # tty = open(args.dev, 'rb+', buffering=0)
    tty = serial.Serial(args.dev, 115200)
    # protocol start bits(header)
    print("send protocol header")
    header = b'c8763'
    for i in header:
        byte = chr(i).encode()
        tty.write(byte)
        check = tty.read(1)
        if check != byte:
            print('error')
            return
    print("send protocol header finish")
    time.sleep(0.002)

    # get file size(bytes) and send
    img_size = os.path.getsize(args.img)
    size_data = img_size.to_bytes(4, 'big')
    print("kernel image size:"+str(img_size))
    print("send kernel images size")
    tty.write(size_data)
    # print(tty.readline())
    # for i in range(4):
    #     byte = chr(size_data[i]).encode()
    #     tty.write(byte)
    #     check = tty.read(1)
    #     if check != byte:
    #         print('error')
    #         return
    print("send kernel images size finish")
    time.sleep(0.002)

    # read image file
    img = open(args.img, 'rb')
    print("send kernel images")
    for i in tqdm(range(img_size)):
        byte = img.read(1)
        tty.write(byte)
        check = tty.read(1)
        if check != byte:
            print('error')
            return

    print("send kernel images finish")
    img.close()
    tty.close()


if __name__ == '__main__':
    main()
