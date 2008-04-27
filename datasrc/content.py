import copy

class SoundSet:
	def __init__(self, name, files):
		self.name = name
		self.files = files

class Image:
	def __init__(self, name, filename):
		self.name = name
		self.filename = filename
		
class Pickup:
	def __init__(self, name, respawntime=15, spawndelay=0):
		self.name = name
		self.respawntime = respawntime
		self.spawndelay = spawndelay

class Variable:
	def __init__(self, name, value):
		self.name = name
		self.value = value

class Int(Variable):
	def emit_declaration(self):
		return ["int %s;"%self.name]
	def emit_definition(self):
		return ["%d"%self.value]
		
class Float(Variable):
	def emit_declaration(self):
		return ["float %s;"%self.name]
	def emit_definition(self):
		return ["%ff"%self.value]
	
class String(Variable):
	def emit_declaration(self):
		return ["const char *%s;"%self.name]
	def emit_definition(self):
		return ['"%s"'%self.value]
		
class SpriteRef(Variable):
	def emit_declaration(self):
		return ["SPRITE *%s;"%self.name]
	def emit_definition(self):
		return ['&sprites[SPRITE_%s]'%self.value.upper()]

class SpriteSet:
	def __init__(self, image, grid, sprites):
		self.image = image
		self.grid = grid
		self.sprites = sprites
		
class Sprite:
	def __init__(self, name, pos):
		self.name = name
		self.pos = pos

# TODO: rename this
class FieldStorage:
	def __init__(self, name, base, fields):
		self.name = name
		self.fields = []
		if base:
			self.fields = copy.deepcopy(base.fields)
		self.base = base
		self.baselen = len(self.fields)
		
		self.autoupdate()
		
		self.update_all(fields)
	
	def update_all(self, fields):
		for v in fields:
			if not self.update(v):
				self.fields += [v]
	
	def update(self, value):
		for i in xrange(0, len(self.fields)):
			if self.fields[i].name == value.name:
				self.fields[i] = value
				return True
		return False
	
	def autoupdate(self):
		pass

def FileList(format, num):
	return [format%x for x in xrange(1,num)]
	

Sounds = [
	SoundSet("gun_fire", FileList("data/audio/wp_gun_fire-%02d.wv", 3)),
	SoundSet("shotgun_fire", FileList("data/audio/wp_shotty_fire-%02d.wv", 3)),
	SoundSet("grenade_fire", FileList("data/audio/wp_flump_launch-%02d.wv", 3)),
	SoundSet("hammer_fire", FileList("data/audio/wp_hammer_swing-%02d.wv", 3)),
	SoundSet("hammer_hit", FileList("data/audio/wp_hammer_hit-%02d.wv", 3)),
	SoundSet("ninja_fire", FileList("data/audio/wp_ninja_attack-%02d.wv", 3)),
	SoundSet("grenade_explode", FileList("data/audio/wp_flump_explo-%02d.wv", 3)),
	SoundSet("ninja_hit", FileList("data/audio/wp_ninja_hit-%02d.wv", 3)),
	SoundSet("rifle_fire", FileList("data/audio/wp_rifle_fire-%02d.wv", 3)),
	SoundSet("rifle_bounce", FileList("data/audio/wp_rifle_bnce-%02d.wv", 3)),
	SoundSet("weapon_switch", FileList("data/audio/wp_switch-%02d.wv", 3)),

	SoundSet("player_pain_short", FileList("data/audio/vo_teefault_pain_short-%02d.wv", 12)),
	SoundSet("player_pain_long", FileList("data/audio/vo_teefault_pain_long-%02d.wv", 2)),

	SoundSet("body_land", FileList("data/audio/foley_land-%02d.wv", 4)),
	SoundSet("player_airjump", FileList("data/audio/foley_dbljump-%02d.wv", 3)),
	SoundSet("player_jump", FileList("data/audio/foley_foot_left-%02d.wv", 4) +  FileList("data/audio/foley_foot_right-%02d.wv", 4)),
	SoundSet("player_die", FileList("data/audio/foley_body_splat-%02d.wv", 3)),
	SoundSet("player_spawn", FileList("data/audio/vo_teefault_spawn-%02d.wv", 7)),
	SoundSet("player_skid", FileList("data/audio/sfx_skid-%02d.wv", 4)),
	SoundSet("tee_cry", FileList("data/audio/vo_teefault_cry-%02d.wv", 2)),

	SoundSet("hook_loop", FileList("data/audio/hook_loop-%02d.wv", 2)),

	SoundSet("hook_attach_ground", FileList("data/audio/hook_attach-%02d.wv", 3)),
	SoundSet("hook_attach_player", FileList("data/audio/foley_body_impact-%02d.wv", 3)),
	SoundSet("pickup_health", FileList("data/audio/sfx_pickup_hrt-%02d.wv", 2)),
	SoundSet("pickup_armor", FileList("data/audio/sfx_pickup_arm-%02d.wv", 4)),

	SoundSet("pickup_grenade", FileList("data/audio/sfx_pickup_arm-%02d.wv", 1)),
	SoundSet("pickup_shotgun", FileList("data/audio/sfx_pickup_arm-%02d.wv", 1)),
	SoundSet("pickup_ninja", FileList("data/audio/sfx_pickup_arm-%02d.wv", 1)),
	SoundSet("weapon_spawn", FileList("data/audio/sfx_spawn_wpn-%02d.wv", 3)),
	SoundSet("weapon_noammo", FileList("data/audio/wp_noammo-%02d.wv", 5)),

	SoundSet("hit", FileList("data/audio/sfx_hit_weak-%02d.wv", 2)),

	SoundSet("chat_server", ["data/audio/sfx_msg-server.wv"]),
	SoundSet("chat_client", ["data/audio/sfx_msg-client.wv"]),
	SoundSet("ctf_drop", ["data/audio/sfx_ctf_drop.wv"]),
	SoundSet("ctf_return", ["data/audio/sfx_ctf_rtn.wv"]),
	SoundSet("ctf_grab_pl", ["data/audio/sfx_ctf_grab_pl.wv"]),
	SoundSet("ctf_grab_en", ["data/audio/sfx_ctf_grab_en.wv"]),
	SoundSet("ctf_capture", ["data/audio/sfx_ctf_cap_pl.wv"]),
]

