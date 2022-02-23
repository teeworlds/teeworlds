/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_TUNING_H
#define GAME_TUNING_H
#undef GAME_TUNING_H // this file will be included several times

// physics tuning
MACRO_TUNING_PARAM(GroundControlSpeed, ground_control_speed, 10.0f)
MACRO_TUNING_PARAM(GroundControlAccel, ground_control_accel, 100.0f / TicksPerSecond)
MACRO_TUNING_PARAM(GroundFriction, ground_friction, 0.5f)
MACRO_TUNING_PARAM(GroundJumpImpulse, ground_jump_impulse, 13.2f)
MACRO_TUNING_PARAM(AirJumpImpulse, air_jump_impulse, 12.0f)
MACRO_TUNING_PARAM(AirControlSpeed, air_control_speed, 250.0f / TicksPerSecond)
MACRO_TUNING_PARAM(AirControlAccel, air_control_accel, 1.5f)
MACRO_TUNING_PARAM(AirFriction, air_friction, 0.95f)
MACRO_TUNING_PARAM(HookLength, hook_length, 380.0f)
MACRO_TUNING_PARAM(HookFireSpeed, hook_fire_speed, 80.0f)
MACRO_TUNING_PARAM(HookDragAccel, hook_drag_accel, 3.0f)
MACRO_TUNING_PARAM(HookDragSpeed, hook_drag_speed, 15.0f)
MACRO_TUNING_PARAM(HookLiquidSlowdown, hook_liq_slowdown, 0.5f)
MACRO_TUNING_PARAM(Gravity, gravity, 0.5f)

MACRO_TUNING_PARAM(VelrampStart, velramp_start, 550)
MACRO_TUNING_PARAM(VelrampRange, velramp_range, 2000)
MACRO_TUNING_PARAM(VelrampCurvature, velramp_curvature, 1.4f)
//liquid physics nondiving
MACRO_TUNING_PARAM(LiquidHorizontalDecel, liq_x_decel, 0.95)
MACRO_TUNING_PARAM(LiquidVerticalDecel, liq_y_decel, 0.95)
MACRO_TUNING_PARAM(LiquidPushOut, liq_push_out, 0.3f)
MACRO_TUNING_PARAM(LiquidCursorMaxSpeed, liq_curs_swim_max_speed, 5.0f)
MACRO_TUNING_PARAM(LiquidCursorRequireDiving, liq_curs_swim_req_gear, 0)
MACRO_TUNING_PARAM(LiquidExplosionRadiusMultiplier, liq_explo_radius_mltpr, 0.67f)
MACRO_TUNING_PARAM(LiquidExplosionDamageMultiplier, liq_explo_dmg_mltp, 0.67f)
MACRO_TUNING_PARAM(LiquidFlagResistance, liq_flag_resistance, 0.995f)
//liquid physics diving
//MACRO_TUNING_PARAM(LiquidSwimmingAccel, liq_swim_accel, 0.1f)
//MACRO_TUNING_PARAM(LiquidSwimmingTopAccel, liq_swim_accel_top, 5.0f)
MACRO_TUNING_PARAM(LiquidDivingGearMaxHorizontalVelocity, liq_div_gear_max_x_vel, 15.0f)
MACRO_TUNING_PARAM(LiquidDivingGearBreath, liq_div_gear_air_tick, 3000)
MACRO_TUNING_PARAM(LiquidDivingCursor, liq_div_gear_curs_swim, 0)
MACRO_TUNING_PARAM(LiquidDivingCursorMaxSpeed, liq_div_gear_curs_swim_max_speed, 7.5f)
MACRO_TUNING_PARAM(LiquidDivingMaxSpeed, liq_div_max_speed_reach, 7.5f)
//liquid physics general
MACRO_TUNING_PARAM(LiquidAirTicks, liq_air_tick, 500)
MACRO_TUNING_PARAM(LiquidTicksPerSuffocationDmg, liq_suff_dmg_delay, 15)
MACRO_TUNING_PARAM(LiquidOnlyHeartDmg, liq_heart_dmg, 1)
MACRO_TUNING_PARAM(LiquidFractionofSwimming, liq_fraction_of_push, 0.2f)
MACRO_TUNING_PARAM(LiquidProjectileResistance, liq_proj_resistance, 0.2f)
MACRO_TUNING_PARAM(LiquidSlowProjectilesAfterWater, liq_proj_leave_water, 1.0f)
MACRO_TUNING_PARAM(LiquidUseAscendDescend, liq_asc_desc, 1.0f)
// weapon tuning
MACRO_TUNING_PARAM(PickUpDisturbedTime, pickup_disturbed_time, 10.0f)

