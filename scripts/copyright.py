import sys, os

notice = "/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */\n"

def fix_copyright_notice(filename):
	f = open(filename, "rb")
	lines = f.readlines()
	f.close()
	
	if "/*" in lines[0] and "copyright" in lines[0]:
		lines[0] = notice
	else:
		lines = [notice] + lines
	file(filename, "wb").writelines(lines)
	
for root, dirs, files in os.walk("src"):
    for name in files:
    	filename = os.path.join(root, name)
    	process = 0
    	if ".h" == filename[-2:] or ".c" == filename[-2:] or ".cpp" == filename[-4:]:
    		process = 1
    	if os.sep + "external" + os.sep in filename:
    		process = 0
    	
    	if process:
    		fix_copyright_notice(filename)
