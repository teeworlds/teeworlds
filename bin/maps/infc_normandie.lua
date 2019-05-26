default_radius = 6500
min_radius = 800
circle_shrink_speed = 1
default_inf_radius = 360
timelimit = 4

circle_positions = {}

inf_circle_positions = {
	-- x, y, radius
	10, 55, default_inf_radius,
	15, 21, default_inf_radius,
}

math.randomseed(os.time())

function infc_init()
	-- infc_init() is called 10 secs after round start
	-- hardcode passed C/C++ values here (e.g for tests)
	-- passed values: infc_num_players
	if infc_num_players < 10 then
		case = math.random(1, 1) -- two different cases
		
		if case == 1 then
			circle_positions = { 
				-- x, y, radius, minradius
				175, 22, default_radius, min_radius, circle_shrink_speed,
			}
		end
	end
end

-- Try not to modify functions below
function infc_get_circle_pos()
	return circle_positions
end

function infc_get_inf_circle_pos()
	return inf_circle_positions
end

function infc_get_timelimit()
	return timelimit
end

function infc_get_circle_min_radius()
	return min_radius
end

function infc_get_circle_shrink_speed()
	return circle_shrink_speed
end