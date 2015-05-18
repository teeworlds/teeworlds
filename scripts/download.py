import urllib.request
import os
import zipfile
import sys

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

def downloadAll(arch, conf):
    downloadUrl = "https://github.com/Zwelf/tw-downloads/raw/master/{}.zip"
    builddir = "../build/" + arch + "/" + conf + "/"
    files = (downloadUrl.format("SDL2.dll"     + "-" + arch), builddir            ),\
            (downloadUrl.format("freetype.dll" + "-" + arch), builddir            ),\
            (downloadUrl.format("sdl"                      ), "../other/sdl/"     ),\
            (downloadUrl.format("freetype"                 ), "../other/freetype/")
    for elem in files:
        _downloadZip(elem[0], elem[1])

def main():
    arch = sys.argv[1] if len(sys.argv) >= 2 else "x86"
    conf = sys.argv[2] if len(sys.argv) >= 3 else "debug"
    downloadAll(arch, conf)

if __name__ == '__main__':
    main()