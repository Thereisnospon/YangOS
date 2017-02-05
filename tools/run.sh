if [ $# -ne 1 ]
	then 
		echo 'error:no input file'
		exit
else
	if [ -e $1 ]
		then
			continue
		else
			echo 'error:no such file: $1'
			exit
		fi
fi

rm ./build/*.img
echo 'ok'
nasm $1 -o build/os.img
bochs -f bochs.conf 