MACRO_TUNING_PARAM(GunCurvature, gun_curvature, 1.25f)
MACRO_TUNING_PARAM(GunSpeed, gun_speed, 2200.0f)
MACRO_TUNING_PARAM(GunLifetime, gun_lifetime, 2.0f)

MACRO_TUNING_PARAM(ShotgunCurvature, shotgun_curvature, 1.25f)
MACRO_TUNING_PARAM(ShotgunSpeed, shotgun_speed, 2750.0f)
MACRO_TUNING_PARAM(ShotgunSpeeddiff, shotgun_speeddiff, 0.8f)
MACRO_TUNING_PARAM(ShotgunLifetime, shotgun_lifetime, 0.20f)
MACRO_TUNING_PARAM(ShotgunWaterResistance, shotgun_liq_resistance, 0.25f)

MACRO_TUNING_PARAM(GrenadeCurvature, grenade_curvature, 7.0f)
MACRO_TUNING_PARAM(GrenadeSpeed, grenade_speed, 1000.0f)
MACRO_TUNING_PARAM(GrenadeLifetime, grenade_lifetime, 2.0f)

MACRO_TUNING_PARAM(HarpoonSpeed, harpoon_speed, 2000.0f)
MACRO_TUNING_PARAM(HarpoonCurvature, harpoon_curvature, 1.25f)
MACRO_TUNING_PARAM(HarpoonDragSpeed, harpoon_drag_speed, 10.0f)
MACRO_TUNING_PARAM(HarpoonEntityDragSpeed, harpoon_entity_drag_speed, 50.0f)
MACRO_TUNING_PARAM(HarpoonEntityMinSpeed, harpoon_entity_min_speed, 30.f)
MACRO_TUNING_PARAM(HarpoonFlagMinSpeed, harpoon_entity_flag_min_speed, 8.0f)
MACRO_TUNING_PARAM(HarpoonMultiplier, harpoon_speed_mltpr, 5.0f)
MACRO_TUNING_PARAM(HarpoonEntityMultiplier, harpoon_entity_speed_mltpr, 10.0f)
MACRO_TUNING_PARAM(HarpoonFlagDragSpeed, harpoon_flag_drag_speed, 8.0f)
MACRO_TUNING_PARAM(HarpoonPickupIgnoreRange, harpoon_pickup_ignore_range, 5.0f)
MACRO_TUNING_PARAM(HarpoonReloadTime, harpoon_reload_time, 1.0f)
MACRO_TUNING_PARAM(HarpoonDamage, harpoon_damage, 2.0f)
MACRO_TUNING_PARAM(HarpoonDelay, harpoon_extra_dmg_delay, 1.5f)

MACRO_TUNING_PARAM(LaserReach, laser_reach, 800.0f)
MACRO_TUNING_PARAM(LaserBounceDelay, laser_bounce_delay, 150)
MACRO_TUNING_PARAM(LaserBounceNum, laser_bounce_num, 1)
MACRO_TUNING_PARAM(LaserBounceCost, laser_bounce_cost, 0)
MACRO_TUNING_PARAM(LaserWaterReachMultiplier, laser_liq_reach, 1.5f)
MACRO_TUNING_PARAM(LaserWaterDiffraction, laser_liq_diffraction, 1.33f)

MACRO_TUNING_PARAM(PlayerCollision, player_collision, 1)
MACRO_TUNING_PARAM(PlayerHooking, player_hooking, 1)
#endif
