from datatypes import *

class Sound(Struct):
	def __init__(self, filename=""):
		Struct.__init__(self, "CDataSound")
		self.id = SampleHandle()
		self.filename = String(filename)

class SoundSet(Struct):
	def __init__(self, name="", files=[]):
		Struct.__init__(self, "CDataSoundset")
		self.name = String(name)
		self.sounds = Array(Sound())
		self.last = Int(-1)
		for name in files:
			self.sounds.Add(Sound(name))

class Image(Struct):
	def __init__(self, name="", filename="", linear_mapping=0):
		Struct.__init__(self, "CDataImage")
		self.name = String(name)
		self.filename = String(filename)
		self.flag = Int(linear_mapping)
		self.id = TextureHandle()

class SpriteSet(Struct):
	def __init__(self, name="", image=None, gridx=0, gridy=0):
		Struct.__init__(self, "CDataSpriteset")
		self.image = Pointer(Image, image) # TODO
		self.gridx = Int(gridx)
		self.gridy = Int(gridy)

class Sprite(Struct):
	def __init__(self, name="", Set=None, x=0, y=0, w=0, h=0):
		Struct.__init__(self, "CDataSprite")
		self.name = String(name)
		self.set = Pointer(SpriteSet, Set) # TODO
		self.x = Int(x)
		self.y = Int(y)
		self.w = Int(w)
		self.h = Int(h)

class Pickup(Struct):
	def __init__(self, name="", respawntime=15, spawndelay=0):
		Struct.__init__(self, "CDataPickupspec")
		self.name = String(name)
		self.respawntime = Int(respawntime)
		self.spawndelay = Int(spawndelay)

class AnimKeyframe(Struct):
	def __init__(self, time=0, x=0, y=0, angle=0):
		Struct.__init__(self, "CAnimKeyframe")
		self.time = Float(time)
		self.x = Float(x)
		self.y = Float(y)
		self.angle = Float(angle)

class AnimSequence(Struct):
	def __init__(self):
		Struct.__init__(self, "CAnimSequence")
		self.frames = Array(AnimKeyframe())

class Animation(Struct):
	def __init__(self, name=""):
		Struct.__init__(self, "CAnimation")
		self.name = String(name)
		self.body = AnimSequence()
		self.back_foot = AnimSequence()
		self.front_foot = AnimSequence()
		self.attach = AnimSequence()

class WeaponSpec(Struct):
	def __init__(self, container=None, name=""):
		Struct.__init__(self, "CDataWeaponspec")
		self.name = String(name)
		self.sprite_body = Pointer(Sprite, Sprite())
		self.sprite_cursor = Pointer(Sprite, Sprite())
		self.sprite_proj = Pointer(Sprite, Sprite())
		self.sprite_muzzles = Array(Pointer(Sprite, Sprite()))
		self.visual_size = Int(96)

		self.firedelay = Int(500)
		self.maxammo = Int(10)
		self.ammoregentime = Int(0)
		self.damage = Int(1)

		self.offsetx = Float(0)
		self.offsety = Float(0)
		self.muzzleoffsetx = Float(0)
		self.muzzleoffsety = Float(0)
		self.muzzleduration = Float(5)

		# dig out sprites if we have a container
		if container:
			for sprite in container.sprites.items:
				if sprite.name.value == "weapon_"+name+"_body": self.sprite_body.Set(sprite)
				elif sprite.name.value == "weapon_"+name+"_cursor": self.sprite_cursor.Set(sprite)
				elif sprite.name.value == "weapon_"+name+"_proj": self.sprite_proj.Set(sprite)
				elif "weapon_"+name+"_muzzle" in sprite.name.value:
					self.sprite_muzzles.Add(Pointer(Sprite, sprite))

