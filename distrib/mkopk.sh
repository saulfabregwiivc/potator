testfile=mkopk.sh
if test -f "$testfile"; then
	rm potator.opk
	cp ../potator.dge potator/potator.dge
	mksquashfs potator potator.opk
else
	echo "Script did not run, likely due to being in the wrong directory. Make sure you are in the distrib folder"
fi