Images = [
	Image("null", ""),
	Image("game", "data/game.png"),
	Image("particles", "data/particles.png"),
	Image("cursor", "data/gui_cursor.png"),
	Image("banner", "data/gui_logo.png"),
	Image("emoticons", "data/emoticons.png"),
	Image("browseicons", "data/browse_icons.png"),
	Image("console_bg", "data/console.png"),
	Image("console_bar", "data/console_bar.png"),
]

Pickups = [
	Pickup("health"),
	Pickup("armor"),
	Pickup("weapon"),
	Pickup("ninja", 90, 90),
]

Sprites = [
	SpriteSet("particles", (8,8), [
		Sprite("part_slice", (0,0,1,1)),
		Sprite("part_ball", (1,0,1,1)),
		Sprite("part_splat01", (2,0,1,1)),
		Sprite("part_splat02", (3,0,1,1)),
		Sprite("part_splat03", (4,0,1,1)),

		Sprite("part_smoke", (0,1,1,1)),
		Sprite("part_shell", (0,2,2,2)),
		Sprite("part_expl01", (0,4,4,4)),
		Sprite("part_airjump", (2,2,2,2)),
	]),

	SpriteSet("game", (8,8), [
		Sprite("health_full", (21,0,2,2)),
		Sprite("health_empty", (23,0,2,2)),
		Sprite("armor_full", (21,2,2,2)),
		Sprite("armor_empty", (23,2,2,2)),
		
		Sprite("star1", (15,0,2,2)),
		Sprite("star2", (17,0,2,2)),
		Sprite("star3", (19,0,2,2)),
			
		Sprite("part1", (6,0,1,1)),
		Sprite("part2", (6,1,1,1)),
		Sprite("part3", (7,0,1,1)),
		Sprite("part4", (7,1,1,1)),
		Sprite("part5", (8,0,1,1)),
		Sprite("part6", (8,1,1,1)),
		Sprite("part7", (9,0,2,2)),
		Sprite("part8", (11,0,2,2)),
		Sprite("part9", (13,0,2,2)),
	
		Sprite("weapon_gun_body", (2,4,4,2)),
		Sprite("weapon_gun_cursor", (0,4,2,2)),
		Sprite("weapon_gun_proj", (6,4,2,2)),
		Sprite("weapon_gun_muzzle1", (8,4,3,2)),
		Sprite("weapon_gun_muzzle2", (12,4,3,2)),
		Sprite("weapon_gun_muzzle3", (16,4,3,2)),
		
		Sprite("weapon_shotgun_body", (2,6,8,2)),
		Sprite("weapon_shotgun_cursor", (0,6,2,2)),
		Sprite("weapon_shotgun_proj", (10,6,2,2)),
		Sprite("weapon_shotgun_muzzle1", (12,6,3,2)),
		Sprite("weapon_shotgun_muzzle2", (16,6,3,2)),
		Sprite("weapon_shotgun_muzzle3", (20,6,3,2)),
		
		Sprite("weapon_grenade_body", (2,8,7,2)),
		Sprite("weapon_grenade_cursor", (0,8,2,2)),
		Sprite("weapon_grenade_proj", (10,8,2,2)),

		Sprite("weapon_hammer_body", (2,1,4,3)),
		Sprite("weapon_hammer_cursor", (0,0,2,2)),
		Sprite("weapon_hammer_proj", (0,0,0,0)),
		
		Sprite("weapon_ninja_body", (2,10,7,2)),
		Sprite("weapon_ninja_cursor", (0,10,2,2)),
		Sprite("weapon_ninja_proj", (0,0,0,0)),

		Sprite("weapon_rifle_body", (2,12,7,3)),
		Sprite("weapon_rifle_cursor", (0,12,2,2)),
		Sprite("weapon_rifle_proj", (10,12,2,2)),
		
		Sprite("hook_chain", (2,0,1,1)),
		Sprite("hook_head", (3,0,2,1)),
		
		Sprite("hadoken1", (25,0,7,4)),
		Sprite("hadoken2", (25,4,7,4)),
		Sprite("hadoken3", (25,8,7,4)),
		
		Sprite("pickup_health", (10,2,2,2)),
		Sprite("pickup_armor", (12,2,2,2)),
		Sprite("pickup_weapon", (3,0,6,2)),
		Sprite("pickup_ninja", (3,10,7,2)),

		Sprite("flag_blue", (12,8,4,8)),
		Sprite("flag_red", (16,8,4,8)),
	]),
	
	SpriteSet(None, (8,4), [
		Sprite("tee_body", (0,0,3,3)),
		Sprite("tee_body_outline", (3,0,3,3)),
		Sprite("tee_foot", (6,1,2,1)),
		Sprite("tee_foot_outline", (6,2,2,1)),
		Sprite("tee_hand", (6,0,1,1)),
		Sprite("tee_hand_outline", (7,0,1,1)),
		
		Sprite("tee_eye_normal", (2,3,1,1)),
		Sprite("tee_eye_angry", (3,3,1,1)),
		Sprite("tee_eye_pain", (4,3,1,1)),
		Sprite("tee_eye_happy", (5,3,1,1)),
		Sprite("tee_eye_dead", (6,3,1,1)),
		Sprite("tee_eye_surprise", (7,3,1,1)),
	]),

	SpriteSet("browseicons", (4,1), [
		Sprite("browse_lock", (0,0,1,1)),
		Sprite("browse_progress1", (1,0,1,1)),
		Sprite("browse_progress2", (2,0,1,1)),
		Sprite("browse_progress3", (3,0,1,1)),
	]),
]

