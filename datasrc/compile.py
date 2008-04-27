import os, imp, sys
import datatypes
import content
import network

	
def create_enum_table(names, num):
	lines = []
	lines += ["enum", "{"]
	lines += ["\t%s=0,"%names[0]]
	for name in names[1:]:
		lines += ["\t%s,"%name]
	lines += ["\t%s" % num, "};"]
	return lines
	
gen_content_header = False
gen_content_source = True


# collect sprites
sprites = []
for set in content.Sprites:
	sprites += set.sprites


if gen_content_header:
	
	print """
struct SOUND
{
	int id;
	const char *filename;
};

struct SOUNDSET
{
	int num_sounds;
	SOUND *sound;
};

struct IMAGE
{
	int id;
	const char *filename;
};

struct SPRITESET
{
	IMAGE *image;
	int gridx;
	int gridy;
};

struct SPRITE
{
	SPRITESET *set;
	int x, y, w, h;
};

"""

	def generate_struct(this, name, parent_name):
		print "struct %s" % name

		print "{"
		if parent_name:
			print "\t%s base;" % parent_name
		for var in this.fields[this.baselen:]:
			for l in var.emit_declaration(): print "\t"+l
		print "};"

	generate_struct(content.WeaponBase, "WEAPONSPEC", None)
	for weapon in content.Weapons:
		generate_struct(weapon, "WEAPONSPEC_%s"%weapon.name.upper(), "WEAPONSPEC")

	# generate enums
	for l in create_enum_table(["SOUND_"+o.name.upper() for o in content.Sounds], "NUM_SOUNDS"): print l
	for l in create_enum_table(["IMAGE_"+o.name.upper() for o in content.Images], "NUM_IMAGES"): print l
	for l in create_enum_table(["SPRITE_"+o.name.upper() for o in sprites], "NUM_SPRITES"): print l

	for l in create_enum_table(["WEAPONTYPE_"+o.name.upper() for o in content.Weapons], "NUM_WEAPONTYPES"): print l

if gen_content_source:
	# generate data
	for s in content.Sounds:
		print "static SOUND sounds_%s[%d] = {" % (s.name, len(s.files))
		for filename in s.files:
			print '\t{%d, "%s"},' % (-1, filename)
		print "};"
		
	print "static SOUNDSET soundsets[%d] = {" % len(content.Sounds)
	for s in content.Sounds:
		print "\t{%d, sounds_%s}," % (len(s.files), s.name)
		#for filename in s.files:
		#	print "\t{%d, '%s'}," % (-1, filename)
	print "};"

	print "static IMAGE images[%d] = {" % len(content.Images)
	for i in content.Images:
		print '\t{%d, "%s"},' % (-1, i.filename)
	print "};"

	print "static SPRITESET spritesets[%d] = {" % len(content.Sprites)
	for set in content.Sprites:
		if set.image:
			print '\t{&images[IMAGE_%s], %d, %d},' % (set.image.upper(), set.grid[0], set.grid[1])
		else:
			print '\t{0, %d, %d},' % (set.grid[0], set.grid[1])
	print "};"

	print "static SPRITE sprites[%d] = {" % len(sprites)
	spritesetid = 0
	for set in content.Sprites:
		for sprite in set.sprites:
			print '\t{&spritesets[%d], %d, %d, %d, %d},' % (spritesetid, sprite.pos[0], sprite.pos[1], sprite.pos[2], sprite.pos[3])
		spritesetid += 1
	print "};"

	for weapon in content.Weapons:
		print "static WEAPONSPEC_%s weapon_%s = {" % (weapon.name.upper(), weapon.name)
		for var in weapon.fields:
			for l in var.emit_definition(): print "\t"+l,
			print ","
		print "};"

	print "struct WEAPONS"
	print "{"
	print "\tWEAPONSPEC *id[%d];" % len(content.Weapons)
	for w in content.Weapons:
		print "\tWEAPONSPEC_%s &weapon_%s;" % (w.name.upper(), w.name)
	print ""
	print "};"

	print "static WEAPONS weapons = {{%s}," % (",".join(["&weapon_%s.base"%w.name for w in content.Weapons]))
	for w in content.Weapons:
		print "\tweapon_%s," % w.name
	print "};"


	
	print """
struct DATACONTAINER
{
	int num_sounds;
	SOUNDSET *sounds;

	int num_images;
	IMAGE *images;
	
	int num_sprites;
	SPRITE *sprites;

	WEAPONS &weapons;
};"""

	print "DATACONTAINER data = {"
	print "\t%d, soundsets," % len(content.Sounds)
	print "\t%d, images," % len(content.Images)
	print "\t%d, sprites," % len(content.Sprites)
	print "\tweapons,"
	print "};"
	
	
# NETWORK
if 0:


	for e in network.Enums:
		for l in create_enum_table(["%s_%s"%(e.name, v) for v in e.values], "NUM_%sS"%e.name): print l
		print ""
		
	for l in create_enum_table([o.enum_name for o in network.Objects], "NUM_NETOBJTYPES"): print l
	print ""
	for l in create_enum_table([o.enum_name for o in network.Messages], "NUM_NETMSGTYPES"): print l
	print ""
		
	for item in network.Objects + network.Messages:
		for line in item.emit_declaration():
			print line
		print ""


if 0:		
	# create names
	lines = []
	lines += ["static const char *netobj_names[] = {"]
	lines += ['\t"%s",' % o.name for o in network.Objects]
	lines += ['\t""', "};", ""]
	
	for l in lines:
		print l

	for item in network.Objects:
		for line in item.emit_validate():
			print line
		print ""

	# create validate tables
	lines = []
	lines += ["typedef int(*VALIDATEFUNC)(void *data, int size);"]
	lines += ["static VALIDATEFUNC validate_funcs[] = {"]
	lines += ['\tvalidate_%s,' % o.name for o in network.Objects]
	lines += ["\t0x0", "};", ""]

	lines += ["int netobj_validate(int type, void *data, int size)"]
	lines += ["{"]
	lines += ["\tif(type < 0 || type >= NUM_NETOBJTYPES) return -1;"]
	lines += ["\treturn validate_funcs[type](data, size);"]
	lines += ["};", ""]
	
	for l in lines:
		print l
	
