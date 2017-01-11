SDL = {
	basepath = PathDir(ModuleFilename()),

	OptFind = function (name, required)
		local check = function(option, settings)
			option.value = false
			option.use_pkgconfig = false
			option.use_winlib = 0
			option.use_osxframework = false
			option.lib_path = nil
			
			if family ~= "windows" and ExecuteSilent("pkg-config sdl2") == 0 then
				option.value = true
				option.use_pkgconfig = true
			end
			
			if platform == "win32" then
				option.value = true
				option.use_winlib = 32
			elseif platform == "win64" then
				option.value = true
				option.use_winlib = 64
			end
			
			if platform == "macosx" then
				option.value = true
				option.use_osxframework = true
				option.use_pkgconfig = false
			end
		end
		
		local apply = function(option, settings)
			if option.use_pkgconfig == true then
				settings.cc.flags:Add("`pkg-config sdl2 --cflags`")
				settings.link.flags:Add("`pkg-config sdl2 --libs`")
			end

			if option.use_osxframework == true then
				client_settings.link.frameworks:Add("SDL2")
				client_settings.cc.includes:Add("/Library/Frameworks/SDL2.framework/Headers")
			end

			if option.use_winlib > 0 then
				settings.cc.includes:Add(SDL.basepath .. "/include")
				if option.use_winlib == 32 then
					settings.link.libpath:Add(SDL.basepath .. "/lib32")
				else
					settings.link.libpath:Add(SDL.basepath .. "/lib64")
				end
				settings.link.libs:Add("SDL2")
				settings.link.libs:Add("SDL2main")
			end
		end
		
		local save = function(option, output)
			output:option(option, "value")
			output:option(option, "use_pkgconfig")
			output:option(option, "use_winlib")
			output:option(option, "use_osxframework")
		end
		
		local display = function(option)
			if option.value == true then
				if option.use_pkgconfig == true then return "using pkg-config" end
				if option.use_winlib == 32 then return "using supplied win32 libraries" end
				if option.use_winlib == 64 then return "using supplied win64 libraries" end
				if option.use_osxframework == true then return "using osx framework" end
				return "using unknown method"
			else
				if option.required then
					return "not found (required)"
				else
					return "not found (optional)"
				end
			end
		end
		
		local o = MakeOption(name, 0, check, save, display)
		o.Apply = apply
		o.include_path = nil
		o.lib_path = nil
		o.required = required
		return o
	end
}
