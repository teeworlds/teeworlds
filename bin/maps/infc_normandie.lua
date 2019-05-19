default_radius = 800
default_inf_radius = 360

function init()
	-- hardcode passed C/C++ values here (e.g for tests)
end

function get_circle_pos()
	-- x, y, radius
	circle_positions = {
		175, 22, default_radius,
	}
	return circle_positions
end

function get_inf_circle_pos()
	-- x, y, radius
	inf_circle_positions = {
		10, 55, default_inf_radius,
	}
	return inf_circle_positions
end