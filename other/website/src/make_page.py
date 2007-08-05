import sys

template = "".join(file("template.html").readlines())
content = "".join(sys.stdin.readlines())
final_page = template.replace("%CONTENT%", content)
print final_page