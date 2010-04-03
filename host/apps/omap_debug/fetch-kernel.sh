if [ $GHQ]; then
	scp $GHQ_USER@astro:/workspace/usrp1-e-dev/kernel_usrp/arch/arm/boot/uImage /media/mmcblk0p1/uImage
else
	scp balister@192.168.1.167:src/git/kernel_usrp/arch/arm/boot/uImage /media/mmcblk0p1/uImage
sync

