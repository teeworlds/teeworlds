#!/bin/sh
docs/doctool/NaturalDocs -r -s Small -i src/ -i docs/articles -o HTML docs/output -p docs/config

#rm -Rf ~/public_html/.docs
#cp -Rf docs/output ~/public_html/.docs
