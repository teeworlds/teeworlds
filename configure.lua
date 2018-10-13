
function loadfile_(filename, env)
	local file
	if _VERSION == "Lua 5.1" then
		file = loadfile(filename)
		if file then
			setfenv(file, env)
		end
	else
		file = loadfile(filename, nil, env)
	end
	return file
end

--[[@GROUP Configuration@END]]--

--[[@FUNCTION
	TODO
@END]]--
function NewConfig(on_configured_callback)
	local config = {}

	config.OnConfigured = function(self)
		return true
	end

	if on_configured_callback then config.OnConfigured = on_configured_callback end

	config.options = {}
	config.settings = NewSettings()

	config.NewSettings = function(self)
		local s = NewSettings()
		for _,v in pairs(self.options) do
			v:Apply(s)
		end
		return s
	end

	config.Add = function(self, o)
		table.insert(self.options, o)
		self[o.name] = o
	end

	config.Print = function(self)
		for k,v in pairs(self.options) do
			print(v:FormatDisplay())
		end
	end

	config.Save = function(self, filename)
		print("saved configuration to '"..filename.."'")
		local file = io.open(filename, "w")

		-- Define a little helper function to save options
		local saver = {}
		saver.file = file

		saver.line = function(self, str)
			self.file:write(str .. "\n")
		end

		saver.option = function(self, option, name)
			local valuestr = "no"
			if type(option[name]) == type(0) then
				valuestr = option[name]
			elseif type(option[name]) == type(true) then
				valuestr = "false"
				if option[name] then
					valuestr = "true"
				end
			elseif type(option[name]) == type("") then
				valuestr = "'"..option[name].."'"
			else
				error("option "..name.." have a value of type ".. type(option[name]).." that can't be saved")
			end
			self.file:write(option.name.."."..name.." = ".. valuestr.."\n")
		end

		-- Save all the options
		for k,v in pairs(self.options) do
			v:Save(saver)
		end
		file:close()
	end

	config.Load = function(self, filename)
		local options_table = {}
		local options_func = loadfile_(filename, options_table)

		if not options_func then
			print("auto configuration")
			self:Config(filename)
			options_func = loadfile_(filename, options_table)
		end

		if options_func then
			-- Setup the options tables
			for k,v in pairs(self.options) do
				options_table[v.name] = {}
			end

			-- this is to make sure that we get nice error messages when
			-- someone sets an option that isn't valid.
			local mt = {}
			mt.__index = function(t, key)
				local v = rawget(t, key)
				if v ~= nil then return v end
				error("there is no configuration option named '" .. key .. "'")
			end

			setmetatable(options_table, mt)

			-- Process the options
			options_func()

			-- Copy the options
			for k,v in pairs(self.options) do
				if options_table[v.name] then
					for k2,v2 in pairs(options_table[v.name]) do
						v[k2] = v2
					end
					v.auto_detected = false
				end
			end
		else
			print("error: no '"..filename.."' found")
			print("")
			print("run 'bam config' to generate")
			print("run 'bam config help' for configuration options")
			print("")
			os.exit(1)
		end
	end

	config.Config = function(self, filename)
		print("")
		print("configuration:")
		if _bam_targets[1] == "print" then
			self:Load(filename)
			self:Print()
			print("")
			print("notes:")
			self:OnConfigured()
			print("")
		else
			self:Autodetect()
			print("")
			print("notes:")
			if self:OnConfigured() then
				self:Save(filename)
			end
			print("")
		end

	end

	config.Autodetect = function(self)
		for k,v in pairs(self.options) do
			v:Check(self.settings)
			print(v:FormatDisplay())
			self[v.name] = v
		end
	end

	config.PrintHelp = function(self)
		print("options:")
		for k,v in pairs(self.options) do
			if v.PrintHelp then
				v:PrintHelp()
			end
		end
	end

	config.Finalize = function(self, filename)
		if _bam_targets[0] == "config" then
			if _bam_targets[1] == "help" then
				self:PrintHelp()
				os.exit(0)
			end

			self:Config(filename)

			os.exit(0)
		end

		self:Load(filename)
		bam_update_globalstamp(filename)
	end

	return config
