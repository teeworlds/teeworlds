svn blame `svn -R ls | grep ^src | grep -v external | grep -v /$ | grep -v \.txt` | python scripts/process_blame.py
