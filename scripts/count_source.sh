svn blame `svn -R ls | grep ^src | grep -v external | grep -v /$` | python scripts/process_blame.py