class Weapon(FieldStorage):
	def autoupdate(self):
		self.update(String("name", self.name))
		self.update(SpriteRef("sprite_body", "weapon_%s_body"%self.name))
		self.update(SpriteRef("sprite_cursor", "weapon_%s_cursor"%self.name))
		self.update(SpriteRef("sprite_proj", "weapon_%s_proj"%self.name))

WeaponBase = Weapon("weapon", None, [
	String("name", "base"),
	SpriteRef("sprite_body", None),
	SpriteRef("sprite_cursor", None),
	SpriteRef("sprite_proj", None),
	Int("damage", 1),
	Int("firedelay", 500),
	Int("visual_size", 96),
])

Weapons = [
	Weapon("hammer", WeaponBase, [
		Int("firedelay", 100),
		Int("damage", 3),
	]),

	Weapon("gun", WeaponBase, [
		Int("firedelay", 100),
		Int("damage", 1),

		Float("curvature", 1.25),
		Float("speed", 2200.0),
		Float("lifetime", 2.0),
	]),

	Weapon("shotgun", WeaponBase, [
		Int("firedelay", 500),
		Int("damage", 1),

		Float("curvature", 1.25),
		Float("speed", 2200.0),
		Float("speeddiff", 0.8),
		Float("lifetime", 0.25),
	]),

	Weapon("grenade", WeaponBase, [
		Int("firedelay", 100),
		Int("damage", 6),

		Float("curvature", 7.0),
		Float("speed", 1000.0),
		Float("lifetime", 2.0),
	]),

	Weapon("rifle", WeaponBase, [
		Int("firedelay", 800),
		Int("visual_size", 92),
		Int("damage", 5),
		
		Float("reach", 800.0),
		Float("bounce_delay", 150),
		Float("bounce_num", 1),
		Float("bounce_cost", 0),
	]),
	
	Weapon("ninja", WeaponBase, [
		Int("firedelay", 800),
		Int("damage", 9),
	]),	
]

ticks_per_second = 50.0
Physics = FieldStorage("physics", None, [
	Float("ground_control_speed", 10.0),
	Float("ground_control_accel", 100.0 / ticks_per_second),
	Float("ground_friction", 0.5),
	Float("ground_jump_impulse", 12.6),
	Float("air_jump_impulse", 11.5),
	Float("air_control_speed", 250.0 / ticks_per_second),
	Float("air_control_accel", 1.5),
	Float("air_friction", 0.95),
	Float("hook_length", 380.0),
	Float("hook_fire_speed", 80.0),
	Float("hook_drag_accel", 3.0),
	Float("hook_drag_speed", 15.0),
	Float("gravity", 0.5),

	Float("velramp_start", 550),
	Float("velramp_range", 2000),
	Float("velramp_curvature", 1.4),
])
