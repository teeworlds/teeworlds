import struct, zlib, sys

class image:
	w = 0
	h = 0
	data = []

def read_tga(f):
	image = f.read()
	img_type = struct.unpack("<B", image[2:3])[0]
	img_bpp = struct.unpack("<B", image[16:17])[0]
	img_width = struct.unpack("<H", image[12:14])[0]
	img_height = struct.unpack("<H", image[14:16])[0]

	if img_type != 2 or img_bpp != 32:
		print "image must be a RGBA"

	start = 18+struct.unpack("B", image[0])[0]
	end = start + img_width*img_height*4
	image_data = image[start:end] # fetch raw data
	return image_data

def write_tga(f, w, h, bpp, data):
	f.write(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, bpp, 0) + data)





def load_png(f):
	def read(fmt): return struct.unpack("!"+fmt, f.read(struct.calcsize("!"+fmt)))
	def skip(count): f.read(count)

	# read signature
	if read("cccccccc") != ('\x89', 'P', 'N', 'G', '\r', '\n', '\x1a', '\n'):
		return 0

	# read chunks
	width = -1
	height = -1
	imagedata = ""
	while 1:
		size, id = read("I4s")
		if id == "IHDR": # read header
			width, height, bpp, colortype, compression, filter, interlace = read("IIBBBBB")
			if bpp != 8 or compression != 0 or filter != 0 or interlace != 0 or (colortype != 2 and colortype != 6):
				print "can't handle png of this type"
				print width, height, bpp, colortype, compression, filter, interlace
				return 0
			skip(4)
		elif id == "IDAT":
			imagedata += f.read(size)
			skip(4) # read data
		elif id == "IEND":
			break # we are done! \o/
		else:
			skip(size+4) # skip unknown chunks

	# decompress image data
	rawdata = map(ord, zlib.decompress(imagedata))
	
	# apply per scanline filters
	pitch = width*4+1
	bpp = 4
	imgdata = []
	prevline = [0 for x in xrange(0, (width+1)*bpp)]
	for y in xrange(0,height):
		filter = rawdata[pitch*y]
		pixeldata = rawdata[pitch*y+1:pitch*y+pitch]
		thisline = [0 for x in xrange(0,bpp)]
		def paeth(a, b, c):
			p = a + b - c
			pa = abs(p - a)
			pb = abs(p - b)
			pc = abs(p - c)
			if pa <= pb and pa <= pc:
				return a
			if pb <= pc:
				return b
			return c
		
		if filter == 0: f = lambda a,b,c: 0
		elif filter == 1: f = lambda a,b,c: a
		elif filter == 2: f = lambda a,b,c: b
		elif filter == 3: f = lambda a,b,c: (a+b)/2
		elif filter == 4: f = paeth
			
		for x in xrange(0, width*bpp):
			thisline += [(pixeldata[x] + f(thisline[x], prevline[x+bpp], prevline[x])) % 256]			
			
		prevline = thisline
		imgdata += thisline[4:]
	
	raw = ""
	for x in imgdata:
		raw += struct.pack("B", x)
		
	#print len(raw), width*height*4
	write_tga(file("test2.tga", "w"), width, height, 32, raw)
	return 0
	
load_png(file("butterfly2.png", "rb"))