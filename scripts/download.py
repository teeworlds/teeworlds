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

def downloadAll(arch, conf):
    downloadUrl = "https://github.com/Zwelf/tw-downloads/raw/master/{}.zip"
    builddir = "../build/" + arch + "/" + conf + "/"
    files = (downloadUrl.format("SDL2.dll"     + "-" + arch), builddir            ),\
            (downloadUrl.format("freetype.dll" + "-" + arch), builddir            ),\
            (downloadUrl.format("sdl"                      ), "other/sdl/"     ),\
            (downloadUrl.format("freetype"                 ), "other/freetype/")
    for elem in files:
        for i in range(1,len(sys.argv)):
            sTmp = re.search('master/(.+?).zip', elem[0])
            curr_pack = sTmp.group(1)
            if sys.argv[i] == "+pack" and sys.argv[i + 1] == curr_pack:
                _downloadZip(elem[0], elem[1])

def main():
    arch = "x86"
    conf = "debug"
    for i in range(1,len(sys.argv)): # this is expandable infinitly for more args
        if sys.argv[i] == "-arch": arch = sys.argv[i + 1] 
        if sys.argv[i] == "-conf": conf = sys.argv[i + 1]

    downloadAll(arch, conf)

if __name__ == '__main__':
    main()
