import urllib.request
import os
import zipfile
import sys
import re

def _downloadZip(url, filePath):
    # create full path, if it doesn't exists
    os.makedirs(filePath, exist_ok=True)
    # create zip-file at the temp fileposition
    temp_filePath = filePath + "temp.zip"
    urllib.request.urlretrieve(url, temp_filePath)
    # exctract full zip-file
    with zipfile.ZipFile(temp_filePath) as myzip:
        myzip.extractall(filePath)
    os.remove(temp_filePath)

def downloadAll(arch, conf, targets):
    download_url = "https://github.com/teeworlds/teeworlds-libs/archive/master.zip".format
    builddir = "build/" + arch + "/" + conf + "/"
    files = {
        "SDL2.dll":     ("SDL2.dll"     + "-" + arch, builddir),
        "freetype.dll": ("freetype.dll" + "-" + arch, builddir),
        "sdl":          ("sdl",                       "other/sdl/"),
        "freetype":     ("freetype",                  "other/freetype/"),
    }

    for target in targets:
        download_file, target_dir = files[target]
        _downloadZip(download_url(download_file), target_dir)

def main():
    import argparse
    p = argparse.ArgumentParser(description="Download freetype and SDL library and header files for Windows.")
    p.add_argument("--arch", default="x86", choices=["x86", "x86_64"], help="Architecture for the downloaded libraries (Default: x86)")
    p.add_argument("--conf", default="debug", choices=["debug", "release"], help="Build type (Default: debug)")
    p.add_argument("targets", metavar="TARGET", nargs='+', choices=["SDL2.dll", "freetype.dll", "sdl", "freetype"], help='Target to download. Valid choices are "SDL.dll", "freetype.dll", "sdl" and "freetype"')
    args = p.parse_args()

    downloadAll(args.arch, args.conf, args.targets)

if __name__ == '__main__':
    main()