end


-- Helper functions --------------------------------------
function DefaultOptionDisplay(option)
	if not option.value then return "no" end
	if option.value == 1 or option.value == true then return "yes" end
	return option.value
end

function IsNegativeTerm(s)
	if s == "no" then return true end
	if s == "false" then return true end
	if s == "off" then return true end
	if s == "disable" then return true end
	if s == "0" then return true end
	return false
end

function IsPositiveTerm(s)
	if s == "yes" then return true end
	if s == "true" then return true end
	if s == "on" then return true end
	if s == "enable" then return true end
	if s == "1" then return true end
	return false
end

function MakeOption(name, value, check, save, display, printhelp)
	local o = {}
	o.name = name
	o.value = value
	o.Check = check
	o.Save = save
	o.auto_detected = true
	o.FormatDisplay = function(self)
		local a = "SET"
		if self.auto_detected then a = "AUTO" end
		return string.format("%-5s %-20s %s", a, self.name, self:Display())
	end

	o.Display = display
	o.PrintHelp = printhelp
	if o.Display == nil then o.Display = DefaultOptionDisplay end
	return o
end


-- Test Compile C --------------------------------------
function OptTestCompileC(name, source, compileoptions, desc)
	local check = function(option, settings)
		option.value = false
		if ScriptArgs[option.name] then
			if IsNegativeTerm(ScriptArgs[option.name]) then
				option.value = false
			elseif IsPositiveTerm(ScriptArgs[option.name]) then
				option.value = true
			else
				error(ScriptArgs[option.name].." is not a valid value for option "..option.name)
			end
			option.auto_detected = false
		else
			if CTestCompile(settings, option.source, option.compileoptions) then
				option.value = true
			end
		end
	end

	local save = function(option, output)
		output:option(option, "value")
	end

	local printhelp = function(option)
		print("\t"..option.name.."=on|off")
		if option.desc then print("\t\t"..option.desc) end
	end

	local o = MakeOption(name, false, check, save, nil, printhelp)
	o.desc = desc
	o.source = source
	o.compileoptions = compileoptions
	return o
end


-- OptToggle --------------------------------------
function OptToggle(name, default_value, desc)
	local check = function(option, settings)
		if ScriptArgs[option.name] then
			if IsNegativeTerm(ScriptArgs[option.name]) then
				option.value = false
			elseif IsPositiveTerm(ScriptArgs[option.name]) then
				option.value = true
			else
				error(ScriptArgs[option.name].." is not a valid value for option "..option.name)
			end
		end
	end

	local save = function(option, output)
		output:option(option, "value")
	end

	local printhelp = function(option)
		print("\t"..option.name.."=on|off")
		if option.desc then print("\t\t"..option.desc) end
	end

	local o = MakeOption(name, default_value, check, save, nil, printhelp)
	o.desc = desc
	return o
end

-- OptInteger --------------------------------------
function OptInteger(name, default_value, desc)
	local check = function(option, settings)
		if ScriptArgs[option.name] then
			option.value = tonumber(ScriptArgs[option.name])
		end
	end

	local save = function(option, output)
		output:option(option, "value")
	end

	local printhelp = function(option)
		print("\t"..option.name.."=N")
		if option.desc then print("\t\t"..option.desc) end
	end

	local o = MakeOption(name, default_value, check, save, nil, printhelp)
	o.desc = desc
	return o
end


-- OptString --------------------------------------
function OptString(name, default_value, desc)
	local check = function(option, settings)
		if ScriptArgs[option.name] then
			option.value = ScriptArgs[option.name]
		end
	end

	local save = function(option, output)
		output:option(option, "value")
	end

	local printhelp = function(option)
		print("\t"..option.name.."=STRING")
		if option.desc then print("\t\t"..option.desc) end
	end

	local o = MakeOption(name, default_value, check, save, nil, printhelp)
	o.desc = desc
	return o
end

