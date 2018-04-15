
xstat=`docker container ls -a --format "{{.Status}}" -f "name=wtest"`

echo $xstat

echo ${#xstat}