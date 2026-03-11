@echo off
setlocal
set TOKEN=11APTP2AA0SDgAUQUI8TB7_1Nfh3MGffsERn5Cu355CNegcP3cH8oEfnUWMGlbBYiZLQLHXOLO24DC2rR7
set GITHUB_USER=7723qqq
git remote set-url origin https://%GITHUB_USER%:%TOKEN%@github.com/%GITHUB_USER%/myweb.git
git push -u origin master 2>&1 | findstr /V "Invalid username"
endlocal
