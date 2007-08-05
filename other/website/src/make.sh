#!/bin/sh

python make_page.py < content_home.html > ../index.html
python make_page.py < content_team.html > ../team.html
python make_page.py < content_download.html > ../download.html
python make_page.py < content_support.html > ../support.html