-- Find Compiler --------------------------------------
--[[@FUNCTION
	TODO
@END]]--
function OptCCompiler(name, default_driver, default_c, default_cxx, desc)
	local check = function(option, settings)
		if ScriptArgs[option.name] then
			-- set compile driver
			option.driver = ScriptArgs[option.name]

			-- set c compiler
			if ScriptArgs[option.name..".c"] then
				option.c_compiler = ScriptArgs[option.name..".c"]
			end

			-- set c+= compiler
			if ScriptArgs[option.name..".cxx"] then
				option.cxx_compiler = ScriptArgs[option.name..".cxx"]
			end

			option.auto_detected = false
		elseif option.driver then
			-- no need todo anything if we have a driver
			-- TODO: test if we can find the compiler
		else
			if ExecuteSilent("cl") == 0 then
				option.driver = "cl"
			elseif ExecuteSilent("g++ -v") == 0 then
				option.driver = "gcc"
			else
				error("no c/c++ compiler found")
			end
		end
		--setup_compiler(option.value)
	end

	local apply = function(option, settings)
		if option.driver == "cl" then
			SetDriversCL(settings)
		elseif option.driver == "gcc" then
			SetDriversGCC(settings)
		elseif option.driver == "clang" then
			SetDriversClang(settings)
		else
			error(option.driver.." is not a known c/c++ compile driver")
		end

		if option.c_compiler then settings.cc.c_compiler = option.c_compiler end
		if option.cxx_compiler then settings.cc.cxx_compiler = option.cxx_compiler end
	end

	local save = function(option, output)
		output:option(option, "driver")
		output:option(option, "c_compiler")
		output:option(option, "cxx_compiler")
	end

	local printhelp = function(option)
		local a = ""
		if option.desc then a = "for "..option.desc end
		print("\t"..option.name.."=gcc|cl|clang")
		print("\t\twhat c/c++ compile driver to use"..a)
		print("\t"..option.name..".c=FILENAME")
		print("\t\twhat c compiler executable to use"..a)
		print("\t"..option.name..".cxx=FILENAME")
		print("\t\twhat c++ compiler executable to use"..a)
	end

	local display = function(option)
		local s = option.driver
		if option.c_compiler then s = s .. " c="..option.c_compiler end
		if option.cxx_compiler then s = s .. " cxx="..option.cxx_compiler end
		return s
	end

	local o = MakeOption(name, nil, check, save, display, printhelp)
	o.desc = desc
	o.driver = false
	o.c_compiler = false
	o.cxx_compiler = false

	if default_driver then o.driver = default_driver end
	if default_c then o.c_compiler = default_c end
	if default_cxx then o.cxx_compiler = default_cxx end

	o.Apply = apply
	return o
end

-- Option Library --------------------------------------
--[[@FUNCTION
	TODO
@END]]--
function OptLibrary(name, header, desc)
	local check = function(option, settings)
		option.value = false
		option.include_path = false

		local function check_compile_include(filename, paths)
			if CTestCompile(settings, "#include <" .. filename .. ">\nint main(){return 0;}", "") then
				return ""
			end

			for k,v in pairs(paths) do
				if CTestCompile(settings, "#include <" .. filename .. ">\nint main(){return 0;}", "-I"..v) then
					return v
				end
			end

			return false
		end

		if ScriptArgs[option.name] then
			if IsNegativeTerm(ScriptArgs[option.name]) then
				option.value = false
			elseif ScriptArgs[option.name] == "system" then
				option.value = true
			else
				option.value = true
				option.include_path = ScriptArgs[option.name]
			end
			option.auto_detected = false
		else
			option.include_path = check_compile_include(option.header, {})
			if option.include_path == false then
				if option.required then
					print(name.." library not found and is required")
					error("required library not found")
				end
			else
				option.value = true
				option.include_path = false
			end
		end
	end

	local save = function(option, output)
		output:option(option, "value")
		output:option(option, "include_path")
	end

	local display = function(option)
		if option.value then
			if option.include_path then
				return option.include_path
			else
				return "(in system path)"
			end
		else
			return "not found"
		end
	end

	local printhelp = function(option)
		print("\t"..option.name.."=disable|system|PATH")
		if option.desc then print("\t\t"..option.desc) end
	end

	local o = MakeOption(name, false, check, save, display, printhelp)
	o.include_path = false
	o.header = header
	o.desc = desc
	return o
end

