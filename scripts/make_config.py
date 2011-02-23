import sys

show_config_name = True

def input_config_var(name, data):
    if data[0] == "int":
        return str(input_config_int(name, data[1], data[2], data[3], data[4]))
    elif data[0] == "str":
        return input_config_str(name, data[1], data[2], data[3], data[4])

def get_config_prompt(config_name, config_help, default_value = ""):
    if show_config_name:
        if default_value:
            return "%s (%s) [%s]: " % (config_name, config_help, default_value)
        else:
            return "%s (%s): " % (config_name, config_help)
    else:
        if not default_value:
            return "%s [%s]:" % (config_help, default_value)
        else:
            return "%s: " % (connfig_help)

def input_config_str(config_name, config_help, v_default = "", v_length = 0, v_possible = []):
    while 1:
        result = raw_input(get_config_prompt(config_name, config_help, v_default))
        if not result:
            return v_default
        elif v_length > 0 and len(result) > v_length:
            print "string too long (max length is " + str(v_length) + ")"
        elif len(v_possible) > 0 and result not in v_possible:
            print "string not valid (must be one of the following: " + ", ".join(v_possible) + ")"
        else:
            return result

def input_config_int(config_name, config_help, v_default, v_min, v_max):
    while 1:
        result = raw_input(get_config_prompt(config_name, config_help, v_default))
        if not result:
            return v_default
        elif not result.isdigit():
            print "not a valid integer"
            continue

        result = int(result)
        if not (v_min <= result <= v_max):
            print "integer out of bounds (min: " + str(v_min) + ", max: " + str(v_max) + ")"
        else:
            return result

def read_config_vars(filename, type):
    type = type.upper()
    result = {}
    f = open(filename, "r")
    MAX_CLIENTS = 16
    for line in f:
        if line.count(type) == 0:
            continue
        if line[0:16] == "MACRO_CONFIG_STR":
            splitted = line.split(",")
            if len(splitted) < 6:
                continue
            result[splitted[1].strip()] = [ "str", line.split("\"")[3], line.split("\"")[1], int(splitted[2]), [] ]

        elif line[0:16] == "MACRO_CONFIG_INT":
            splitted = line.split(",")
            if len(splitted) < 7:
                continue
            result[splitted[1].strip()] = [ "int", line.split("\"")[1], eval(splitted[2]), eval(splitted[3]), eval(splitted[4]) ]
        else:
            continue
    f.close()
    return result;

def prompt_config_vars(vars):
    config = ""
    for key in vars:
        config += key + " \"" + input_config_var(key, vars[key]) + "\"\n"
    return config

def correct_config_vars(vars):
    vars["sv_gametype"][4] = [ "dm", "tdm", "ctf", "mod" ]
    delete = []
    for key in vars:
        if key[0:3] == "dbg":
            delete.append(key)
        elif key == "debug":
            delete.append(key)
        elif key == "console_output_level":
            delete.append(key)
        elif vars[key][0] == "int":
            if vars[key][3] == vars[key][4]: # min = max ?
                delete.append(key)
    for key in delete:
        vars.pop(key)
    return vars

if len(sys.argv) >= 2:
    config_type = sys.argv[1]
else:
    config_type = "server"

config_variables = read_config_vars("src/engine/shared/config_variables.h", config_type)
config_variables.update(read_config_vars("src/game/variables.h", config_type))
config_file_content = prompt_config_vars(correct_config_vars(config_variables))
config_file_name = input_config_str("output file", "Config file that will be created", "autoexec.cfg")
config_file = open(config_file_name, "w")
config_file.write(config_file_content)
config_file.close()