class Weapon_Hammer(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecHammer")
		self.base = Pointer(WeaponSpec, WeaponSpec())

class Weapon_Gun(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecGun")
		self.base = Pointer(WeaponSpec, WeaponSpec())
		self.curvature = Float(1.25)
		self.speed = Float(2200)
		self.lifetime = Float(2.0)

class Weapon_Shotgun(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecShotgun")
		self.base = Pointer(WeaponSpec, WeaponSpec())
		self.curvature = Float(1.25)
		self.speed = Float(2200)
		self.speeddiff = Float(0.8)
		self.lifetime = Float(0.25)

class Weapon_Grenade(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecGrenade")
		self.base = Pointer(WeaponSpec, WeaponSpec())
		self.curvature = Float(7.0)
		self.speed = Float(1000)
		self.lifetime = Float(2.0)

class Weapon_Laser(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecLaser")
		self.base = Pointer(WeaponSpec, WeaponSpec())
		self.reach = Float(800.0)
		self.bounce_delay = Int(150)
		self.bounce_num = Int(1)
		self.bounce_cost = Float(0)

class Weapon_Ninja(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecNinja")
		self.base = Pointer(WeaponSpec, WeaponSpec())
		self.duration = Int(15000)
		self.movetime = Int(200)
		self.velocity = Int(50)

class Weapons(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataWeaponspecs")
		self.hammer = Weapon_Hammer()
		self.gun = Weapon_Gun()
		self.shotgun = Weapon_Shotgun()
		self.grenade = Weapon_Grenade()
		self.laser = Weapon_Laser()
		self.ninja = Weapon_Ninja()
		self.id = Array(WeaponSpec())

class Explosion(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataExplosion")
		self.radius = Float(135)
		self.max_force = Float(12)

class DataContainer(Struct):
	def __init__(self):
		Struct.__init__(self, "CDataContainer")
		self.sounds = Array(SoundSet())
		self.images = Array(Image())
		self.pickups = Array(Pickup())
		self.spritesets = Array(SpriteSet())
		self.sprites = Array(Sprite())
		self.animations = Array(Animation())
		self.weapons = Weapons()
		self.explosion = Explosion()

def FileList(format, num):
	return [format%(x+1) for x in range(0,num)]

container = DataContainer()
container.sounds.Add(SoundSet("gun_fire", FileList("audio/wp_gun_fire-%02d.wv", 3)))
container.sounds.Add(SoundSet("shotgun_fire", FileList("audio/wp_shotty_fire-%02d.wv", 3)))

container.sounds.Add(SoundSet("grenade_fire", FileList("audio/wp_flump_launch-%02d.wv", 3)))
container.sounds.Add(SoundSet("hammer_fire", FileList("audio/wp_hammer_swing-%02d.wv", 3)))
container.sounds.Add(SoundSet("hammer_hit", FileList("audio/wp_hammer_hit-%02d.wv", 3)))
container.sounds.Add(SoundSet("ninja_fire", FileList("audio/wp_ninja_attack-%02d.wv", 3)))
container.sounds.Add(SoundSet("grenade_explode", FileList("audio/wp_flump_explo-%02d.wv", 3)))
container.sounds.Add(SoundSet("ninja_hit", FileList("audio/wp_ninja_hit-%02d.wv", 3)))
container.sounds.Add(SoundSet("laser_fire", FileList("audio/wp_laser_fire-%02d.wv", 3)))
container.sounds.Add(SoundSet("laser_bounce", FileList("audio/wp_laser_bnce-%02d.wv", 3)))
container.sounds.Add(SoundSet("weapon_switch", FileList("audio/wp_switch-%02d.wv", 3)))

container.sounds.Add(SoundSet("player_pain_short", FileList("audio/vo_teefault_pain_short-%02d.wv", 12)))
container.sounds.Add(SoundSet("player_pain_long", FileList("audio/vo_teefault_pain_long-%02d.wv", 2)))

container.sounds.Add(SoundSet("body_land", FileList("audio/foley_land-%02d.wv", 4)))
container.sounds.Add(SoundSet("player_airjump", FileList("audio/foley_dbljump-%02d.wv", 3)))
container.sounds.Add(SoundSet("player_jump", FileList("audio/foley_foot_left-%02d.wv", 4) + FileList("audio/foley_foot_right-%02d.wv", 4)))
container.sounds.Add(SoundSet("player_die", FileList("audio/foley_body_splat-%02d.wv", 3)))
container.sounds.Add(SoundSet("player_spawn", FileList("audio/vo_teefault_spawn-%02d.wv", 7)))
container.sounds.Add(SoundSet("player_skid", FileList("audio/sfx_skid-%02d.wv", 4)))
container.sounds.Add(SoundSet("tee_cry", FileList("audio/vo_teefault_cry-%02d.wv", 2)))

container.sounds.Add(SoundSet("hook_loop", FileList("audio/hook_loop-%02d.wv", 2)))

container.sounds.Add(SoundSet("hook_attach_ground", FileList("audio/hook_attach-%02d.wv", 3)))
container.sounds.Add(SoundSet("hook_attach_player", FileList("audio/foley_body_impact-%02d.wv", 3)))
container.sounds.Add(SoundSet("hook_noattach", FileList("audio/hook_noattach-%02d.wv", 2)))
container.sounds.Add(SoundSet("pickup_health", FileList("audio/sfx_pickup_hrt-%02d.wv", 2)))
container.sounds.Add(SoundSet("pickup_armor", FileList("audio/sfx_pickup_arm-%02d.wv", 4)))

container.sounds.Add(SoundSet("pickup_grenade", ["audio/sfx_pickup_launcher.wv"]))
container.sounds.Add(SoundSet("pickup_shotgun", ["audio/sfx_pickup_sg.wv"]))
container.sounds.Add(SoundSet("pickup_ninja", ["audio/sfx_pickup_ninja.wv"]))
container.sounds.Add(SoundSet("weapon_spawn", FileList("audio/sfx_spawn_wpn-%02d.wv", 3)))
container.sounds.Add(SoundSet("weapon_noammo", FileList("audio/wp_noammo-%02d.wv", 5)))

container.sounds.Add(SoundSet("hit", FileList("audio/sfx_hit_weak-%02d.wv", 2)))

container.sounds.Add(SoundSet("chat_server", ["audio/sfx_msg-server.wv"]))
container.sounds.Add(SoundSet("chat_client", ["audio/sfx_msg-client.wv"]))
container.sounds.Add(SoundSet("chat_highlight", ["audio/sfx_msg-highlight.wv"]))
container.sounds.Add(SoundSet("ctf_drop", ["audio/sfx_ctf_drop.wv"]))
container.sounds.Add(SoundSet("ctf_return", ["audio/sfx_ctf_rtn.wv"]))
container.sounds.Add(SoundSet("ctf_grab_pl", ["audio/sfx_ctf_grab_pl.wv"]))
container.sounds.Add(SoundSet("ctf_grab_en", ["audio/sfx_ctf_grab_en.wv"]))
container.sounds.Add(SoundSet("ctf_capture", ["audio/sfx_ctf_cap_pl.wv"]))

container.sounds.Add(SoundSet("menu", ["audio/music_menu.wv"]))

image_null = Image("null", "")
image_particles = Image("particles", "particles.png")
image_game = Image("game", "game.png")
image_browseicons = Image("browseicons", "ui/icons/browse.png", 1)
image_browsericon = Image("browser", "ui/icons/browser.png", 1)
image_emoticons = Image("emoticons", "emoticons.png")
image_demobuttons = Image("demobuttons", "ui/demo_buttons.png", 1)
image_fileicons = Image("fileicons", "ui/file_icons.png", 1)
image_guibuttons = Image("guibuttons", "ui/gui_buttons.png", 1)
image_guiicons = Image("guiicons", "ui/gui_icons.png", 1)
image_menuicons = Image("menuicons", "ui/icons/menu.png", 1)
image_soundicons = Image("soundicons", "ui/sound_icons.png", 1)
image_toolicons = Image("toolicons", "ui/icons/tools.png", 1)
image_arrowicons = Image("arrowicons", "ui/icons/arrows.png", 1)
image_friendicons = Image("friendicons", "ui/icons/friend.png", 1)
image_levelicons = Image("levelicons", "ui/icons/level.png", 1)
image_sidebaricons = Image("sidebaricons", "ui/icons/sidebar.png", 1)
image_chatwhisper = Image("chatwhisper", "ui/icons/chat_whisper.png", 1)
image_timerclock = Image("timerclock", "ui/icons/timer_clock.png", 1)

container.images.Add(image_null)
container.images.Add(image_game)
container.images.Add(Image("deadtee", "deadtee.png"))
container.images.Add(image_particles)
container.images.Add(Image("cursor", "ui/gui_cursor.png"))
container.images.Add(Image("banner", "ui/gui_logo.png"))
container.images.Add(image_emoticons)
container.images.Add(image_browseicons)
container.images.Add(image_browsericon)
container.images.Add(Image("console_bg", "ui/console.png"))
container.images.Add(Image("console_bar", "ui/console_bar.png"))
container.images.Add(image_demobuttons)
container.images.Add(image_fileicons)
container.images.Add(image_guibuttons)
container.images.Add(image_guiicons)
container.images.Add(Image("no_skinpart", "ui/no_skinpart.png"))
container.images.Add(image_menuicons)
container.images.Add(image_soundicons)
container.images.Add(image_toolicons)
container.images.Add(image_arrowicons)
container.images.Add(image_friendicons)
container.images.Add(image_levelicons)
container.images.Add(image_sidebaricons)
container.images.Add(image_chatwhisper)
container.images.Add(Image("raceflag", "race_flag.png"))
container.images.Add(image_timerclock)

container.pickups.Add(Pickup("health"))
container.pickups.Add(Pickup("armor"))
container.pickups.Add(Pickup("grenade"))
container.pickups.Add(Pickup("shotgun"))
container.pickups.Add(Pickup("laser"))
container.pickups.Add(Pickup("ninja", 90, 90))
container.pickups.Add(Pickup("gun"))
container.pickups.Add(Pickup("hammer"))

set_particles = SpriteSet("particles", image_particles, 8, 8)
set_game = SpriteSet("game", image_game, 32, 16)
set_tee_body = SpriteSet("tee_body", image_null, 2, 2)
set_tee_markings = SpriteSet("tee_markings", image_null, 1, 1)
set_tee_decoration = SpriteSet("tee_decoration", image_null, 2, 1)
set_tee_hands = SpriteSet("tee_hands", image_null, 2, 1)
set_tee_feet = SpriteSet("tee_feet", image_null, 2, 1)
set_tee_eyes = SpriteSet("tee_eyes", image_null, 2, 4)
set_tee_hats = SpriteSet("tee_hats", image_null, 1, 4)
set_tee_bot = SpriteSet("tee_bot", image_null, 12, 5)
set_browseicons = SpriteSet("browseicons", image_browseicons, 4, 2)
set_browsericon = SpriteSet("browsericon", image_browsericon, 1, 2)
set_emoticons = SpriteSet("emoticons", image_emoticons, 4, 4)
set_demobuttons = SpriteSet("demobuttons", image_demobuttons, 5, 1)
set_fileicons = SpriteSet("fileicons", image_fileicons, 8, 1)
set_guibuttons = SpriteSet("guibuttons", image_guibuttons, 12, 4)
set_guiicons = SpriteSet("guiicons", image_guiicons, 8, 2)
set_menuicons = SpriteSet("menuicons", image_menuicons, 4, 4)
set_toolicons = SpriteSet("toolicons", image_toolicons, 4, 2)
set_soundicons = SpriteSet("guiicons", image_soundicons, 1, 2)
set_arrowicons = SpriteSet("arrowicons", image_arrowicons, 4, 3)
set_friendicons = SpriteSet("friendicons", image_friendicons, 2, 2)
set_levelicons = SpriteSet("levelicons", image_levelicons, 4, 4)
set_sidebaricons = SpriteSet("sidebaricons", image_sidebaricons, 4, 2)
set_timerclock = SpriteSet("timerclock", image_timerclock, 1, 2)

container.spritesets.Add(set_particles)
container.spritesets.Add(set_game)
container.spritesets.Add(set_tee_body)
container.spritesets.Add(set_tee_markings)
container.spritesets.Add(set_tee_decoration)
container.spritesets.Add(set_tee_hands)
container.spritesets.Add(set_tee_feet)
container.spritesets.Add(set_tee_eyes)
container.spritesets.Add(set_tee_hats)
container.spritesets.Add(set_tee_bot)
container.spritesets.Add(set_browseicons)
container.spritesets.Add(set_emoticons)
container.spritesets.Add(set_demobuttons)
container.spritesets.Add(set_fileicons)
container.spritesets.Add(set_guibuttons)
container.spritesets.Add(set_guiicons)
container.spritesets.Add(set_menuicons)
container.spritesets.Add(set_soundicons)
container.spritesets.Add(set_toolicons)
container.spritesets.Add(set_arrowicons)
container.spritesets.Add(set_friendicons)
container.spritesets.Add(set_levelicons)
container.spritesets.Add(set_sidebaricons)
container.spritesets.Add(set_timerclock)
container.spritesets.Add(set_browsericon)


container.sprites.Add(Sprite("part_slice", set_particles, 0,0,1,1))
container.sprites.Add(Sprite("part_ball", set_particles, 1,0,1,1))
container.sprites.Add(Sprite("part_splat01", set_particles, 2,0,1,1))
container.sprites.Add(Sprite("part_splat02", set_particles, 3,0,1,1))
container.sprites.Add(Sprite("part_splat03", set_particles, 4,0,1,1))

container.sprites.Add(Sprite("part_smoke", set_particles, 0,1,1,1))
container.sprites.Add(Sprite("part_shell", set_particles, 0,2,2,2))
container.sprites.Add(Sprite("part_expl01", set_particles, 0,4,4,4))
container.sprites.Add(Sprite("part_airjump", set_particles, 2,2,2,2))
container.sprites.Add(Sprite("part_hit01", set_particles, 4,1,2,2))

container.sprites.Add(Sprite("health_full", set_game, 21,0,2,2))
container.sprites.Add(Sprite("health_empty", set_game, 23,0,2,2))
container.sprites.Add(Sprite("armor_full", set_game, 21,2,2,2))
container.sprites.Add(Sprite("armor_empty", set_game, 23,2,2,2))

container.sprites.Add(Sprite("star1", set_game, 15,0,2,2))
container.sprites.Add(Sprite("star2", set_game, 17,0,2,2))
container.sprites.Add(Sprite("star3", set_game, 19,0,2,2))

container.sprites.Add(Sprite("part1", set_game, 6,0,1,1))
container.sprites.Add(Sprite("part2", set_game, 6,1,1,1))
container.sprites.Add(Sprite("part3", set_game, 7,0,1,1))
container.sprites.Add(Sprite("part4", set_game, 7,1,1,1))
container.sprites.Add(Sprite("part5", set_game, 8,0,1,1))
container.sprites.Add(Sprite("part6", set_game, 8,1,1,1))
container.sprites.Add(Sprite("part7", set_game, 9,0,2,2))
container.sprites.Add(Sprite("part8", set_game, 11,0,2,2))
container.sprites.Add(Sprite("part9", set_game, 13,0,2,2))

container.sprites.Add(Sprite("weapon_gun_body", set_game, 2,4,4,2))
container.sprites.Add(Sprite("weapon_gun_cursor", set_game, 0,4,2,2))
container.sprites.Add(Sprite("weapon_gun_proj", set_game, 6,4,2,2))
container.sprites.Add(Sprite("weapon_gun_muzzle1", set_game, 8,4,3,2))
container.sprites.Add(Sprite("weapon_gun_muzzle2", set_game, 12,4,3,2))
container.sprites.Add(Sprite("weapon_gun_muzzle3", set_game, 16,4,3,2))

container.sprites.Add(Sprite("weapon_shotgun_body", set_game, 2,6,8,2))
container.sprites.Add(Sprite("weapon_shotgun_cursor", set_game, 0,6,2,2))
container.sprites.Add(Sprite("weapon_shotgun_proj", set_game, 10,6,2,2))
container.sprites.Add(Sprite("weapon_shotgun_muzzle1", set_game, 12,6,3,2))
container.sprites.Add(Sprite("weapon_shotgun_muzzle2", set_game, 16,6,3,2))
container.sprites.Add(Sprite("weapon_shotgun_muzzle3", set_game, 20,6,3,2))

container.sprites.Add(Sprite("weapon_grenade_body", set_game, 2,8,7,2))
container.sprites.Add(Sprite("weapon_grenade_cursor", set_game, 0,8,2,2))
container.sprites.Add(Sprite("weapon_grenade_proj", set_game, 10,8,2,2))

container.sprites.Add(Sprite("weapon_hammer_body", set_game, 2,1,4,3))
container.sprites.Add(Sprite("weapon_hammer_cursor", set_game, 0,0,2,2))
container.sprites.Add(Sprite("weapon_hammer_proj", set_game, 0,0,0,0))

container.sprites.Add(Sprite("weapon_ninja_body", set_game, 2,10,8,2))
container.sprites.Add(Sprite("weapon_ninja_cursor", set_game, 0,10,2,2))
container.sprites.Add(Sprite("weapon_ninja_proj", set_game, 0,0,0,0))

container.sprites.Add(Sprite("weapon_laser_body", set_game, 2,12,7,3))
container.sprites.Add(Sprite("weapon_laser_cursor", set_game, 0,12,2,2))
container.sprites.Add(Sprite("weapon_laser_proj", set_game, 10,12,2,2))

container.sprites.Add(Sprite("hook_chain", set_game, 2,0,1,1))
container.sprites.Add(Sprite("hook_head", set_game, 3,0,2,1))

container.sprites.Add(Sprite("weapon_ninja_muzzle1", set_game, 25,0,7,4))
container.sprites.Add(Sprite("weapon_ninja_muzzle2", set_game, 25,4,7,4))
container.sprites.Add(Sprite("weapon_ninja_muzzle3", set_game, 25,8,7,4))

container.sprites.Add(Sprite("pickup_health", set_game, 10,2,2,2))
container.sprites.Add(Sprite("pickup_armor", set_game, 12,2,2,2))
container.sprites.Add(Sprite("pickup_grenade", set_game, 2,8,7,2))
container.sprites.Add(Sprite("pickup_shotgun", set_game, 2,6,8,2))
container.sprites.Add(Sprite("pickup_laser", set_game, 2,12,7,3))
container.sprites.Add(Sprite("pickup_ninja", set_game, 2,10,8,2))
container.sprites.Add(Sprite("pickup_gun", set_game, 2,4,4,2))
container.sprites.Add(Sprite("pickup_hammer", set_game, 2,1,4,3))

container.sprites.Add(Sprite("flag_blue", set_game, 12,8,4,8))
container.sprites.Add(Sprite("flag_red", set_game, 16,8,4,8))

container.sprites.Add(Sprite("ninja_bar_full_left", set_game, 21,4,1,2))
container.sprites.Add(Sprite("ninja_bar_full", set_game, 22,4,1,2))
container.sprites.Add(Sprite("ninja_bar_empty", set_game, 23,4,1,2))
container.sprites.Add(Sprite("ninja_bar_empty_right", set_game, 24,4,1,2))

container.sprites.Add(Sprite("tee_body_outline", set_tee_body, 0,0,1,1))
container.sprites.Add(Sprite("tee_body", set_tee_body, 1,0,1,1))
container.sprites.Add(Sprite("tee_body_shadow", set_tee_body, 0,1,1,1))
container.sprites.Add(Sprite("tee_body_upper_outline", set_tee_body, 1,1,1,1))

container.sprites.Add(Sprite("tee_marking", set_tee_markings, 0,0,1,1))

container.sprites.Add(Sprite("tee_decoration", set_tee_decoration, 0,0,1,1))
container.sprites.Add(Sprite("tee_decoration_outline", set_tee_decoration, 1,0,1,1))

container.sprites.Add(Sprite("tee_hand", set_tee_hands, 0,0,1,1))
container.sprites.Add(Sprite("tee_hand_outline", set_tee_hands, 1,0,1,1))

container.sprites.Add(Sprite("tee_foot", set_tee_feet, 0,0,1,1))
container.sprites.Add(Sprite("tee_foot_outline", set_tee_feet, 1,0,1,1))

container.sprites.Add(Sprite("tee_eyes_normal", set_tee_eyes, 0,0,1,1))
container.sprites.Add(Sprite("tee_eyes_angry", set_tee_eyes, 1,0,1,1))
container.sprites.Add(Sprite("tee_eyes_pain", set_tee_eyes, 0,1,1,1))
container.sprites.Add(Sprite("tee_eyes_happy", set_tee_eyes, 1,1,1,1))
container.sprites.Add(Sprite("tee_eyes_surprise", set_tee_eyes, 0,2,1,1))

container.sprites.Add(Sprite("tee_hats_top1", set_tee_hats, 0,0,1,1))
container.sprites.Add(Sprite("tee_hats_top2", set_tee_hats, 0,1,1,1))
container.sprites.Add(Sprite("tee_hats_side1", set_tee_hats, 0,2,1,1))
container.sprites.Add(Sprite("tee_hats_side2", set_tee_hats, 0,3,1,1))

container.sprites.Add(Sprite("tee_bot_glow", set_tee_bot, 0,0,4,4))
container.sprites.Add(Sprite("tee_bot_foreground", set_tee_bot, 4,0,4,4))
container.sprites.Add(Sprite("tee_bot_background", set_tee_bot, 8,0,4,4))

container.sprites.Add(Sprite("oop", set_emoticons, 0, 0, 1, 1))
container.sprites.Add(Sprite("exclamation", set_emoticons, 1, 0, 1, 1))
container.sprites.Add(Sprite("hearts", set_emoticons, 2, 0, 1, 1))
container.sprites.Add(Sprite("drop", set_emoticons, 3, 0, 1, 1))
container.sprites.Add(Sprite("dotdot", set_emoticons, 0, 1, 1, 1))
container.sprites.Add(Sprite("music", set_emoticons, 1, 1, 1, 1))
container.sprites.Add(Sprite("sorry", set_emoticons, 2, 1, 1, 1))
container.sprites.Add(Sprite("ghost", set_emoticons, 3, 1, 1, 1))
container.sprites.Add(Sprite("sushi", set_emoticons, 0, 2, 1, 1))
container.sprites.Add(Sprite("splattee", set_emoticons, 1, 2, 1, 1))
container.sprites.Add(Sprite("deviltee", set_emoticons, 2, 2, 1, 1))
container.sprites.Add(Sprite("zomg", set_emoticons, 3, 2, 1, 1))
container.sprites.Add(Sprite("zzz", set_emoticons, 0, 3, 1, 1))
container.sprites.Add(Sprite("wtf", set_emoticons, 1, 3, 1, 1))
container.sprites.Add(Sprite("eyes", set_emoticons, 2, 3, 1, 1))
container.sprites.Add(Sprite("question", set_emoticons, 3, 3, 1, 1))

container.sprites.Add(Sprite("browse_lock_a", set_browseicons, 0,0,1,1))
container.sprites.Add(Sprite("browse_lock_b", set_browseicons, 0,1,1,1))
container.sprites.Add(Sprite("browse_unpure_a", set_browseicons, 1,0,1,1))
container.sprites.Add(Sprite("browse_unpure_b", set_browseicons, 1,1,1,1))
container.sprites.Add(Sprite("browse_star_a", set_browseicons, 2,0,1,1))
container.sprites.Add(Sprite("browse_star_b", set_browseicons, 2,1,1,1))
container.sprites.Add(Sprite("browse_heart_a", set_browseicons, 3,0,1,1))
container.sprites.Add(Sprite("browse_heart_b", set_browseicons, 3,1,1,1))

container.sprites.Add(Sprite("demobutton_play", set_demobuttons, 0,0,1,1))
container.sprites.Add(Sprite("demobutton_pause", set_demobuttons, 1,0,1,1))
container.sprites.Add(Sprite("demobutton_stop", set_demobuttons, 2,0,1,1))
container.sprites.Add(Sprite("demobutton_slower", set_demobuttons, 3,0,1,1))
container.sprites.Add(Sprite("demobutton_faster", set_demobuttons, 4,0,1,1))

container.sprites.Add(Sprite("file_demo1", set_fileicons, 0,0,1,1))
container.sprites.Add(Sprite("file_demo2", set_fileicons, 1,0,1,1))
container.sprites.Add(Sprite("file_folder", set_fileicons, 2,0,1,1))
container.sprites.Add(Sprite("file_map1", set_fileicons, 5,0,1,1))
container.sprites.Add(Sprite("file_map2", set_fileicons, 6,0,1,1))

container.sprites.Add(Sprite("guibutton_off", set_guibuttons, 0,0,4,4))
container.sprites.Add(Sprite("guibutton_on", set_guibuttons, 4,0,4,4))
container.sprites.Add(Sprite("guibutton_hover", set_guibuttons, 8,0,4,4))

container.sprites.Add(Sprite("guiicon_mute", set_guiicons, 0,0,4,2))
container.sprites.Add(Sprite("guiicon_friend", set_guiicons, 4,0,4,2))

container.sprites.Add(Sprite("menu_checkbox_active", set_menuicons, 0,0,1,1))
container.sprites.Add(Sprite("menu_checkbox_inactive", set_menuicons, 0,1,1,1))
container.sprites.Add(Sprite("menu_checkbox_hover", set_menuicons, 0,2,1,1))
container.sprites.Add(Sprite("menu_collapsed", set_menuicons, 1,0,1,1))
container.sprites.Add(Sprite("menu_expanded", set_menuicons, 1,1,1,1))

container.sprites.Add(Sprite("soundicon_on", set_soundicons, 0,0,1,1))
container.sprites.Add(Sprite("soundicon_mute", set_soundicons, 0,1,1,1))

container.sprites.Add(Sprite("tool_up_a", set_toolicons, 0,0,1,1))
container.sprites.Add(Sprite("tool_up_b", set_toolicons, 0,1,1,1))
container.sprites.Add(Sprite("tool_down_a", set_toolicons, 1,0,1,1))
container.sprites.Add(Sprite("tool_down_b", set_toolicons, 1,1,1,1))
container.sprites.Add(Sprite("tool_edit_a", set_toolicons, 2,0,1,1))
container.sprites.Add(Sprite("tool_edit_b", set_toolicons, 2,1,1,1))
container.sprites.Add(Sprite("tool_x_a", set_toolicons, 3,0,1,1))
container.sprites.Add(Sprite("tool_x_b", set_toolicons, 3,1,1,1))

container.sprites.Add(Sprite("arrow_left_a", set_arrowicons, 0,0,1,1))
container.sprites.Add(Sprite("arrow_left_b", set_arrowicons, 0,1,1,1))
container.sprites.Add(Sprite("arrow_left_c", set_arrowicons, 0,2,1,1))
container.sprites.Add(Sprite("arrow_up_a", set_arrowicons, 1,0,1,1))
container.sprites.Add(Sprite("arrow_up_b", set_arrowicons, 1,1,1,1))
container.sprites.Add(Sprite("arrow_up_c", set_arrowicons, 1,2,1,1))
container.sprites.Add(Sprite("arrow_right_a", set_arrowicons, 2,0,1,1))
container.sprites.Add(Sprite("arrow_right_b", set_arrowicons, 2,1,1,1))
container.sprites.Add(Sprite("arrow_right_c", set_arrowicons, 2,2,1,1))
container.sprites.Add(Sprite("arrow_down_a", set_arrowicons, 3,0,1,1))
container.sprites.Add(Sprite("arrow_down_b", set_arrowicons, 3,1,1,1))
container.sprites.Add(Sprite("arrow_down_c", set_arrowicons, 3,2,1,1))

container.sprites.Add(Sprite("friend_plus_a", set_friendicons, 0,0,1,1))
container.sprites.Add(Sprite("friend_plus_b", set_friendicons, 0,1,1,1))
container.sprites.Add(Sprite("friend_x_a", set_friendicons, 1,0,1,1))
container.sprites.Add(Sprite("friend_x_b", set_friendicons, 1,1,1,1))

container.sprites.Add(Sprite("level_a_on", set_levelicons, 0,0,1,1))
container.sprites.Add(Sprite("level_a_a", set_levelicons, 0,1,1,1))
container.sprites.Add(Sprite("level_a_b", set_levelicons, 0,2,1,1))
container.sprites.Add(Sprite("level_b_on", set_levelicons, 1,0,1,1))
container.sprites.Add(Sprite("level_b_a", set_levelicons, 1,1,1,1))
container.sprites.Add(Sprite("level_b_b", set_levelicons, 1,2,1,1))
container.sprites.Add(Sprite("level_c_on", set_levelicons, 2,0,1,1))
container.sprites.Add(Sprite("level_c_a", set_levelicons, 2,1,1,1))
container.sprites.Add(Sprite("level_c_b", set_levelicons, 2,2,1,1))

container.sprites.Add(Sprite("sidebar_refresh_a", set_sidebaricons, 0,0,1,1))
container.sprites.Add(Sprite("sidebar_refresh_b", set_sidebaricons, 0,1,1,1))
container.sprites.Add(Sprite("sidebar_friend_a", set_sidebaricons, 1,0,1,1))
container.sprites.Add(Sprite("sidebar_friend_b", set_sidebaricons, 1,1,1,1))
container.sprites.Add(Sprite("sidebar_filter_a", set_sidebaricons, 2,0,1,1))
container.sprites.Add(Sprite("sidebar_filter_b", set_sidebaricons, 2,1,1,1))
container.sprites.Add(Sprite("sidebar_info_a", set_sidebaricons, 3,0,1,1))
container.sprites.Add(Sprite("sidebar_info_b", set_sidebaricons, 3,1,1,1))

container.sprites.Add(Sprite("browser_a", set_browsericon, 0,0,1,1))
container.sprites.Add(Sprite("browser_b", set_browsericon, 0,1,1,1))

container.sprites.Add(Sprite("timerclock_a", set_timerclock, 0,0,1,1))
container.sprites.Add(Sprite("timerclock_b", set_timerclock, 0,1,1,1))

anim = Animation("base")
anim.body.frames.Add(AnimKeyframe(0, 0, -4, 0))
anim.back_foot.frames.Add(AnimKeyframe(0, 0, 10, 0))
anim.front_foot.frames.Add(AnimKeyframe(0, 0, 10, 0))
container.animations.Add(anim)

anim = Animation("idle")
anim.back_foot.frames.Add(AnimKeyframe(0, -7, 0, 0))
anim.front_foot.frames.Add(AnimKeyframe(0, 7, 0, 0))
container.animations.Add(anim)

anim = Animation("inair")
anim.back_foot.frames.Add(AnimKeyframe(0, -3, 0, -0.1))
anim.front_foot.frames.Add(AnimKeyframe(0, 3, 0, -0.1))
container.animations.Add(anim)

anim = Animation("walk")
anim.body.frames.Add(AnimKeyframe(0.0, 0, 0, 0))
anim.body.frames.Add(AnimKeyframe(0.2, 0,-1, 0))
anim.body.frames.Add(AnimKeyframe(0.4, 0, 0, 0))
anim.body.frames.Add(AnimKeyframe(0.6, 0, 0, 0))
anim.body.frames.Add(AnimKeyframe(0.8, 0,-1, 0))
anim.body.frames.Add(AnimKeyframe(1.0, 0, 0, 0))

anim.back_foot.frames.Add(AnimKeyframe(0.0, 8, 0, 0))
anim.back_foot.frames.Add(AnimKeyframe(0.2, -8, 0, 0))
anim.back_foot.frames.Add(AnimKeyframe(0.4,-10,-4, 0.2))
anim.back_foot.frames.Add(AnimKeyframe(0.6, -8,-8, 0.3))
anim.back_foot.frames.Add(AnimKeyframe(0.8, 4,-4,-0.2))
anim.back_foot.frames.Add(AnimKeyframe(1.0, 8, 0, 0))

anim.front_foot.frames.Add(AnimKeyframe(0.0,-10,-4, 0.2))
anim.front_foot.frames.Add(AnimKeyframe(0.2, -8,-8, 0.3))
anim.front_foot.frames.Add(AnimKeyframe(0.4, 4,-4,-0.2))
anim.front_foot.frames.Add(AnimKeyframe(0.6, 8, 0, 0))
anim.front_foot.frames.Add(AnimKeyframe(0.8, 8, 0, 0))
anim.front_foot.frames.Add(AnimKeyframe(1.0,-10,-4, 0.2))
container.animations.Add(anim)

anim = Animation("hammer_swing")
anim.attach.frames.Add(AnimKeyframe(0.0, 0, 0, -0.10))
anim.attach.frames.Add(AnimKeyframe(0.3, 0, 0, 0.25))
anim.attach.frames.Add(AnimKeyframe(0.4, 0, 0, 0.30))
anim.attach.frames.Add(AnimKeyframe(0.5, 0, 0, 0.25))
anim.attach.frames.Add(AnimKeyframe(1.0, 0, 0, -0.10))
container.animations.Add(anim)

anim = Animation("ninja_swing")
anim.attach.frames.Add(AnimKeyframe(0.00, 0, 0, -0.25))
anim.attach.frames.Add(AnimKeyframe(0.10, 0, 0, -0.05))
anim.attach.frames.Add(AnimKeyframe(0.15, 0, 0, 0.35))
anim.attach.frames.Add(AnimKeyframe(0.42, 0, 0, 0.40))
anim.attach.frames.Add(AnimKeyframe(0.50, 0, 0, 0.35))
anim.attach.frames.Add(AnimKeyframe(1.00, 0, 0, -0.25))
container.animations.Add(anim)

weapon = WeaponSpec(container, "hammer")
weapon.firedelay.Set(125)
weapon.damage.Set(3)
weapon.visual_size.Set(96)
weapon.offsetx.Set(4)
weapon.offsety.Set(-20)
container.weapons.hammer.base.Set(weapon)
container.weapons.id.Add(weapon)

weapon = WeaponSpec(container, "gun")
weapon.firedelay.Set(125)
weapon.damage.Set(1)
weapon.ammoregentime.Set(500)
weapon.visual_size.Set(64)
weapon.offsetx.Set(32)
weapon.offsety.Set(-4)
weapon.muzzleoffsetx.Set(50)
weapon.muzzleoffsety.Set(6)
container.weapons.gun.base.Set(weapon)
container.weapons.id.Add(weapon)

weapon = WeaponSpec(container, "shotgun")
weapon.firedelay.Set(500)
weapon.damage.Set(1)
weapon.visual_size.Set(96)
weapon.offsetx.Set(24)
weapon.offsety.Set(-2)
weapon.muzzleoffsetx.Set(70)
weapon.muzzleoffsety.Set(6)
container.weapons.shotgun.base.Set(weapon)
container.weapons.id.Add(weapon)

weapon = WeaponSpec(container, "grenade")
weapon.firedelay.Set(500) # TODO: fix this
weapon.damage.Set(6)
weapon.visual_size.Set(96)
weapon.offsetx.Set(24)
weapon.offsety.Set(-2)
container.weapons.grenade.base.Set(weapon)
container.weapons.id.Add(weapon)

weapon = WeaponSpec(container, "laser")
weapon.firedelay.Set(800)
weapon.damage.Set(5)
weapon.visual_size.Set(92)
weapon.offsetx.Set(24)
weapon.offsety.Set(-2)
container.weapons.laser.base.Set(weapon)
container.weapons.id.Add(weapon)

weapon = WeaponSpec(container, "ninja")
weapon.firedelay.Set(800)
weapon.damage.Set(9)
weapon.visual_size.Set(96)
weapon.offsetx.Set(0)
weapon.offsety.Set(0)
weapon.muzzleoffsetx.Set(40)
weapon.muzzleoffsety.Set(-4)
container.weapons.ninja.base.Set(weapon)
container.weapons.id.Add(weapon)
