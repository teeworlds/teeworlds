-- shrinking from 10000 to 800 (with shrink speed = 1) takes 184 seconds
-- why: 10000 - 800 = 9200; 9200 / TickSpeed = 184; where TickSpeed = 50
default_radius = 10000
min_radius = 800
circle_shrink_speed = 1
default_inf_radius = 250
timelimit = 5

circle_positions = {}

inf_circle_positions = {
	-- x, y, radius
	150, 94, default_inf_radius,
	128, 106, default_inf_radius,
	124, 90, default_inf_radius,
}

math.randomseed(os.time())

function infc_init()
	-- infc_init() is called 10 secs after round start
	-- hardcode passed C/C++ values here (e.g for tests)
	-- passed values: infc_num_players
	if infc_num_players < 10 then
		timelimit = 4
		case = math.random(1, 3) -- three different cases
		if case == 1 then
			circle_positions = { 
				-- x, y, radius, minradius
				208, 72, default_radius, min_radius, circle_shrink_speed,
			}
		end
		
		if case == 2 then
			circle_positions = { 
				-- x, y, radius, minradius
				23, 89, default_radius, min_radius, circle_shrink_speed,
			}
		end
		
		if case == 3 then
			circle_positions = { 
				-- x, y, radius, minradius
				197, 11, default_radius, min_radius, circle_shrink_speed,
			}
		end
	else
		timelimit = 5
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