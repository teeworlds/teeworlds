from __future__ import with_statement
import struct
import sys
import re

def convert(input, output):
    with open(input, "r") as in_file:
        with open(output, "w") as out_file:
            def build_dic(parts):
                dic = {}

                for part in parts:
                    key, value = part.split('=')

                    try:
                        dic[key] = int(value)
                    except:
                        dic[key] = value

                return dic

            def get_entry(line):
                while line[-1] == "\r" or line[-1] == "\n":
                    line = line[0:-1]
                parts = []

                quote = 0
                part = ""

                for c in line:
                    if c == "\"":
                        quote = 1-quote
                    elif c == " " and not quote:
                        if part:
                            parts.append(part)
                            part = ""
                    else:
                        part += c

                if part:
                    parts.append(part)

                type = parts[0]

                dic = build_dic(parts[1:])

                return type, dic

            def write_int16(val):
                out_file.write(struct.pack('<h', val))

            def write_info(dic):
                write_int16(dic["size"])

            def write_common(dic):
                write_int16(dic["scaleW"])
                write_int16(dic["scaleH"])
                write_int16(dic["lineHeight"])
                write_int16(dic["base"])

            def write_page(dic):
                pass

            def write_chars(dic):
                pass

            def write_char(dic):
                write_int16(dic["x"])
                write_int16(dic["y"])
                write_int16(dic["width"])
                write_int16(dic["height"])
                write_int16(dic["xoffset"])
                write_int16(dic["yoffset"])
                write_int16(dic["xadvance"])

            def write_default_char():
                write_int16(0)
                write_int16(0)
                write_int16(0)
                write_int16(0)
                write_int16(0)
                write_int16(0)
                write_int16(0)

            def write_kernings(dic):
                pass

            def write_kerning(dic):
                write_int16(dic["amount"])

            def write_default_kerning():
                write_int16(0)

            chars = []
            kernings = []
            for i in range(256):
                chars.append(None)
            for i in range(256*256):
                kernings.append(None)

            def save_char(dic):
                if dic["id"] < 256:
                    chars[dic["id"]] = dic

            def save_kerning(dic):
                kernings[dic["first"] + dic["second"]*256] = dic

            write_table = {
                "info": write_info,
                "common": write_common,
                "page": write_page,
                "chars": write_chars,
                "char": save_char,
                "kernings": write_kernings,
                "kerning": save_kerning
            }

            for line in in_file:
                type, dic = get_entry(line)
                
                write_table[type](dic)

            for i in range(256):
                if chars[i]:
                    write_char(chars[i])
                else:
                    write_default_char()

            for i in range(256*256):
                if kernings[i]:
                    write_kerning(kernings[i])
                else:
                    write_default_kerning()

if len(sys.argv) >= 2:
    print "converting..."

    filenames = sys.argv[1:]
    for filename in filenames:
        input = filename
        output = re.sub("fnt$", "tfnt", input)
            
        print "input: %s, output: %s" % (input, output)
        convert(input, output)
    print "done!"
else:
    print "font converter! converts .fnt files to teeworlds .tfnt"
    print "usage: font_converter <input>"
