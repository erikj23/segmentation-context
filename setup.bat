echo off
curl -L -o opencv.exe https://sourceforge.net/projects/opencvlibrary/files/latest/download
move opencv.exe c:\
c:\opencv.exe rem use default and install on c:
del c:\opencv.exe
rename c:\opencv OpenCV
move c:\OpenCV "c:\Program Files"
echo "Done!"
