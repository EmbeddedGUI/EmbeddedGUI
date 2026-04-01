@echo off
setlocal
python "%~dp0emcc_wrapper.py" %*
exit /b %ERRORLEVEL%
