if [ $GHQ ]; then
	scp $GHQ_USER@astro:/workspace/usrp1-e-dev/u-boot-overo/u-boot.bin /media/mmcblk0p1/
else
	scp balister@192.168.1.167:src/git/u-boot/u-boot.bin /media/mmcblk0p1/ 
fi
sync

